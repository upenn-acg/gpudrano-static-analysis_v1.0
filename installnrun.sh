# ============================
# GPUDrano Installation Script
# ============================
# Authors: Nimit Singhania, Omar Navarro Leija
#          University of Pennsylvania


# =========== Customize ================
# GPUDrano Root Directory full path!
ROOT_DIR="/home/ubuntu/gpudrano-static-analysis_v1.0"

# Specify more cores for faster build.
CORES=4

# ============ Derived =================
# LLVM Source Dir
LLVM_DIR=$ROOT_DIR/llvm

# LLVM Build Dir
LLVM_BUILD_DIR=$ROOT_DIR/build

# GPUDrano Source Dir
SRC_DIR=$ROOT_DIR/src

# GPUDrano Rodinia Benchamrks
RODINIA_DIR=$ROOT_DIR/rodinia_3.1

# =========== Build Rules =============
# Get LLVM
svn co http://llvm.org/svn/llvm-project/llvm/trunk llvm &&

# Get Clang
cd ${LLVM_DIR}/tools/ &&
svn co http://llvm.org/svn/llvm-project/cfe/trunk clang &&

# Get GPUDrano
cd ${ROOT_DIR} &&
mkdir -p ${LLVM_DIR}/lib/Transforms/UncoalescedAnalysis &&
cp -R ${SRC_DIR}/abstract-execution/* ${LLVM_DIR}/lib/Transforms/UncoalescedAnalysis/ &&
cp -R ${SRC_DIR}/uncoalesced-analysis/* ${LLVM_DIR}/lib/Transforms/UncoalescedAnalysis/ &&

cd ${ROOT_DIR} &&
mkdir -p ${LLVM_DIR}/lib/Transforms/BlockSizeInvarianceAnalysis &&
cp -R ${SRC_DIR}/abstract-execution/* ${LLVM_DIR}/lib/Transforms/BlockSizeInvarianceAnalysis/ &&
cp -R ${SRC_DIR}/bsize-invariance-analysis/* ${LLVM_DIR}/lib/Transforms/BlockSizeInvarianceAnalysis/ &&


# Add Drano to LLVM build environment
if ! grep -q UncoalescedAnalysis "${LLVM_DIR}/lib/Transforms/CMakeLists.txt" ; then
  echo "add_subdirectory(UncoalescedAnalysis)" >> ${LLVM_DIR}/lib/Transforms/CMakeLists.txt ;
fi

if ! grep -q BlockSizeInvarianceAnalysis "${LLVM_DIR}/lib/Transforms/CMakeLists.txt" ; then
  echo "add_subdirectory(BlockSizeInvarianceAnalysis)" >> ${LLVM_DIR}/lib/Transforms/CMakeLists.txt ;
fi


# Build GPUDrano
mkdir -p ${LLVM_BUILD_DIR} &&
cd ${LLVM_BUILD_DIR} &&
echo "Building LLVM..."
cmake -G 'Unix Makefiles' ${LLVM_DIR} &&
make -j $CORES
sudo make install

# ==== NVIDIA drivers, toolkit, and SDK ====
#
# Get NVIDIA CUDA 8.0 binaries (CUDA toolkit and CUDA SDK samples required only)
# (https://developer.nvidia.com/cuda-downloads)
# Steps to install on a linux machine
#   wget -P $ROOT_DIR/tools https://developer.nvidia.com/compute/cuda/8.0/Prod2/local_installers/cuda_8.0.61_375.26_linux-run
#   cd $ROOT_DIR/tools
#   sudo sh cuda_8.0.61_375.26_linux-run &&
#
# Get g++-multilib (Ubuntu 16.04 LTS)
#   sudo apt-get install g++-multilib &&

# ========== Run Benchmarks ============
# Benchmarks: CUDA programs from Rodinia Benchmark Suite 3.1
#
# Update rodinia_3.1/common/make.config with the following
# CUDA_DIR=<CUDA DIR>
# SDK_DIR=<SDK DIR>
# OPT= opt -load ${LLVM_BUILD_DIR}/lib/LLVMUncoalescedAnalysis.so
#
# Build and run benchmarks
#   cd rodinia_3.1/cuda &&
#   sh compile.sh &&
#   sh run-analysis.sh &&
#
# Analyze the summary
#   ./summarize-results.sh
#
# Cleanup benchmarks
#   sh clean-up.sh
