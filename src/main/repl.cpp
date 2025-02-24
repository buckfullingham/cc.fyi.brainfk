#include "repl.hpp"
#include "brainfk.hpp"
#include "readline.hpp"
#include "util.hpp"

#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <regex>

int brainfk::repl_main(int argc, const char *argv[], brainfk::readline_t &rl) {
  static const std::regex quit_re{"quit", std::regex_constants::icase};

  auto outstream = rl.outstream();
  auto instream = rl.instream();

  brainfk::vm vm{
      [&]() -> std::uint8_t { return std::uint8_t(::fgetc(instream)); },
      [&](std::uint8_t c) {
        ::fputc(c, outstream);
        fflush(outstream);
      }};

  std::string program;

  if (argc > 1) {
    std::unique_ptr<FILE, void (*)(FILE *)> infile{fopen(argv[1], "r"),
                                                   [](FILE *f) {
                                                     if (f != nullptr)
                                                       fclose(f);
                                                   }};

    if (!infile)
      return EXIT_FAILURE;

    char buf[1 << 13];

    for (;;) {
      if (fgets(buf, sizeof(buf), infile.get()) == nullptr)
        break;
      program += buf;
    }

    vm.execute({reinterpret_cast<const std::uint8_t *>(program.c_str()),
                program.size()});

    fflush(outstream);
    return EXIT_SUCCESS;
  }

  assert(outstream);

  try {
    while (auto line = adopt_c_ptr(rl.readline("ccbf> "))) {
      if (*line) {
        rl.add_history(line.get());
        if (std::regex_match(line.get(), quit_re))
          break;
        rl.on_new_line();
        program += line.get();
      } else {
        if (program.empty())
          continue;
        vm.execute({reinterpret_cast<const std::uint8_t *>(program.c_str()),
                    program.size()});
        ::fputc('\n', outstream);
        ::fflush(outstream);
        program.clear();
      }
    }
    return EXIT_SUCCESS;
  } catch (const std::exception &) {
    return EXIT_FAILURE;
  }
}
