#define DEBUG_TYPE "uncoalesced-analysis"

#include "UncoalescedAnalysis.h"

using namespace llvm;

// Searches FunctionArgumentValues_ for argument values of F_. If found, returns
// the values, otherwise returns (zero) for all arguments.
GPUState UncoalescedAnalysis::BuildInitialState() const {
  std::map<const Value*, MultiplierValue> argMap;
  // Check if argument values already exist.
  if (FunctionArgumentValues_ &&
      FunctionArgumentValues_->find(F_) != FunctionArgumentValues_->end()) {
    argMap = FunctionArgumentValues_->at(F_);
  }
  GPUState st;
  for (Function::const_arg_iterator argIt = F_->arg_begin();
                              argIt != F_->arg_end(); argIt++) {
    MultiplierValue v;
    const Value* arg = &*argIt;
    // Check if argument value exists.
    if (argMap.find(arg) == argMap.end()) { v = MultiplierValue(ZERO); }
    else { v = MultiplierValue(argMap.at(arg)); }
    // If argument is a pointer, set v to address type.
    if (arg->getType()->isPointerTy()) { v.setAddressType(); }
    st.setValue(arg, v);
  }
  return st;
}

MultiplierValue UncoalescedAnalysis::getConstantExprValue(const Value* p) {
  MultiplierValue v = MultiplierValue(BOT);
  ConstantExpr *pe = const_cast<ConstantExpr*>(cast<ConstantExpr>(p));
  // Handle inline getelementptr instruction.
  GetElementPtrInst* gep = nullptr;
  if (pe->getOpcode() == Instruction::GetElementPtr) {
    DEBUG(errs() << "Inline get elementptr found \n");
    gep = cast<GetElementPtrInst>(pe->getAsInstruction());
    if (isa<ConstantExpr>(gep->getPointerOperand())) {
      pe = cast<ConstantExpr>(gep->getPointerOperand());
    }
  }
  if (pe->getOpcode() == Instruction::AddrSpaceCast) {
    DEBUG(errs() << "Special memory space found \n");
    const AddrSpaceCastInst* asci =
        cast<AddrSpaceCastInst>(pe->getAsInstruction());
    // Shared memory
    if (asci->getSrcAddressSpace() == 3) {
      v = MultiplierValue(BOT);
    }
    // Constant memory (never written)
    else if (asci->getSrcAddressSpace() == 4) {
      v = MultiplierValue(ZERO);
    }
    delete asci;
  }
  if (gep) delete gep;
  return v;
}


size_t UncoalescedAnalysis::getBaseTypeSize(
    const Value *v, const Type *ty, const DataLayout &DL) const {
  // Return value in baseSizeMap_, if found.
  if (baseSizeMap_.find(v) != baseSizeMap_.end() && baseSizeMap_.at(v) != 0) {
    return baseSizeMap_.at(v);
  }
  // Extract native type from pointer type.
  while (ty->isPointerTy()) ty = cast<PointerType>(ty)->getElementType();
  // Extract the type of array elements.
  while (ty->isArrayTy()) ty = cast<ArrayType>(ty)->getElementType();
  return DL.getTypeAllocSize(const_cast<Type*>(ty));
}

void UncoalescedAnalysis::setBaseTypeSize(
    const Value *v, size_t size) {
  baseSizeMap_[v] = size;
}

GPUState UncoalescedAnalysis::ExecuteInstruction(
    const Instruction* I, GPUState st) {
 if (isa<BinaryOperator>(I)) {
    const BinaryOperator* BO = cast<BinaryOperator>(I);
    const Value* in1 = BO->getOperand(0);
    const Value* in2 = BO->getOperand(1);
    // Get values
    MultiplierValue v1, v2;
    v1 = st.getValue(in1);
    v2 = st.getValue(in2);
    // Apply operation to get the resultant value.
    MultiplierValue v;
    auto op = BO->getOpcode();
    switch (op) {
      case Instruction::URem:
      case Instruction::SRem:
      case Instruction::AShr:
      case Instruction::LShr:
        v = v1;
        break;
      case Instruction::Add:
        v = v1 + v2;
        break;
      case Instruction::Sub:
        v = v1 + (- v2);
        break;
      case Instruction::Shl:
      case Instruction::Mul:
        v = v1 * v2;
        break;
      case Instruction::UDiv:
      case Instruction::SDiv:
        v = v1 * v2;
        break;
      case Instruction::Or:
        v = v1 || v2;
        break;
      case Instruction::And:
        v = v1 && v2;
        break;
      case Instruction::Xor:
        v = (v1 && (-v2)) || (v2 && (-v1));
        break;
      default:
        v = MultiplierValue(TOP);
        break;
    }
    st.setValue(BO, v);

  } else if (isa<CastInst>(I)) {
    st.setValue(I, st.getValue(I->getOperand(0)));

  } else if (isa<CallInst>(I)) {
    const CallInst *CI = cast<CallInst>(I);
    if (CI->isInlineAsm()) {
      st.setValue(CI, MultiplierValue(TOP));
    } else {
      // If function has no name, return!
      Function *calledF = CI->getCalledFunction();
      if (!calledF->hasName()) {
        st.setValue(CI, MultiplierValue(TOP));
      } else {
        StringRef name = calledF->getName();
        if (name.equals("llvm.nvvm.read.ptx.sreg.tid.x")) {
          st.setValue(CI, MultiplierValue(ONE));
        } else if (name.equals("llvm.nvvm.read.ptx.sreg.tid.y") ||
            name.equals("llvm.nvvm.read.ptx.sreg.tid.z") ||
            name.equals("llvm.nvvm.read.ptx.sreg.ntid.x") ||
            name.equals("llvm.nvvm.read.ptx.sreg.ntid.y") ||
            name.equals("llvm.nvvm.read.ptx.sreg.ntid.z") ||
            name.equals("llvm.nvvm.read.ptx.sreg.ctaid.x") ||
            name.equals("llvm.nvvm.read.ptx.sreg.ctaid.y") ||
            name.equals("llvm.nvvm.read.ptx.sreg.ctaid.z") ||
            name.equals("llvm.nvvm.read.ptx.sreg.nctaid.x") ||
            name.equals("llvm.nvvm.read.ptx.sreg.nctaid.y") ||
            name.equals("llvm.nvvm.read.ptx.sreg.nctaid.z")) {
          st.setValue(CI, MultiplierValue(ZERO));
        } else {
          st.setValue(CI, MultiplierValue(TOP));
        }
      }
      // If calledF is not a declaration and FunctionArgumentValues_ is not
      // nullptr, create a call context consisting of mapping from arguments
      // to their abstract values and merge it with the existing call context
      // for calledF. This represents the values that flow during the call into
      // the arguments of calledF.
      if (!calledF->isDeclaration() && FunctionArgumentValues_) {
        std::map<const Value*, MultiplierValue> argMap;
        // Check if argument values exist in the map.
        if (FunctionArgumentValues_->find(calledF) !=
                FunctionArgumentValues_->end()) {
          argMap = FunctionArgumentValues_->at(calledF);
        }
        // Iterate over arguments and update argMap.
        unsigned i = 0;
        for (auto argIt = calledF->arg_begin();
             argIt != calledF->arg_end() && i < CI->getNumArgOperands();
                                                                 argIt++) {
          const Value* arg = &*argIt;
          MultiplierValue v = st.getValue(CI->getArgOperand(i));
          if (argMap.find(arg) == argMap.end()) { argMap[arg] = v; }
          else { argMap[arg] = v.join(argMap[arg]); }
          ++i;
        }
        FunctionArgumentValues_->emplace(calledF, argMap);

        // Print called arguments.
        DEBUG(errs() << "Called function " << calledF->getName()
              << " with args (");
        for (auto argIt = calledF->arg_begin();
             argIt != calledF->arg_end(); argIt++) {
          DEBUG(errs() << argMap[&*argIt].getString() << ", ");
        }
        DEBUG(errs() << ")\n");
      }
    }

  } else if (isa<AllocaInst>(I)) {
    const AllocaInst *AI = cast<AllocaInst>(I);
    // If the allocated type is a pointer, the initial value of the
    // pointer is assumed to be threadID-independent and initialized to
    // (zero) value.
    if (AI->getAllocatedType()->isPointerTy()) {
      st.setValue(AI, MultiplierValue(ZERO));
    }

  } else if (isa<LoadInst>(I)) {
    const LoadInst *LI = cast<LoadInst>(I);
    const Value* p = LI->getPointerOperand();
    MultiplierValue v = st.getValue(p);
    // HACK to identify inline getelementptr
    if(isa<ConstantExpr>(p)) {
      v = getConstantExprValue(p);
    }
    // Detect uncoalesced accesses.
    size_t psize = getBaseTypeSize(p, p->getType(),
                                   LI->getModule()->getDataLayout());
    if (v.isAddressType() && (st.getNumThreads().getType() == TOP) &&
        ((psize > 4 && (v.getType() == ONE || v.getType() == NEGONE)) ||
         (v.getType() == TOP))) {
      UncoalescedAccesses_.insert(LI);
      DEBUG(errs() << "UNCOALESCED ACCESS FOUND in access at ");
      DEBUG(cast<Instruction>(LI)->getDebugLoc().print(errs()));
      DEBUG(errs() << " in \n      " << *LI << "\n\n");
    }
    // if p stores address, return value(p) * (incr 0)
    // else return value(p).
    if (v.isAddressType()) { v = v * MultiplierValue(ZERO); }
    // If I is a pointer, v corresponds to the address of a variable.
    // NOTICE: We implicitly assume that all global variables/arrays
    //         will be sent as a pointer argument to the kernel function.
    //         Hence, only loads on such variables are set to address type.
    if (LI->getType()->isPointerTy()) { v.setAddressType(); }

    st.setValue(LI, v);

  } else if (isa<GetElementPtrInst>(I)) {
    const GetElementPtrInst* GEPI = cast<GetElementPtrInst>(I);
    const Value* p = GEPI->getPointerOperand();
    // The resultant variable has value that is sum of the value of the
    // pointer variable + values of the index variables.
    MultiplierValue v = st.getValue(p);
    for (unsigned i = 0; i < GEPI->getNumIndices(); i++) {
      const Value* idx = GEPI->getOperand(i+1);
      v = v + st.getValue(idx);
    }

    if (isa<ConstantExpr>(p)) {
      v = getConstantExprValue(p);
    }
    // Set base size of dereferenced pointer to that of p.
    size_t size = getBaseTypeSize(p, p->getType(),
                                  GEPI->getModule()->getDataLayout());
    setBaseTypeSize(GEPI, size);
    // Set v to address type if p stores address, since v corresponds to the
    // offset increment in address stored in pointer p.
    if (st.getValue(p).isAddressType()) v.setAddressType();

    st.setValue(GEPI, v);

  } else if (isa<StoreInst>(I)) {
    const StoreInst *SI = cast<StoreInst>(I);
    const Value* p = SI->getPointerOperand();
    MultiplierValue v = st.getValue(p);

    // Detect uncoalesced accesses.
    size_t psize = getBaseTypeSize(p, p->getType(),
                                   SI->getModule()->getDataLayout());
    if (v.isAddressType() && (st.getNumThreads().getType() == TOP) &&
        ((psize > 4 && (v.getType() == ONE || v.getType() == NEGONE)) ||
         (v.getType() == TOP))) {
      UncoalescedAccesses_.insert(SI);
      DEBUG(errs() << "UNCOALESCED ACCESS FOUND in access at ");
      DEBUG(cast<Instruction>(SI)->getDebugLoc().print(errs()));
      DEBUG(errs() << " in \n      " << *SI << "\n\n");
    }

    const Value* val = SI->getValueOperand();
    if (!v.isAddressType()) {
      // Store value only if p is not address type and not an array.
      v = st.getValue(val);
      // Set it to value type explicitly.
      v.setValueType();
      st.setValue(p, v);
    }

  } else if(isa<PHINode>(I)) {
    const PHINode *PHI = cast<PHINode>(I);
    // Check if the dominator instruction for the PHI node is a conditional
    // and if the conditional is threadId-independent (i.e. the branch condition
    // is constant across threads). If so, set the output to the max of incoming
    // values. Otherwise, the output of PHI node might be a non-linear function
    // of thread ID and hence, is assigned the value (unknown).
    const BasicBlock *domBlock
        = DT_->getNode(const_cast<BasicBlock*>(I->getParent()))->getIDom()->getBlock();
    DEBUG(errs() << "...Dominator Instruction for PHI node:"
        << "\n     " << *domBlock->getTerminator() << "\n");

    if (isa<BranchInst>(domBlock->getTerminator())) {
      const BranchInst *BI = cast<BranchInst>(domBlock->getTerminator());
      // Dominating branch statement found!
      if (BI->isConditional() &&
          st.getValue(BI->getCondition()).getType() == ZERO) {
        // Branch is threadId-independent.
        MultiplierValue v = st.getValue(PHI);
        for(unsigned i = 0; i < PHI->getNumIncomingValues(); i++) {
          v = v.join(st.getValue(PHI->getIncomingValue(i)));
        }
        st.setValue(PHI, v);
      } else {
        st.setValue(PHI, MultiplierValue(TOP));
      }
    } else {
      st.setValue(PHI, MultiplierValue(TOP));
    }

  } else if (isa<CmpInst>(I)) {
    const CmpInst* CI = cast<CmpInst>(I);
    const Value* in1 = CI->getOperand(0);
    const Value* in2 = CI->getOperand(1);
    const CmpInst::Predicate pred = CI->getPredicate();
    // Compute the abstract value of the predicate.
    MultiplierValue v;
    if (pred == CmpInst::ICMP_EQ) {
      v = eq(st.getValue(in1), st.getValue(in2));
    } else if (pred == CmpInst::ICMP_NE) {
      v = neq(st.getValue(in1), st.getValue(in2));
    } else {
      v = MultiplierValue(TOP);
    }
    st.setValue(CI, v);

  } else if (isa<BranchInst>(I)) {
    const BranchInst* BI = cast<BranchInst>(I);
    if (BI->isConditional()) {
      const Value* cond = BI->getCondition();
      const BasicBlock* nb1 = BI->getSuccessor(0);
      const BasicBlock* nb2 = BI->getSuccessor(1);
      GPUState st1 = st;
      GPUState st2 = st;
      // Get the abstract value for branch condition.
      MultiplierValue v = st.getValue(cond);
      // Compute number of threads on the two branches.
      st1.setNumThreads(v && st.getNumThreads());
      st2.setNumThreads((- v) && st.getNumThreads());
      // Add new items to the buffer.
      AddBlockToExecute(nb1, st1);
      AddBlockToExecute(nb2, st2);
    } else {
      const BasicBlock* nb = BI->getSuccessor(0);
      AddBlockToExecute(nb, st);
    }

  } else if (isa<TerminatorInst>(I)) {
    // Add next blocks.
    const TerminatorInst *TI = cast<TerminatorInst>(I);
    for (unsigned i = 0; i < TI->getNumSuccessors(); i++) {
      const BasicBlock *nb = TI->getSuccessor(i);
      AddBlockToExecute(nb, st);
    }
  }
  return st;
}

void UncoalescedAnalysis::BuildAnalysisInfo(GPUState st) {
  UncoalescedAccesses_.clear();
  baseSizeMap_.clear();

  DEBUG(errs() << "-------------- computing uncoalesced accesses ------------------\n");
  errs() << "Function: " << F_->getName() << "\n";
  initialState_ = st;
  entryBlock_ = &F_->getEntryBlock();
  Execute();

  // Print uncoalesced accesses found by the analysis.
  errs() << "  Uncoalesced accesses: #" << UncoalescedAccesses_.size() << "\n";
  for (auto it = UncoalescedAccesses_.begin(), ite = UncoalescedAccesses_.end();
                                                                it != ite; ++it) {
    errs() << "  -- ";
    (*it)->getDebugLoc().print(errs());
    errs() << "\n";
  }
  errs() << "\n";
}
