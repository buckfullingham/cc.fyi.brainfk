#include "readline.hpp"

#include <readline/history.h>
#include <readline/readline.h>

char *brainfk::default_readline_t::readline(const char *s) {
  return ::readline(s);
}

void brainfk::default_readline_t::on_new_line() { ::rl_on_new_line(); }

void brainfk::default_readline_t::add_history(const char *s) {
  ::add_history(s);
}

FILE *brainfk::default_readline_t::outstream() {
  return ::rl_outstream ? ::rl_outstream : stdout;
}

FILE *brainfk::default_readline_t::instream() {
  return ::rl_instream ? ::rl_instream : stdin;
}
