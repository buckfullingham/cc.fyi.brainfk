#ifndef LLVM_HPP
#define LLVM_HPP

#include <string_view>

namespace brainfk::llvm {

void exec(std::string_view program, unsigned char *memory,
          void (*putc)(unsigned char, void *), unsigned char (*getc)(void *),
          void *capture);

} // namespace brainfk::llvm

#endif // LLVM_HPP
