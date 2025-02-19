#include "readline.hpp"
#include "repl.hpp"

int main(int argc, const char *argv[]) {
  brainfk::default_readline_t readline;
  return brainfk::repl_main(argc, argv, readline);
}
