#include "repl.hpp"
#include "brainfk.hpp"
#include "llvm_machine.hpp"
#include "readline.hpp"
#include "util.hpp"

#include <cassert>
#include <cstdio>
#include <memory>
#include <regex>

int brainfk::repl_main(int argc, const char *argv[], brainfk::readline_t &rl) {
  static const std::regex quit_re{"quit|exit", std::regex_constants::icase};

  auto outstream = rl.outstream();
  auto instream = rl.instream();

  llvm_machine_t vm;

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

    auto compiled = vm.compile(program);

    auto memory = std::make_unique<std::byte[]>(30'000);

    vm.execute(
        compiled, memory.get(),
        [&](std::byte c) { ::fputc(char(c), outstream); },
        [&]() -> std::byte { return std::byte(::fgetc(instream)); });

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
        auto compiled = vm.compile(program);
        auto memory = std::make_unique<std::byte[]>(30'000);
        vm.execute(
            compiled, memory.get(),
            [&](std::byte c) { ::fputc(char(c), outstream); },
            [&]() -> std::byte { return std::byte(::fgetc(instream)); });
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
