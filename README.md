# GPU Drano

GPU Drano is a static analysis tool for finding
[uncoalesced memory accesses](https://devblogs.nvidia.com/parallelforall/how-access-global-memory-efficiently-cuda-c-kernels/)
in CUDA code.

Modern GPUs bundle threads into warps. All threads in a warp perform operations
in lockstep. Memory accesses to different memory locations can be coalesced into
a single load/store if the memory is adjacent or *close enough* in memory. When 
the memory accessed by a warp is far apart, multiple load/stores are required to
complete the memory transaction, and we say the access is *uncoalesced*.

## Details
### Implementation
GPU Drano is implemented as a compiler pass for LLVM using Google's open source CUDA
implementation: [`gpucc`](https://research.google.com/pubs/pub45226.html).
Therefore, Drano is tightly coupled with LLVM.

### Requirements
Drano requires LLVM version 6.0 or later. It also requires CUDA toolkit (version 7.5
or later) from NVIDIA. It has been tested on Ubuntu 16.04 LTS, but should work with 
most existing Linux systems.

The static analysis itself does not require a GPU. However to execute the programs
and to run dynamic anlaysis, an NVIDIA GPU and compatible drivers are required. Check 
[NVIDIA's system requirements](https://docs.nvidia.com/cuda/cuda-installation-guide-linux/index.html#system-requirements)
for more information.

### Algorithm and Performance
The details of the algorithm and the design choices can be found in our paper 
[GPUDrano: Detecting Uncoalesced Accesses in GPU Programs](https://www.cis.upenn.edu/~alur/Cav17.pdf).

## Installation
The script `installnrun.sh` included with the project details the steps required
to build and execute GPU Drano on an Ubuntu system. To run the script,
* Download GPU Drano and set `ROOT_DIR` to the path to the downloaded folder. 
* Run `sh installnrun.sh`

This would automatically install GPU Drano on a Linux system. We also describe the
installation steps below. 

### Setup LLVM

1) Get LLVM source:

   Ensure `subversion` is installed. Download the newest version of LLVM:
```
    svn co http://llvm.org/svn/llvm-project/llvm/trunk llvm
 ```
2) Get Clang source:

   Change your current working directory to `llvm/tools/` and check out `clang`
   from the svn repository:
```
    svn co http://llvm.org/svn/llvm-project/cfe/trunk clang
```
3) Add GPU Drano to LLVM:

   Copy GPU Drano's `src/` folder into the directory `llvm/lib/Transforms/UncoalescedAnalysis`
   in your source code:
```   
   cp -r src/abstract-execution/* llvm/lib/Transforms/UncoalescedAnalysis
   cp -r src/uncoalesced-analysis/* llvm/lib/Transforms/UncoalescedAnalysis
```
   This will create a folder called `llvm/lib/Transforms/UncoalescedAnalysis/`. We must 
   register our pass with the LLVM build system, `cmake`. Therefore, append 
   `add_subdirectory(UncoalescedAnalysis)` to `llvm/lib/Transforms/CMakeLists.txt`
   
   Sample CMakeLists.txt file:
```
   $>  more CMakeLists.txt
   add_subdirectory(Utils)
   add_subdirectory(Instrumentation)
   add_subdirectory(InstCombine)
   add_subdirectory(Scalar)
   add_subdirectory(IPO)
   add_subdirectory(Vectorize)
   add_subdirectory(Hello)
   add_subdirectory(ObjCARC)
   add_subdirectory(Coroutines)
   add_subdirectory(UncoalescedAnalysis)
```
4) Build LLVM and GPU Drano:

   From the root directory of Drano, create a `build/` directory. Then,
   change directory to the `build/` directory. Ensure CMake is installed on
   the system. Execute the following commands here:
```   
   cmake ../llvm 
   make
```

   That's `cmake` with the path to LLVM directory (`../llvm`). CMake configures
   LLVM for your system. It should generate several files in your current working
   directory (`build/`). The command `make` builds LLVM and Drano.

   Ideally, use `make -j N` where N is your number of cores to build with in 
   parallel as llvm takes a long time to build.

   **Notice**: LLVM build takes a large amount of memory to compile, specifically 
   at the linking step. If you have < 8 gigabytes of RAM, LLVM may fail to build.
   If so, you can rerun `make` without the `-j` option. This will only recompile the
   failed parts of the build, and still save more time than not using `-j` from
   the start.

5) Install LLVM and GPU Drano 
   Perform `sudo make install`. This should install the libraries and binaries in the
   default location (or the specified location). If installed locally, this guide
   assumes bash can find the command in it's PATH.

   The above is a quick start guide. If you're unfamiliar with LLVM you may find all
   details for installation at: http://llvm.org/docs/GettingStarted.html

### Setup NVIDIA drivers, toolkit and SDK
The script `installnrun.sh` briefly describes the process to build NVIDIA drivers,
toolkit and the SDK. The script has been commented out to avoid the risk of overriding
existing NVIDIA drivers. Note that we only need the toolkit and the SDK to run
the static analysis. We also need drivers and a working GPU to execute dynamic analysis
and CUDA programs themselves.

1) Install the NVIDIA drivers, CUDA toolkit and SDK:
   Please refer to the [NVIDIA installtion guide](https://docs.nvidia.com/cuda/cuda-installation-guide-linux/index.html)
   for instructions.

   In summary, if you use a recent and popular Linux distro you should be able 
   to use the automatic download tool to install the required drivers and sdk.

   If you already have the required drivers and SDK installed you may skip this step.
   It's probably not a good idea to override your existing drivers as this might render
   your system unusable.

2) You may require g++-multilib to install necessary libraries required by `clang`. 

### Verifying CUDA + LLVM installation
This step is *optional*. A simple CUDA program like "hello world" should now compile 
using `clang` or `clang++`:
```
clang -x cuda helloWorld.cu
```
The `-x cuda` option explicitly states the language. You may omit it as well and 
clang will infer this as a CUDA program.

**Notice:** Your clang installation may need not be able to find several internal
LLVM functions, if so, you may need to include `-lcudart`. You may also need to
point to the location of the `cudart.so`.

Example:
```
clang -x cuda -L /usr/local/cuda/targets/x86_64-linux/lib/ -lcudart helloWorld.cu
```
The path to *your* cudart.so may be different depending on your system.

### GPU Drano binary
LLVM should have generated a shared object `.so` file, called `LLVMUncoalescedAnalysis.so`, 
under the `build/lib/` directory.

## Running GPU Drano
The LLVM complier `clang` generates separate device (the gpu kernel code) and
host (code run on CPU) IR files when compiling CUDA programs. GPU Drano analyzes
the LLVM IR files for device code.

As an example, let's analyze the `Rodinia` kernel code for the `gaussian` benchmark.
We change directory into `rodinia_3.1/cuda/gaussian/`.

First we have clang generate LLVM IR files for the code we are interested in:
```
clang++ -S -g -emit-llvm gaussian.cu
```
Notice we compile with debug symbol `-g`, to keep the debug information about
source code locations in the generated IR. This is used to point source code locations
with potential uncoalesced accesses from LLVM IR.

The compilation generates two files:
```
gaussian-cuda-nvptx64-nvidia-cuda-sm_20.ll
gaussian.ll
```
We can then run the static analysis through LLVM's `opt` by specifying the path to
the GPU Drano binary and specifying pass `-interproc-uncoalesced-analysis` to be run. This
pass is an interprocedural analysis to detect uncoalesced accesses. It starts with
the analysis of the top-most function in the call-graph and then proceeds with the
analysis of its callees in a topological order. While analyzing a specific callee,
it considers the join of the call contexts of all its callers. To run an intraprocedural
analysis (that assumes all initial function arguments are independent of thread's id),
specify pass `-uncoalesced-analysis` to be run, instead of `-interproc-uncoalesced-analysis`.
```
opt -load ../../../build/lib/LLVMUncoalescedAnalysis.so -instnamer -interproc-uncoalesced-analysis < gaussian-cuda-nvptx64-nvidia-cuda-sm_20.ll > /dev/null 2> gpuDranoResults.txt
```
Notice `opt` reads the IR file from standard input. `opt` writes it's own uninteresting
output to standard out so we redirection it to `/dev/null` GPU Drano's output is
written to standard error which may be redirected to a file.

To generate verbose analysis results (LLVM IR annotated with analysis info), run
`opt` with an additional `-debug-only=uncoalesced-analysis` flag.

### Understanding GPU Drano's output
The generated results reports all accesses that might be potentially uncoalesced
in each of the GPU kernels. For example, here are the results for the analysis of
`gaussian.cu`. 
```
Analysis Results: 
Function: _Z4Fan1PfS_ii
  Uncoalesced accesses: #2
  -- gaussian.cu:295:59
  -- gaussian.cu:295:61

Analysis Results: 
Function: _Z4Fan2PfS_S_iii
  Uncoalesced accesses: #4
  -- gaussian.cu:312:38
  -- gaussian.cu:312:35
  -- gaussian.cu:312:35
  -- gaussian.cu:317:23
```
Each result item points to a potentially uncoalesced access in the source code.
For example, access to `m_cuda` at line 295, column 59 in gaussian.cu at method
`Fan1()` is uncoalesced.

### Running GPU Drano on Rodinia Benchmark Suite
Rodinia is popular benchmark suite of GPU programs consisting of 22 programs from
different domains. We analyzed the suite and found 111 real uncoalesced accesses
using static analysis. To reproduce the results, here are the steps involved:

1) Update CUDA and Drano configuration in `rodinia_3.1/common/make.config`. Set
   `CUDA_DIR` and `SDK_DIR` with NVIDIA toolkit and SDK paths. Update `opt` alias
   with path to GPU Drano binary `LLVMUncoalescedAnalysis.so`.

2) Go to benchmarks directory `rodinia_3.1/cuda`.

3) Compile benchmarks:
```
   sh compile.sh
```
4) Run GPU Drano on benchmarks (takes about 2 hours):
```
   sh run-analysis.sh
```

   Analysis of each benchmark generates results in its respective folder in log-files
   named `log_<filename>`.

5) Summarize results:
```
   ./summarize-results.sh
```
