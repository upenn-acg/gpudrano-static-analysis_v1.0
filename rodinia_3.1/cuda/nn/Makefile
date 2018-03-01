include ../../common/make.config

LOCAL_CC = gcc -g -O3 -Wall
CC := $(CUDA_DIR)/bin/nvcc

all : nn hurricane_gen

clean :
	rm -rf *.o nn hurricane_gen

nn : nn_cuda.cu
	$(CC) -cuda nn_cuda.cu
	$(CC) -o nn nn_cuda.cu

hurricane_gen : hurricane_gen.c
	$(LOCAL_CC) -o $@ $< -lm

#data :
#	mkdir data
#	./gen_dataset.sh


KERNELS = nn_cuda

drano: $(addsuffix .cu, $(KERNELS))
	$(foreach KERNEL, $(KERNELS), clang++ -S -g -emit-llvm $(KERNEL).cu --cuda-gpu-arch=sm_30 ;)

drano_analysis: $(addsuffix .cu, $(KERNELS))
	$(foreach KERNEL, $(KERNELS), $(OPT) -instnamer -interproc-uncoalesced-analysis < $(notdir $(KERNEL))-cuda-nvptx64-nvidia-cuda-sm_30.ll > /dev/null 2>log_$(subst /,_,$(KERNEL));)

dranoclean:
	find . -name \*.ll -type f -delete; \
	rm log*
