#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>

#include <iostream>
#include <memory>
#include <stack>

namespace {

struct Module : llvm::Module {
  using llvm::Module::Module;
  char buf[1 << 10];
};

} // namespace

unsigned char memory[30000];

int main() {
  auto ctx = std::make_unique<llvm::LLVMContext>();
  auto module = std::make_unique<Module>("brainfk", *ctx);

  auto void_type = llvm::Type::getVoidTy(*ctx);
  auto byte_type = llvm::IntegerType::get(*ctx, 8);
  auto ptr_type = llvm::PointerType::get(byte_type, 0);
  auto byte_inc = llvm::ConstantInt::get(byte_type, 1);
  auto byte_dec = llvm::ConstantInt::get(byte_type, -1);
  auto byte_zero = llvm::ConstantInt::get(byte_type, 0);

  auto putc = llvm::Function::Create(
      llvm::FunctionType::get(void_type, {byte_type}, false),
      llvm::Function::ExternalLinkage, "brainfk_putc", module.get());

  auto getc = llvm::Function::Create(
      llvm::FunctionType::get(byte_type, {}, false),
      llvm::Function::ExternalLinkage, "brainfk_getc", module.get());

  auto main = llvm::Function::Create(
      llvm::FunctionType::get(void_type, {ptr_type}, false),
      llvm::Function::ExternalLinkage, "brainfk_main", module.get());

  std::stack<llvm::BasicBlock *> stack;
  stack.push(llvm::BasicBlock::Create(*ctx, "", main));

  auto builder = std::make_unique<llvm::IRBuilder<>>(stack.top());

  // create ptr on the stack assign arg 0 into it
  auto ptr = builder->CreateAlloca(ptr_type, nullptr, "ptr");

  std::string_view program =
      "++++++++++[>+>+++>+++++++>++++++++++<<<<-]>>>++.>+++++.<<<.";

  llvm::Value *last = builder->CreateStore(main->getArg(0), ptr);

  for (auto instruction : program) {
    switch (instruction) {
    case '+': {
      last = builder->CreateLoad(byte_type, ptr);
      last = builder->CreateAdd(last, byte_inc);
      last = builder->CreateStore(last, ptr, false);
      break;
    }
    case '-': {
      last = builder->CreateLoad(byte_type, ptr);
      last = builder->CreateAdd(last, byte_dec);
      last = builder->CreateStore(last, ptr, false);
      break;
    }
    case '>': {
      last = builder->CreateInBoundsGEP(byte_type, ptr, {byte_inc});
      last = builder->CreateStore(last, ptr);
      break;
    }
    case '<': {
      last = builder->CreateInBoundsGEP(byte_type, ptr, {byte_dec});
      last = builder->CreateStore(last, ptr);
      break;
    }
    case '[': {
      stack.push(llvm::BasicBlock::Create(*ctx, "", main));
      builder->SetInsertPoint(stack.top());
      break;
    }
    case ']': {
      auto next = llvm::BasicBlock::Create(*ctx, "", main);
      auto body = stack.top();
      stack.pop();
      last = builder->CreateLoad(byte_type, ptr);
      last = builder->CreateCmp(llvm::CmpInst::ICMP_EQ, last, byte_zero);
      last = builder->CreateCondBr(last, next, body);
      builder->SetInsertPoint(stack.top());
      last = builder->CreateLoad(byte_type, ptr);
      last = builder->CreateCmp(llvm::CmpInst::ICMP_EQ, last, byte_zero);
      builder->CreateCondBr(last, next, body);
      stack.top() = next;
      builder->SetInsertPoint(stack.top());
      break;
    }
    case '.': {
      last = builder->CreateLoad(byte_type, ptr);
      last = builder->CreateCall(putc, {last});
      break;
    }
    case ',': {
      last = builder->CreateCall(getc, {});
      last = builder->CreateStore(last, ptr);
      break;
    }
    default:
      break;
    }
  }

  stack.pop();
  assert(stack.empty());

  builder->CreateRetVoid();

  if (!llvm::verifyFunction(*main, &llvm::errs()))
    module->print(llvm::outs(), nullptr);

  std::string errStr;
  auto executionEngine = llvm::EngineBuilder(std::move(module))
                             .setErrorStr(&errStr)
                             .setOptLevel(*llvm::CodeGenOpt::getLevel(3))
                             .create();

  if (!executionEngine) {
    std::cerr << "Failed to create ExecutionEngine: " << errStr << std::endl;
    return 1;
  }

  // Step 9: Get a pointer to the JIT compiled function
  typedef void (*brainfk_main_t)(unsigned char *);
  auto f = reinterpret_cast<brainfk_main_t>(
      executionEngine->getPointerToFunction(main));
  f(memory);

  // Clean up
  return 0;
}
