#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>

#include <memory>

int main() {
  auto ctx = std::make_unique<llvm::LLVMContext>();
  //  auto module = std::make_unique<llvm::Module>("brainfk", *ctx);
  //  auto builder = std::make_unique<llvm::IRBuilder<>>(*ctx);

  //  auto main = llvm::Function::Create(
  //      llvm::FunctionType::get(llvm::Type::getVoidTy(*ctx),
  //                              {llvm::PointerType::get(builder->getInt8Ty(),
  //                              0)}, false),
  //      llvm::Function::ExternalLinkage, "brainfk_main", module.get());
  //
  //  auto block = llvm::BasicBlock::Create(*ctx, "entry", main);
  //
  //  builder->SetInsertPoint(block);
  //
  //  builder->CreateRetVoid();
  //  llvm::verifyFunction(*main);
}
