HOME = /home/ubuntu/gpudrano-static-analysis_v1.0

# Inter-procedural BSI analysis
# - use flag `-debug' for debugging.
OPT = opt -load $(HOME)/build/lib/LLVMBlockSizeInvarianceAnalysis.so -instnamer -always-inline -interproc-bsize-invariance-analysis -debug 

# Intra-procedural BSI analysis
#OPT = opt -load $(HOME)/build/lib/LLVMBlockSizeInvarianceAnalysis.so -instnamer -always-inline -bsize-invariance-analysis #-debug 

drano: $(addsuffix .cu, $(KERNELS))
	$(foreach KERNEL, $(KERNELS), clang++ -S -g -std=c++11 -emit-llvm $(KERNEL).cu --cuda-gpu-arch=sm_30 $(INCLUDES);)

drano_analysis: $(addsuffix .cu, $(KERNELS))
	$(foreach KERNEL, $(KERNELS), $(OPT) < $(notdir $(KERNEL))-cuda-nvptx64-nvidia-cuda-sm_30.ll > /dev/null 2>log_$(subst /,_,$(KERNEL));)

dranoclean:
	find . -name \*.ll -type f -delete; \
	rm log*
