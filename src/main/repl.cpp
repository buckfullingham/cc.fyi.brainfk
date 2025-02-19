#include "repl.hpp"
#include "readline.hpp"
#include "util.hpp"

#include <cassert>
#include <cstdio>
#include <iostream>
#include <regex>

int brainfk::repl_main(int, const char *[], brainfk::readline_t &rl) {
  std::regex quit_re{"quit", std::regex_constants::icase};

  auto outstream = rl.outstream();
  assert(outstream);

  try {
    while (auto line = brainfk::adopt_c_ptr(rl.readline("ccbf> "))) {
      if (*line) {
        rl.add_history(line.get());
        if (std::regex_match(line.get(), quit_re)) {
          break;
        }
        fprintf(outstream, "%s\n", line.get());
        fflush(outstream);
        rl.on_new_line();
      }
    }
  } catch (const std::exception &) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
