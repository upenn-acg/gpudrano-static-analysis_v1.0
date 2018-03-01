include ../../common/make.config

CC := $(CUDA_DIR)/bin/nvcc

INCLUDE := $(CUDA_DIR)/include

# Example
# target: dependencies
	# command 1
	# command 2
          # .
          # .
          # .
	# command n

# link objects(binaries) together
a.out:		main.o
	$(CC)	main.o \
				-I$(INCLUDE) \
				-L$(CUDA_LIB_DIR)  \
				-lm -lcuda -lcudart -o srad

# compile main function file into object (binary)
main.o: 	main.cu \
				define.c \
				graphics.c \
				extract_kernel.cu \
				prepare_kernel.cu \
				reduce_kernel.cu \
				srad_kernel.cu \
				srad2_kernel.cu \
				compress_kernel.cu
	nvcc	main.cu \
				-c -O3 -arch sm_35

# delete all object files
clean:
	rm *.o srad

KERNELS = main 

drano: $(addsuffix .cu, $(KERNELS))
	$(foreach KERNEL, $(KERNELS), clang++ -S -g -emit-llvm $(KERNEL).cu --cuda-gpu-arch=sm_30 ;)

drano_analysis: $(addsuffix .cu, $(KERNELS))
	$(foreach KERNEL, $(KERNELS), $(OPT) -instnamer -interproc-uncoalesced-analysis < $(notdir $(KERNEL))-cuda-nvptx64-nvidia-cuda-sm_30.ll > /dev/null 2>log_$(subst /,_,$(KERNEL));)

dranoclean:
	find . -name \*.ll -type f -delete; \
	rm log*
