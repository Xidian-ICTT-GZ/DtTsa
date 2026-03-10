// MemAccessInstrumentPass.cpp  (LLVM 10 + 新 PM 插件版本)

#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {

class MemInstPass : public PassInfoMixin<MemInstPass> {
public:
    PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM) {
        LLVMContext &Ctx = M.getContext();

        // 声明 runtime Hook:
        //   void recordAccess(void *addr, int isWrite, int line, const char *file);
        FunctionCallee hookFunc = M.getOrInsertFunction(
            "recordAccess",
            FunctionType::get(
                Type::getVoidTy(Ctx),
                {
                    Type::getInt8PtrTy(Ctx), // addr
                    Type::getInt1Ty(Ctx),    // isWrite
                    Type::getInt32Ty(Ctx),   // line
                    Type::getInt8PtrTy(Ctx)  // file
                },
                false));

        // 遍历所有函数 / 基本块 / 指令
        for (Function &F : M) {
            if (F.isDeclaration())
                continue;

            for (BasicBlock &BB : F) {
                for (Instruction &I : BB) {

                    auto instrument = [&](Value *ptr, bool isWrite) {
                        IRBuilder<> builder(&I);

                        // 地址统一成 i8*
                        Value *addr =
                            builder.CreateBitCast(ptr, Type::getInt8PtrTy(Ctx));

                        // isWrite
                        Value *isWriteVal = builder.getInt1(isWrite);

                        // 行号 + 文件名，带容错
                        int line = 0;
                        std::string fileStr = "unknown";

                        if (DILocation *Loc = I.getDebugLoc()) {
                            line = Loc->getLine();

                            // 尝试从 DebugLoc / Scope 拿文件名
                            StringRef fname = Loc->getFilename();
                            if (fname.empty()) {
                                if (DIScope *S = Loc->getScope())
                                    fname = S->getFilename();
                            }
                            if (!fname.empty()) {
                                fileStr = fname.str();  // 真正拷贝到 std::string
                            }
                        }

                        Value *lineVal = builder.getInt32(line);
                        // ❗注意：这里用 std::string，再 CreateGlobalStringPtr，
                        // 不会出现 c_str() 悬空指针问题
                        Value *fileVal =
                            builder.CreateGlobalStringPtr(fileStr);

                        builder.CreateCall(hookFunc,
                                           {addr, isWriteVal, lineVal, fileVal});
                    };

                    if (auto *LI = dyn_cast<LoadInst>(&I)) {
                        if (!LI->isVolatile()) {
                            instrument(LI->getPointerOperand(), false);
                        }
                    } else if (auto *SI = dyn_cast<StoreInst>(&I)) {
                        if (!SI->isVolatile()) {
                            instrument(SI->getPointerOperand(), true);
                        }
                    }
                }
            }
        }

        return PreservedAnalyses::none();
    }
};

} // namespace

// 插件注册入口（新 PassManager 风格）
extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo llvmGetPassPluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION,
        "MemInstPass",
        LLVM_VERSION_STRING,
        [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                    if (Name == "meminst") {
                        MPM.addPass(MemInstPass());
                        return true;
                    }
                    return false;
                });
        }
    };
}
