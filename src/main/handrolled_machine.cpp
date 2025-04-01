#include "handrolled_machine.hpp"
#include "util.hpp"

#include <cassert>
#include <cstdint>
#include <format>
#include <functional>
#include <span>
#include <utility>
#include <vector>

namespace {

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

struct executable_t : public brainfk::executable_t {
  explicit executable_t(std::string_view span) {
    // this regex has 3 mutually exclusive groups:
    // 1: a sequence of one or more [-]> blocks (set to zero & advance pointer)
    // 2: a single [-] block (set to zero)
    // 3: a sequence of:
    //      one or more >; or // move pointer right by number of instructions
    //      one or more <; or // move pointer left by number of instructions
    //      one or more +; or // increment by the number of instructions
    //      one or more -; or // decrement by the number of instructions
    //      one of .,[]
    static const std::regex re{
        R"xx(((?:\[-]>)+)|(\[-])|(>+|<+|\++|-+|[.,[\]]))xx"};
    using it_t = std::cregex_iterator;
    // the stack contains a 2-tuple of:
    // 0: the index of a zjmp instruction in result; and
    // 1: the index of its corresponding [ instruction in the input
    std::stack<std::tuple<std::int32_t, std::uint32_t>> stack;
    for (it_t i{span.begin(), span.end(), re}, e; i != e; ++i) {
      auto &m = *i;
      if (m[1].matched) {
        result.emplace_back(op_code_t::zero, (m[1].second - m[1].first) / 4);
      } else if (m[2].matched) {
        result.emplace_back(op_code_t::zero, 0);
      } else {
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
  }

  void operator()(std::byte *pointer_, const brainfk::putc_t &putc,
                  const brainfk::getc_t &getc) const {
    for (auto i = result.begin(), e = result.end(); i != e; ++i) {
      switch (i->op_code) {
      case op_code_t::padd:
        std::advance(pointer_, i->operand);
        break;
      case op_code_t::dadd:
        *pointer_ = std::byte(std::int32_t(*pointer_) + i->operand);
        break;
      case op_code_t::zjmp:
        if (*pointer_ == std::byte(0))
          std::advance(i, i->operand);
        break;
      case op_code_t::njmp:
        if (*pointer_ != std::byte(0))
          std::advance(i, i->operand);
        break;
      case op_code_t::putc:
        putc(*pointer_);
        break;
      case op_code_t::getc:
        *pointer_ = getc();
        break;
      case op_code_t::zero:
        if (i->operand)
          pointer_ = std::fill_n(pointer_, i->operand, std::byte(0));
        else
          *pointer_ = std::byte(0);
        break;
      }
    }
  }

  std::vector<instruction_t> result;
};

} // namespace

brainfk::machine_t::executable_ptr_t
brainfk::handrolled_machine_t::compile_impl(std::string_view program) {
  return std::make_unique<::executable_t>(program);
}

void brainfk::handrolled_machine_t::execute_impl(
    const brainfk::machine_t::executable_ptr_t &exe, std::byte *mem,
    const brainfk::putc_t &putc, const brainfk::getc_t &getc) {
  dynamic_cast<const ::executable_t &>(*exe)(mem, putc, getc);
}
