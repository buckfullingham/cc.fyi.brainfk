#include "repl.hpp"
#include "handrolled_machine.hpp"
#include "llvm_machine.hpp"
#include "readline.hpp"
#include "util.hpp"

#include <cassert>
#include <cstdio>
#include <memory>
#include <optional>
#include <regex>
#include <string_view>

#include <unistd.h>

namespace {

struct settings_t {
  std::unique_ptr<brainfk::machine_t> machine =
      std::make_unique<brainfk::handrolled_machine_t>();
  std::optional<std::string> script_name{};
};

settings_t parse_cmdline(int argc, const char *argv[]) {
  using namespace std::literals;
  settings_t result;

  int c;
  while ((c = getopt(argc, const_cast<char **>(argv), ":m:")) != -1) {
    switch (c) {
    case 'm':
      if (optarg == "llvm"sv) {
        result.machine = std::make_unique<brainfk::llvm_machine_t>();
      } else if (optarg == "handrolled"sv) {
        result.machine = std::make_unique<brainfk::handrolled_machine_t>();
      } else {
        throw std::runtime_error("bad machine");
      }
      break;
    case ':':
      printf("-%c without argument\n", optopt);
      break;
    default:
      printf("unknown arg %c\n", optopt);
      break;
    }
  }

  if (optind < argc) {
    result.script_name = argv[optind];
  }

  return result;
}

} // namespace

int brainfk::repl_main(int argc, const char *argv[], brainfk::readline_t &rl) {
  static const std::regex quit_re{"quit|exit", std::regex_constants::icase};

  const auto settings = parse_cmdline(argc, argv);

  auto outstream = rl.outstream();
  auto instream = rl.instream();

  llvm_machine_t vm;
  std::string program;

  if (settings.script_name) {
    std::unique_ptr<FILE, void (*)(FILE *)> infile{
        fopen(settings.script_name->c_str(), "r"), [](FILE *f) {
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
