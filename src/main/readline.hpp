#ifndef BRAINFK_READLINE_HPP
#define BRAINFK_READLINE_HPP

#include <cstdio>

namespace brainfk {

class readline_t {
public:
  virtual char *readline(const char *prompt) = 0;
  virtual void add_history(const char *) = 0;
  virtual void on_new_line() = 0;
  virtual FILE *outstream() = 0;
  virtual ~readline_t() = default;
};

class default_readline_t : public readline_t {
public:
  char *readline(const char *prompt) override;
  void add_history(const char *) override;
  void on_new_line() override;
  FILE *outstream() override;
};

} // namespace brainfk

#endif // BRAINFK_READLINE_HPP
