#ifndef BRAINFK_BRAINFK_HPP
#define BRAINFK_BRAINFK_HPP

#include "util.hpp"
#include <cassert>
#include <cstdint>
#include <functional>
#include <span>
#include <vector>

namespace brainfk {

class vm {
  static std::function<std::uint8_t()> default_getc() {
    return []() {
      std::uint8_t result;
      if (posix(::read, STDIN_FILENO, &result, 1) != 1)
        throw std::runtime_error("failed to get char");
      return result;
    };
  }

  static std::function<void(std::uint8_t)> default_putc() {
    return [](std::uint8_t c) {
      if (posix(::write, STDOUT_FILENO, &c, 1) != 1)
        throw std::runtime_error("failed to put char");
    };
  }

public:
  explicit vm(std::function<std::uint8_t()> getc = default_getc(),
              std::function<void(std::uint8_t)> putc = default_putc())
      : getc_{std::move(getc)}, putc_{std::move(putc)}, memory_{},
        pointer_{memory_.begin()} {}

  vm(const vm &) = delete;
  vm &operator=(const vm &) = delete;

  void execute(std::span<const std::uint8_t> program) {
    memory_.fill(0);

    for (auto i = program.begin(), e = program.end(); i != e; ++i) {
      switch (*i) {
      case '>':
        ++pointer_;
        break;
      case '<':
        --pointer_;
        break;
      case '+':
        ++deref();
        break;
      case '-':
        --deref();
        break;
      case '.':
        putc_(deref());
        break;
      case ',':
        deref() = getc_();
        break;
      case '[':
        if (deref() == 0) {
          i = std::find_if(i, e, [count = 0](auto j) mutable {
            if (j == '[')
              ++count;
            if (j == ']')
              --count;
            return count == 0;
          });
        }
        break;
      case ']':
        if (deref() != 0) {
          i = std::find_if(std::reverse_iterator(i + 1), program.rend(),
                           [count = 0](auto j) mutable {
                             if (j == ']')
                               ++count;
                             if (j == '[')
                               --count;
                             return count == 0;
                           })
                  .base() -
              1;
        }
        break;
      default:
        break;
      }
    }
  }

private:
  std::uint8_t &deref() {
    assert(pointer_ >= memory_.begin());
    assert(pointer_ < memory_.end());
    return *pointer_;
  }

  std::function<std::uint8_t()> getc_;
  std::function<void(std::uint8_t)> putc_;
  std::array<std::uint8_t, 30'000ull> memory_;
  std::uint8_t *pointer_;
};

} // namespace brainfk

#endif // BRAINFK_BRAINFK_HPP
