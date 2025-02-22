//===-- Frequent Path Loop Invariant Code Motion Pass --------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===//
//
// EECS583 W25 - This pass can be used as a template for your FPLICM homework
//               assignment.
//               The passes get registered as "fplicm-correctness" and
//               "fplicm-performance".
//
//
////===-------------------------------------------------------------------===//
#include "llvm/Analysis/BlockFrequencyInfo.h"
#include "llvm/Analysis/BranchProbabilityInfo.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopIterator.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Scalar/LoopPassManager.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/LoopUtils.h"

/* *******Implementation Starts Here******* */
// You can include more Header files here
#include <unordered_set>
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/Support/raw_ostream.h"
/* *******Implementation Ends Here******* */

// Remove when submitting
#define VERBOSE

using namespace llvm;

namespace {
  struct HW2CorrectnessPass : public PassInfoMixin<HW2CorrectnessPass> {
    
    // (both directions, because of some ultra-efficient programmers who may use last loop's value in next loop)
    // Check if there is any store (or recursive check for indirect store thorugh address calculation) that might change value of this load on frequent path
    bool checkStoreOnFrequentPath (Value *addr, std::unordered_set<llvm::BasicBlock *> frequentPathBlocks) {
      bool virgin = true;
      #ifdef VERBOSE_MAX
      errs() << " src : " << loadSrc << ", dest : ";
      #endif
      for (BasicBlock *BB : frequentPathBlocks){
        for (auto &II : *BB){
          // Check if any store writes to the address it reads from
          if(II.getOpcode() == Instruction::Store){
            Value *storeDest = II.getOperand(1); // Store operand 0: value to be stored, 1: destination
            #ifdef VERBOSE_MAX
              errs() << ", " << storeDest;
            #endif
            if(addr == storeDest){
              virgin = false;
            }
          }
          else if (II.getOpcode() == Instruction::GetElementPtr){
            Value *storeDest = llvm::dyn_cast<llvm::GetElementPtrInst>(&II); // Pointer address gets stored here
            #ifdef VERBOSE_MAX
              errs() << ";; " << storeDest;
            #endif
            if(addr == storeDest){            
              // Check if any of the input operands have a store
              for (auto It = llvm::dyn_cast<llvm::GetElementPtrInst>(&II)->idx_begin(); It != llvm::dyn_cast<llvm::GetElementPtrInst>(&II)->idx_end(); ++It) {
                if (checkStoreOnFrequentPath(*It, frequentPathBlocks)) virgin = false;
              }
            }
          }
        }
      }
      return virgin;
    }

    PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM) {
      llvm::BlockFrequencyAnalysis::Result &bfi = FAM.getResult<BlockFrequencyAnalysis>(F);
      llvm::BranchProbabilityAnalysis::Result &bpi = FAM.getResult<BranchProbabilityAnalysis>(F);
      llvm::LoopAnalysis::Result &li = FAM.getResult<LoopAnalysis>(F);

      /* *******Implementation Starts Here******* */
      #ifdef VERBOSE
        errs() << "\n\n**** Rudra Start of Pass\n";
      #endif
      // Your core logic should reside here.
      for (auto &currLoop : li) {

          // Detect frequent path blocks by a forward pass

          bool frequentPathExists = true;
          std::unordered_set<BasicBlock*> frequentPathBlocks;
          BasicBlock *currBB = currLoop->getHeader(); // Start from header
          #ifdef VERBOSE
            errs() << " CurrentLoop: " << currLoop << ", detecting frequent trace on this loop \n";
          #endif
          do{
            #ifdef VERBOSE
              errs() << " CurrentBlock: " << currBB;
            #endif
            frequentPathBlocks.insert(currBB);
            frequentPathExists = false;
            for (BasicBlock* nextBB: successors(currBB)) {
              // Check if next BB is on the frequent path, if yes, move to it
              if(bpi.getEdgeProbability(currBB, nextBB) >= BranchProbability(80,100)){
                currBB = nextBB;
                frequentPathExists = true;
                break;
              }
            }
            if(currLoop->getHeader() == currBB) break; // This is the end, hold your breath and count to 10
          } while(frequentPathExists);
          
          if(!frequentPathExists){
            #ifdef VERBOSE
              errs() << " Frequent trace has not been detected in current loop. Skipping!";
            #endif
            continue;
          }

          // Detect loads on the frequent path 
          std::vector<llvm::Instruction*> hoistableLoads; 
          for (BasicBlock *currBB : frequentPathBlocks) {
            #ifdef VERBOSE
              errs() << "\n Examining BB " << currBB << " : ";
            #endif
            for (auto &I : *currBB){
              if (I.getOpcode() != Instruction::Load){
                continue;
              }
              // Instruction is load, we need to check if it's value changes along with path
              Value *loadSrc = I.getOperand(0);
              if(checkStoreOnFrequentPath(loadSrc, frequentPathBlocks)) {
                hoistableLoads.push_back(&I);
              #ifdef VERBOSE
                errs() << "\n ** ";
                I.print(errs());  // Print to stderr
                errs() << "   ";
              #endif
              }
            }
          }
          // Get terminator (last instruction of the pre-header before branch)
          llvm::Instruction *terminator;
          BasicBlock *preheader = currLoop->getLoopPreheader();
          if(preheader != NULL)
            terminator = preheader->getTerminator();
          else{
            #ifdef VERBOSE
              errs() << " Current loop does not have pre-header! Investigate";
            #endif
            continue;
          }

          for (llvm::Instruction *origLoad: hoistableLoads){

            // Pre-header actions first
            #ifdef VERBOSE
              errs() << " \n*** Performing PreHeader actions: \n";
            #endif
            // Create alloca instr before terminator (insert alloca %var1 before terminator)
            llvm::AllocaInst *AI = new llvm::AllocaInst(origLoad->getType(), 0, nullptr, llvm::dyn_cast<llvm::LoadInst>(origLoad)->getAlign(), "newreg_load_src", terminator); 
            // Hoist the load (insert %1 = load %j before terminator)
            llvm::LoadInst *LI_clone = llvm::dyn_cast<llvm::LoadInst>(origLoad->clone()); // Only hoisting loads here
            LI_clone->insertBefore(terminator);
            // Create store instr storing value of load in the alloca var (insert store %1, %var1 before terminator) %1 is LI_clone value, %var1 is Alloca value (AI)
            llvm::StoreInst *SI = new llvm::StoreInst(LI_clone, AI, terminator);

            // Create a new load instruction that replaces the original load instruction in loop and uses var1 ((%4 = load %var1) instead of (%1 = load %j))
            llvm::LoadInst *origLoad_replacement = new llvm::LoadInst(origLoad->getType(), AI, "newreg_load_dst", origLoad->getNextNode());
            for (llvm::Use &U: origLoad->uses()){
              U.getUser()->replaceUsesOfWith(origLoad, origLoad_replacement);
            }
            

            // Finally, replace all uses of %1 with %4 and all stores of %j with %var1 (In performance mode also recompute all steps for var1)
            for (llvm::BasicBlock *BB: currLoop->getBlocks()){
              for (llvm::Instruction &I: *BB){
                if (I.getOpcode() == Instruction::Load){
                  #ifdef VERBOSE_MAX
                    errs() << " \nPatching a load \n";
                  #endif
                }
                else if (I.getOpcode() == Instruction::Store){
                  #ifdef VERBOSE_MAX
                    errs() << " \nPatching a store \n";
                  #endif
                  // If I stores to original load src, then instead store to replacement load src
                  if(I.getOperand(1) == origLoad->getOperand(0)){
                    I.replaceUsesOfWith(origLoad->getOperand(0), origLoad_replacement->getOperand(0));
                  }
                }
              }
            }

            origLoad->eraseFromParent(); // In example, erased instr is %13 = load i32, ptr %7, align 4
          }
        
      }


      /* *******Implementation Ends Here******* */
      #ifdef VERBOSE
        errs() << "\n**** Rudra End of Pass\n\n\n";
      #endif

      // Your pass is modifying the source code. Figure out which analyses
      // are preserved and only return those, not all.
      return PreservedAnalyses::all();
    }
  };
  struct HW2PerformancePass : public PassInfoMixin<HW2PerformancePass> {
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM) {
      llvm::BlockFrequencyAnalysis::Result &bfi = FAM.getResult<BlockFrequencyAnalysis>(F);
      llvm::BranchProbabilityAnalysis::Result &bpi = FAM.getResult<BranchProbabilityAnalysis>(F);
      llvm::LoopAnalysis::Result &li = FAM.getResult<LoopAnalysis>(F);
      /* *******Implementation Starts Here******* */
      // This is a bonus. You do not need to attempt this to receive full credit.
      /* *******Implementation Ends Here******* */

      // Your pass is modifying the source code. Figure out which analyses
      // are preserved and only return those, not all.
      return PreservedAnalyses::all();
    }
  };
}

extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK llvmGetPassPluginInfo() {
  return {
    LLVM_PLUGIN_API_VERSION, "HW2Pass", "v0.1",
    [](PassBuilder &PB) {
      PB.registerPipelineParsingCallback(
        [](StringRef Name, FunctionPassManager &FPM,
        ArrayRef<PassBuilder::PipelineElement>) {
          if(Name == "fplicm-correctness"){
            FPM.addPass(HW2CorrectnessPass());
            return true;
          }
          if(Name == "fplicm-performance"){
            FPM.addPass(HW2PerformancePass());
            return true;
          }
          return false;
        }
      );
    }
  };
}