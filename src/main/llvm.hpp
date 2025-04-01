#ifndef LLVM_HPP
#define LLVM_HPP

#include <string_view>

namespace brainfk::llvm {

void exec(std::string_view program, std::byte *memory,
          void (*putc)(std::byte, void *), std::byte (*getc)(void *),
          void *capture);

} // namespace brainfk::llvm

#endif // LLVM_HPP
