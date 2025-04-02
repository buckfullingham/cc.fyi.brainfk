#ifndef ENGINE_HPP
#define ENGINE_HPP

#include <cstddef>
#include <functional>
#include <memory>
#include <string_view>

namespace brainfk {

using putc_t = std::function<void(std::byte)>;
using getc_t = std::function<std::byte()>;

struct executable_t {
  virtual ~executable_t() = default;
};

class machine_t {
public:
  using executable_ptr_t = std::unique_ptr<executable_t>;

  executable_ptr_t compile(std::string_view program) {
    return compile_impl(program);
  }

  void execute(const executable_ptr_t &executable, std::byte *mem,
               const putc_t &putc, const getc_t &getc) {
    return execute_impl(executable, mem, putc, getc);
  }

  virtual ~machine_t() = default;

private:
  virtual executable_ptr_t compile_impl(std::string_view) = 0;
  virtual void execute_impl(const executable_ptr_t &, std::byte *,
                            const putc_t &, const getc_t &) = 0;
};

} // namespace brainfk

#endif // ENGINE_HPP
