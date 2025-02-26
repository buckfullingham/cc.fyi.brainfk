#ifndef BRAINFK_BRAINFK_HPP
#define BRAINFK_BRAINFK_HPP

#include "util.hpp"
#include <cassert>
#include <cstdint>
#include <functional>
#include <span>
#include <vector>

namespace brainfk {

enum class op_code_t : std::uint8_t {
  iadd,
  dadd,
  zjmp,
  njmp,
  putc,
  getc,
};

struct instruction_t {
  op_code_t op_code;
  std::int32_t operand;
};

inline std::vector<instruction_t> compile(std::string_view span) {
  static const std::regex re{R"xx(>+|<+|\++|-+|[.,[\]])xx"};
  using it_t = std::cregex_iterator;
  std::vector<instruction_t> result;
  std::stack<std::int32_t> stack;
  for (it_t i{span.begin(), span.end(), re}, e; i != e; ++i) {
    auto &m = *i;
    switch (*m[0].first) {
    case '>':
      result.emplace_back(op_code_t::iadd, m[0].second - m[0].first);
      break;
    case '<':
      result.emplace_back(op_code_t::iadd, m[0].first - m[0].second);
      break;
    case '+':
      result.emplace_back(op_code_t::dadd, m[0].second - m[0].first);
      break;
    case '-':
      result.emplace_back(op_code_t::dadd, m[0].first - m[0].second);
      break;
    case '.':
      result.emplace_back(op_code_t::putc, 0);
      break;
    case ',':
      result.emplace_back(op_code_t::getc, 0);
      break;
    case '[':
      stack.push(std::int32_t(result.size()));
      result.emplace_back(op_code_t::zjmp, 0);
      break;
    case ']':
      result[stack.top()].operand = std::int32_t(result.size()) - stack.top();
      result.emplace_back(op_code_t::njmp,
                          stack.top() - std::int32_t(result.size()));
      stack.pop();
      break;
    }
  }
  return result;
}

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

  void execute(std::span<const instruction_t> program) {
    reset();
    for (auto i = program.begin(), e = program.end(); i != e; ++i) {
      switch (i->op_code) {
      case op_code_t::iadd:
        std::advance(pointer_, i->operand);
        break;
      case op_code_t::dadd:
        *pointer_ += i->operand;
        break;
      case op_code_t::zjmp:
        if (*pointer_ == 0)
          std::advance(i, i->operand);
        break;
      case op_code_t::njmp:
        if (*pointer_ != 0)
          std::advance(i, i->operand);
        break;
      case op_code_t::putc:
        putc_(*pointer_);
        break;
      case op_code_t::getc:
        *pointer_ = getc_();
        break;
      }
    }
  }

  void execute(std::span<const char> program) {
    reset();
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

  void reset() {
    memory_.fill(0);
    pointer_ = &memory_[0];
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
