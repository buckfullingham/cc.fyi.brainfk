#ifndef HANDROLLED_MACHINE_HPP
#define HANDROLLED_MACHINE_HPP

#include "machine.hpp"

namespace brainfk {

class handrolled_machine_t : public machine_t {
private:
  std::unique_ptr<executable_t> compile_impl(std::string_view,
                                             bool optimise) override;
  void execute_impl(const std::unique_ptr<executable_t> &, std::byte *,
                            const putc_t &, const getc_t &) override;
};

}

#endif //HANDROLLED_MACHINE_HPP
