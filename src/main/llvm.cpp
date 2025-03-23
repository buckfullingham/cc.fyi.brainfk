#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/ValueSymbolTable.h>
#include <llvm/IR/Verifier.h>

#include <array>
#include <memory>
#include <stack>
#include <variant>

struct Module : llvm::Module {
  using llvm::Module::Module;
  [[maybe_unused]] char buf[1 << 10];
};

int main() {
  std::string_view program =
      "++++++++++[>+>+++>+++++++>++++++++++<<<<-]>>>++.>+++++.<<<.";
  auto ctx = std::make_unique<llvm::LLVMContext>();
  auto module = std::make_unique<Module>("brainfk", *ctx);
  auto builder = std::make_unique<llvm::IRBuilder<>>(*ctx);
#if 0
; Function Attrs: mustprogress noinline nounwind optnone ssp uwtable
define void @program(ptr noundef %0) #0 {
  %2 = alloca ptr, align 8
  store ptr %0, ptr %2, align 8
  %3 = load ptr, ptr %2, align 8
  %4 = load i8, ptr %3, align 1
  %5 = add i8 %4, 1
  store i8 %5, ptr %3, align 1
  ret void
}
#endif

  llvm::Type *void_type = llvm::Type::getVoidTy(*ctx);
  llvm::Type *arg_types[] = {llvm::PointerType::get(*ctx, 0)};
  llvm::FunctionType *func_type =
      llvm::FunctionType::get(void_type, {arg_types, 1}, false);

  [[maybe_unused]] auto main = llvm::Function::Create(
      func_type, llvm::Function::ExternalLinkage, "brainfk_main", module.get());

  std::stack<llvm::BasicBlock *> blocks;
  blocks.push(llvm::BasicBlock::Create(*ctx, "start_of_function", main));
  builder->SetInsertPoint(blocks.top());

  auto type = builder->getInt8Ty();
  auto arg = main->getArg(0);
  auto value_name = arg->getValueName();
  auto value = value_name->getValue();
  builder->CreateLoad(type, value, "blah");

  for (char instruction : program) {
    switch (instruction) {
    case '+':
      break;
    case '-':
      break;
    case '<':
      break;
    case '>':
      break;
    case '[':
      break;
    case ']':
      break;
    case '.':
      break;
    case ',':
      break;
    default:
      break;
    }
  }

  builder->CreateRetVoid();
  llvm::verifyFunction(*main, &llvm::outs());
  llvm::verifyModule(*module, &llvm::outs());
  module->print(llvm::outs(), nullptr);
}
