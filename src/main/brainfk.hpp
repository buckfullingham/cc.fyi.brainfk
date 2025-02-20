#ifndef BRAINFK_BRAINFK_HPP
#define BRAINFK_BRAINFK_HPP

#include <cstdint>
#include <functional>
#include <vector>

namespace brainfk {

class vm {
public:
  vm(std::function<std::uint8_t()> getc, std::function<void(std::uint8_t)> putc)
      : getc_(std::move(getc)), putc_(std::move(putc)), pointer_(0),
        memory_(30'000ull) {}

  vm(const vm &) = delete;
  vm &operator=(const vm &) = delete;

  void execute(std::span<std::uint8_t> program) {
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
    assert(pointer_ < memory_.size());
    return memory_[pointer_];
  }

  std::function<std::uint8_t()> getc_;
  std::function<void(std::uint8_t)> putc_;
  std::uint64_t pointer_;
  std::vector<std::uint8_t> memory_;
};

} // namespace brainfk

#endif // BRAINFK_BRAINFK_HPP
