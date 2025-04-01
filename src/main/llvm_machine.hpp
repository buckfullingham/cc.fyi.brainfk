#ifndef LLVM_MACHINE_HPP
#define LLVM_MACHINE_HPP

#include "machine.hpp"

namespace brainfk {
class llvm_machine_t : public machine_t {
  executable_ptr_t compile_impl(std::string_view) override;
  void execute_impl(const executable_ptr_t &, std::byte *, const putc_t &,
                    const getc_t &) override;
};
} // namespace brainfk

#endif // LLVM_MACHINE_HPP
