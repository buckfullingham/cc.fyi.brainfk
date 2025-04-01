#include "llvm.hpp"

#include <llvm-c/Analysis.h>
#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>

#include <array>
#include <bits/codecvt.h>
#include <cassert>
#include <iostream>
#include <optional>
#include <stack>

namespace {

template <typename T> class llvm_ptr {
public:
  llvm_ptr() : ref_(), dtor_() {}
  llvm_ptr(auto (*dtor)(T), T ref) : ref_(ref), dtor_(dtor) {}

  llvm_ptr(const llvm_ptr &) = delete;
  llvm_ptr &operator=(const llvm_ptr &) = delete;
  llvm_ptr(llvm_ptr &&) = delete;
  llvm_ptr &operator=(llvm_ptr &&) = delete;

  [[nodiscard]] const T &get() const { return *ref_; }
  [[nodiscard]] T &get() { return *ref_; }

  ~llvm_ptr() {
    if (ref_) {
      assert(dtor_ != nullptr);
      dtor_(*ref_);
    }
  }

private:
  std::optional<T> ref_;
  void (*dtor_)(T);
};

} // namespace

void brainfk::llvm::exec(std::string_view program, std::byte *memory,
                         void (*bfputc)(std::byte, void *),
                         std::byte (*bfgetc)(void *), void *capture) {
  LLVMInitializeNativeTarget();
  LLVMInitializeNativeAsmPrinter();
  LLVMInitializeNativeAsmParser();
  LLVMInitializeNativeDisassembler();
  LLVMLinkInMCJIT();

  auto ctx = llvm_ptr(LLVMContextDispose, LLVMContextCreate());
  auto module = llvm_ptr(LLVMDisposeModule,
                         LLVMModuleCreateWithNameInContext("", ctx.get()));

  auto void_type = LLVMVoidTypeInContext(ctx.get());
  auto byte_type = LLVMInt8TypeInContext(ctx.get());
  auto ptr_type = LLVMPointerType(byte_type, 0);
  auto void_ptr_type = LLVMPointerType(void_type, 0);
  auto byte_0 = LLVMConstInt(byte_type, 0, false);
  auto byte_1 = LLVMConstInt(byte_type, 1, false);
  auto byte_minus_1 = LLVMConstInt(byte_type, -1, true);

  std::array putc_param_types{byte_type, void_ptr_type};
  auto putc_type = LLVMFunctionType(void_type, putc_param_types.begin(),
                                    putc_param_types.size(), 0);
  auto putc_ptr_type = LLVMPointerType(putc_type, 0);

  std::array getc_param_types{void_ptr_type};
  auto getc_type = LLVMFunctionType(byte_type, getc_param_types.begin(),
                                    getc_param_types.size(), 0);
  auto getc_ptr_type = LLVMPointerType(getc_type, 0);

  std::array main_arg_types{ptr_type, putc_ptr_type, getc_ptr_type,
                            void_ptr_type};
  auto main_type = LLVMFunctionType(void_type, main_arg_types.begin(),
                                    main_arg_types.size(), 0);
  auto main = LLVMAddFunction(module.get(), "brainfk_main", main_type);
  LLVMSetLinkage(main, LLVMExternalLinkage);

  auto builder = llvm_ptr(LLVMDisposeBuilder, LLVMCreateBuilder());

  LLVMPositionBuilderAtEnd(builder.get(),
                           LLVMAppendBasicBlockInContext(ctx.get(), main, ""));

  const auto ptr = LLVMBuildAlloca(builder.get(), ptr_type, "");
  const auto putc_ptr = LLVMBuildAlloca(builder.get(), putc_ptr_type, "");
  const auto getc_ptr = LLVMBuildAlloca(builder.get(), getc_ptr_type, "");
  const auto capture_ptr = LLVMBuildAlloca(builder.get(), void_ptr_type, "");

  LLVMBuildStore(builder.get(), LLVMGetParam(main, 0), ptr);
  LLVMBuildStore(builder.get(), LLVMGetParam(main, 1), putc_ptr);
  LLVMBuildStore(builder.get(), LLVMGetParam(main, 2), getc_ptr);
  LLVMBuildStore(builder.get(), LLVMGetParam(main, 3), capture_ptr);

  std::stack<LLVMBasicBlockRef> stack;

  for (auto instruction : program) {
    switch (instruction) {
    case '+': {
      auto ref = LLVMBuildLoad2(builder.get(), ptr_type, ptr, "");
      auto last = LLVMBuildLoad2(builder.get(), byte_type, ref, "");
      last = LLVMBuildAdd(builder.get(), last, byte_1, "");
      last = LLVMBuildStore(builder.get(), last, ref);
      break;
    }
    case '-': {
      auto ref = LLVMBuildLoad2(builder.get(), ptr_type, ptr, "");
      auto last = LLVMBuildLoad2(builder.get(), byte_type, ref, "");
      last = LLVMBuildSub(builder.get(), last, byte_1, "");
      last = LLVMBuildStore(builder.get(), last, ref);
      break;
    }
    case '>': {
      auto last = LLVMBuildLoad2(builder.get(), ptr_type, ptr, "");
      last = LLVMBuildGEP2(builder.get(), byte_type, last, &byte_1, 1, "");
      last = LLVMBuildStore(builder.get(), last, ptr);
      break;
    }
    case '<': {
      auto last = LLVMBuildLoad2(builder.get(), ptr_type, ptr, "");
      last =
          LLVMBuildGEP2(builder.get(), byte_type, last, &byte_minus_1, 1, "");
      last = LLVMBuildStore(builder.get(), last, ptr);
      break;
    }
    case '[': {
      auto head = LLVMAppendBasicBlockInContext(ctx.get(), main, "head");
      auto body = LLVMAppendBasicBlockInContext(ctx.get(), main, "body");
      auto tail = LLVMAppendBasicBlockInContext(ctx.get(), main, "tail");
      auto next = LLVMAppendBasicBlockInContext(ctx.get(), main, "next");

      stack.push(next);
      stack.push(tail);
      stack.push(body);
      stack.push(head);

      LLVMBuildBr(builder.get(), head);
      LLVMPositionBuilderAtEnd(builder.get(), body);
      break;
    }
    case ']': {
      auto head = stack.top();
      stack.pop();
      auto body = stack.top();
      stack.pop();
      auto tail = stack.top();
      stack.pop();
      auto next = stack.top();
      stack.pop();

      LLVMBuildBr(builder.get(), tail);

      {
        LLVMPositionBuilderAtEnd(builder.get(), head);
        auto ref = LLVMBuildLoad2(builder.get(), ptr_type, ptr, "");
        auto last = LLVMBuildLoad2(builder.get(), byte_type, ref, "");
        last = LLVMBuildICmp(builder.get(), LLVMIntEQ, last, byte_0, "");
        last = LLVMBuildCondBr(builder.get(), last, next, body);
      }

      {
        LLVMPositionBuilderAtEnd(builder.get(), tail);
        auto ref = LLVMBuildLoad2(builder.get(), ptr_type, ptr, "");
        auto last = LLVMBuildLoad2(builder.get(), byte_type, ref, "");
        last = LLVMBuildICmp(builder.get(), LLVMIntEQ, last, byte_0, "");
        last = LLVMBuildCondBr(builder.get(), last, next, head);
      }

      LLVMPositionBuilderAtEnd(builder.get(), next);
      break;
    }
    case '.': {
      auto putc = LLVMBuildLoad2(builder.get(), putc_ptr_type, putc_ptr, "");
      auto addr = LLVMBuildLoad2(builder.get(), ptr_type, ptr, "");
      auto arg0 = LLVMBuildLoad2(builder.get(), byte_type, addr, "");
      auto arg1 = LLVMBuildLoad2(builder.get(), void_ptr_type, capture_ptr, "");
      std::array<LLVMValueRef, 2> args = {arg0, arg1};
      LLVMBuildCall2(builder.get(), putc_type, putc, args.begin(), args.size(),
                     "");
      break;
    }
    case ',': {
      auto getc = LLVMBuildLoad2(builder.get(), getc_ptr_type, getc_ptr, "");
      auto capture =
          LLVMBuildLoad2(builder.get(), void_ptr_type, capture_ptr, "");
      auto result =
          LLVMBuildCall2(builder.get(), getc_type, getc, &capture, 1, "");
      auto last = LLVMBuildLoad2(builder.get(), ptr_type, ptr, "");
      last = LLVMBuildGEP2(builder.get(), byte_type, last, &byte_0, 1, "");
      last = LLVMBuildStore(builder.get(), result, last);
      break;
    }
    default:
      break;
    }
  }

  LLVMBuildRetVoid(builder.get());

  if (LLVMVerifyFunction(main, LLVMReturnStatusAction))
    throw std::runtime_error("LLVMVerifyFunction failed");

  LLVMExecutionEngineRef engine;
  if (LLVMCreateJITCompilerForModule(&engine, module.get(), 3, nullptr))
    throw std::runtime_error("LLVMCreateJITCompilerForModule failed");

  auto compiled_function = reinterpret_cast<void (*)(
      std::byte *, void (*)(std::byte, void *), std::byte (*)(void *), void *)>(
      LLVMGetFunctionAddress(engine, "brainfk_main"));

  compiled_function(memory, bfputc, bfgetc, capture);
}
