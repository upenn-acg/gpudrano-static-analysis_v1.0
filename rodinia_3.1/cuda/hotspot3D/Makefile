include ../../common/make.config

CC := $(CUDA_DIR)/bin/nvcc

INCLUDE := $(CUDA_DIR)/include

SRC = 3D.cu

EXE = 3D 

OUTPUT = *.out

FLAGS = -g -G #-arch sm_20 --ptxas-options=-v
release: $(SRC)
	$(CC) $(KERNEL_DIM) $(FLAGS) $(SRC) -o $(EXE) -I$(INCLUDE) -L$(CUDA_LIB_DIR) 

enum: $(SRC)
	$(CC) $(KERNEL_DIM) $(FLAGS) -deviceemu $(SRC) -o $(EXE) -I$(INCLUDE) -L$(CUDA_LIB_DIR) 

debug: $(SRC)
	$(CC) $(KERNEL_DIM) $(FLAGS) -g $(SRC) -o $(EXE) -I$(INCLUDE) -L$(CUDA_LIB_DIR) 

debugenum: $(SRC)
	$(CC) $(KERNEL_DIM) $(FLAGS) -g -deviceemu $(SRC) -o $(EXE) -I$(INCLUDE) -L$(CUDA_LIB_DIR) 

clean: $(SRC)
	rm -f $(EXE) $(EXE).linkinfo $(OUTPUT) 

KERNELS = 3D

drano: $(addsuffix .cu, $(KERNELS))
	$(foreach KERNEL, $(KERNELS), clang++ -S -g -emit-llvm $(KERNEL).cu --cuda-gpu-arch=sm_30;)

drano_analysis: $(addsuffix .cu, $(KERNELS))
	$(foreach KERNEL, $(KERNELS), $(OPT) -instnamer -interproc-uncoalesced-analysis < $(notdir $(KERNEL))-cuda-nvptx64-nvidia-cuda-sm_30.ll > /dev/null 2>log_$(subst /,_,$(KERNEL));)

dranoclean:
	find . -name \*.ll -type f -delete; \
	rm log*
