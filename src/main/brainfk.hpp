#ifndef BRAINFK_BRAINFK_HPP
#define BRAINFK_BRAINFK_HPP

#include "util.hpp"
#include <cassert>
#include <cstdint>
#include <format>
#include <functional>
#include <span>
#include <utility>
#include <vector>

namespace brainfk {

enum class op_code_t : std::uint8_t {
  padd, // move pointer by signed offset
  dadd, // (de|in)crement by signed value
  zjmp, // jump to a signed offset if zero
  njmp, // jump to a signed offset if non-zero
  putc, // output the current byte
  getc, // input into the current byte
  zero, // set 1 or more bytes to zero
};

struct instruction_t {
  op_code_t op_code;
  std::int32_t operand;
};

inline std::vector<instruction_t> compile(std::string_view span) {
  static const std::regex re{
      R"xx(((?:\[-]>)+)|(\[-])|(>+|<+|\++|-+|[.,[\]]))xx"};
  using it_t = std::cregex_iterator;
  std::vector<instruction_t> result;
  std::stack<std::tuple<std::int32_t, std::uint32_t>> stack;
  for (it_t i{span.begin(), span.end(), re}, e; i != e; ++i) {
    auto &m = *i;
    if (m[1].matched) {
      // a sequence of [-]> blocks
      result.emplace_back(op_code_t::zero, (m[1].second - m[1].first) / 4);
    } else if (m[2].matched) {
      // a [-] block
      result.emplace_back(op_code_t::zero, 0);
    } else {
      // a single instruction
      assert(m[3].matched);
      const auto input_pos = m[3].first - span.begin();
      switch (*m[3].first) {
      case '>':
        result.emplace_back(op_code_t::padd, m[3].second - m[3].first);
        break;
      case '<':
        result.emplace_back(op_code_t::padd, m[3].first - m[3].second);
        break;
      case '+':
        result.emplace_back(op_code_t::dadd, m[3].second - m[3].first);
        break;
      case '-':
        result.emplace_back(op_code_t::dadd, m[3].first - m[3].second);
        break;
      case '.':
        result.emplace_back(op_code_t::putc, 0);
        break;
      case ',':
        result.emplace_back(op_code_t::getc, 0);
        break;
      case '[':
        stack.emplace(std::int32_t(result.size()), input_pos);
        result.emplace_back(op_code_t::zjmp, 0);
        break;
      case ']':
        if (stack.empty()) {
          throw std::runtime_error{
              std::format("malformed program: unmatched ']' at {}",
                          m[3].first - span.begin())};
        }
        result[std::get<0>(stack.top())].operand =
            std::int32_t(result.size()) - std::get<0>(stack.top());
        result.emplace_back(op_code_t::njmp, std::get<0>(stack.top()) -
                                                 std::int32_t(result.size()));
        stack.pop();
        break;
      default:
        std::unreachable();
      }
    }
  }
  if (!stack.empty()) {
    throw std::runtime_error(std::format(
        "malformed program: unmatched '[' at {}", std::get<1>(stack.top())));
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
      case op_code_t::padd:
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
      case op_code_t::zero:
        if (i->operand)
          pointer_ = std::fill_n(pointer_, i->operand, 0);
        else
          *pointer_ = 0;
        break;
      }
    }
  }

  void execute(std::string_view input) {
    auto program = compile(input);
    execute({program.begin(), program.size()});
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
