include ../../common/make.config

CC := $(CUDA_DIR)/bin/nvcc

INCLUDE := $(CUDA_DIR)/include

SRC = gaussian.cu
EXE = gaussian

KERNEL = gaussian

release: $(SRC)
	$(CC) $(KERNEL_DIM) $(SRC) -o $(EXE) -I$(INCLUDE) -L$(CUDA_LIB_DIR) 

clean:
	rm gaussian log*

drano: $(KERNEL).cu
	clang++ -S -g -emit-llvm $(KERNEL).cu --cuda-gpu-arch=sm_30

drano_analysis: $(KERNEL).cu
	$(OPT) -instnamer -interproc-uncoalesced-analysis < $(KERNEL)-cuda-nvptx64-nvidia-cuda-sm_30.ll > /dev/null 2>log_$(KERNEL)
        
dranoclean:
	find . -name \*.ll -type f -delete; \
	rm log*
