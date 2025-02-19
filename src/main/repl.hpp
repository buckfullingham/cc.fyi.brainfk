#ifndef BRAINFK_REPL_HPP
#define BRAINFK_REPL_HPP

#include "readline.hpp"
#include <memory>

namespace brainfk {

int repl_main(int, const char *[], brainfk::readline_t &);

} // namespace brainfk

#endif // BRAINFK_REPL_HPP
