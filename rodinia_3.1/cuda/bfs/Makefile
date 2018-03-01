include ../../common/make.config

CC := $(CUDA_DIR)/bin/nvcc

INCLUDE := $(CUDA_DIR)/include

SRC = bfs.cu

EXE = bfs

KERNEL = bfs

release: $(SRC)
	$(CC) $(SRC) -o $(EXE) -I$(INCLUDE) -L$(CUDA_LIB_DIR) 

enum: $(SRC)
	$(CC) -deviceemu $(SRC) -o $(EXE) -I$(INCLUDE) -L$(CUDA_LIB_DIR) 

debug: $(SRC)
	$(CC) -g $(SRC) -o $(EXE) -I$(INCLUDE) -L$(CUDA_LIB_DIR) 

debugenum: $(SRC)
	$(CC) -g -deviceemu $(SRC) -o $(EXE) -I$(INCLUDE) -L$(CUDA_LIB_DIR) 

clean: $(SRC)
	rm -f $(EXE) $(EXE).linkinfo result.txt log*

drano: $(KERNEL).cu
	clang++ -S -g -emit-llvm $(KERNEL).cu --cuda-gpu-arch=sm_30 

drano_analysis: $(KERNEL).cu
	$(OPT) -instnamer -interproc-uncoalesced-analysis < $(KERNEL)-cuda-nvptx64-nvidia-cuda-sm_30.ll > /dev/null 2>log_$(KERNEL)

dranoclean:
	find . -name \*.ll -type f -delete; \
	rm log*
