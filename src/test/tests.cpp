#include <catch2/catch_all.hpp>
#include <fakeit.hpp>

#include "brainfk.hpp"
#include "repl.hpp"
#include "util.hpp"

#include <filesystem>
#include <fstream>
#include <random>
#include <vector>

#include <fcntl.h>
#include <unistd.h>

namespace {

/**
 * Drain a file descriptor into a std::string.
 */
std::string drain(int fd) {
  std::string result;
  for (;;) {
    char buf[1 << 10];
    auto n = TEMP_FAILURE_RETRY(read(fd, buf, sizeof(buf)));
    if (n == -1) {
      if (errno == EAGAIN)
        break;
      else
        throw std::system_error(errno, std::system_category());
    }
    std::copy(buf, buf + n, std::back_inserter(result));
  }
  return result;
}

/**
 * Make a pipe with a non-blocking read end.
 */
std::array<int, 2> make_pipe() {
  std::array<int, 2> result{};
  brainfk::posix(pipe, result.begin());
  const int flags = brainfk::posix(fcntl, result[0], F_GETFL);
  brainfk::posix(fcntl, result[0], F_SETFL, flags | O_NONBLOCK);
  return result;
}

/**
 * Make a FILE stream from a file descriptor.
 */
FILE *make_file(int fd, const char *mode) {
  auto result = fdopen(fd, mode);
  if (result == nullptr)
    throw std::system_error(errno, std::system_category());
  return result;
}

std::tuple<std::filesystem::path, std::fstream>
make_temp_file(std::mt19937 &prng) {
  for (int i = 0; i < 10; ++i) {
    std::uniform_int_distribution<char> dist{'a', 'z'};
    std::string filename;
    std::generate_n(std::back_inserter(filename), 10,
                    [&]() { return dist(prng); });

    auto path = std::filesystem::temp_directory_path() / filename;

    std::fstream f{path, std::ios_base::in | std::ios_base::out |
                             std::ios_base::trunc | std::ios_base::noreplace};

    if (f.is_open())
      return {std::move(path), std::move(f)};
  }

  throw std::runtime_error("can't create temp file");
}

struct fixture_t {
  fixture_t()
      : stdout_pipe_(make_pipe()), stdin_pipe_(make_pipe()),
        outstream_(make_file(stdout_pipe_[1], "w")),
        instream_(make_file(stdin_pipe_[0], "r")) {
    using namespace fakeit;
    When(Method(mock_, outstream)).AlwaysReturn(outstream_);
    When(Method(mock_, instream)).AlwaysReturn(instream_);
    When(Method(mock_, add_history)).AlwaysDo([&](auto line) {
      history_.emplace_back(line);
    });
    When(Method(mock_, on_new_line)).AlwaysReturn();
  }

  ~fixture_t() {
    fclose(instream_);
    fclose(outstream_);
    close(stdin_pipe_[0]);
    close(stdin_pipe_[1]);
    close(stdout_pipe_[0]);
    close(stdout_pipe_[1]);
  }

  std::array<int, 2> stdout_pipe_;
  std::array<int, 2> stdin_pipe_;
  FILE *outstream_;
  FILE *instream_;
  fakeit::Mock<brainfk::readline_t> mock_;
  std::vector<std::string> history_;
};

} // namespace

TEST_CASE_METHOD(fixture_t,
                 "repl_main echoes back its input and exits on quit", "[repl]") {
  using namespace std::literals;
  using namespace fakeit;

  char script[] = "++++++++++[>+>+++>+++++++>++++++++++<<<<-]>>>++.>+++++.<<<.";
  char quit[] = "quit";

  When(Method(mock_, readline)).Return(strdup(script)).Return(strdup("")).Return(strdup(quit));

  const char *argv[] = {"repl"};
  brainfk::repl_main(1, argv, mock_.get());

  CHECK(drain(stdout_pipe_[0]) == "Hi\n\n"sv);
  CHECK(history_ == std::vector<std::string>{script, quit});
}

TEST_CASE_METHOD(fixture_t, "repl echoes a file provided on the command line", "[repl]") {
  using namespace std::literals;

  std::mt19937 prng{Catch::rngSeed()};
  auto [fpath, fstream] = make_temp_file(prng);
  brainfk::guard fguard{
      [&]() {
        fstream.close();
        std::filesystem::remove(fpath);
      }
  };

  char script[] = "++++++++++[>+>+++>+++++++>++++++++++<<<<-]>>>++.>+++++.<<<.";

  fstream << script << '\n';
  CHECK(!fstream.fail());
  fstream.close();

  const char *argv[] = {"repl", fpath.c_str()};
  CHECK(brainfk::repl_main(2, argv, mock_.get()) == EXIT_SUCCESS);

  CHECK(drain(stdout_pipe_[0]) == "Hi\n"sv);
  CHECK(history_.empty());
}

TEST_CASE("vm can execute the cc test script", "[brainfk][vm]") {
  std::string result;

  brainfk::vm vm([]() -> std::uint8_t { std::abort(); },
                 [&](std::uint8_t b) -> void { result.push_back(char(b)); });

  std::uint8_t script[] = R"xx(
    This is a test Brainf*ck script written
    for Coding Challenges!
    ++++++++++[>+>+++>+++++++>++++++++++<<<
    <-]>>>++.>+.+++++++..+++.<<++++++++++++
    ++.------------.>-----.>.-----------.++
    +++.+++++.-------.<<.>.>+.-------.+++++
    ++++++..-------.+++++++++.-------.--.++
    ++++++++++++. What does it do?
  )xx";

  vm.execute(script);
  CHECK(result == "Hello, Coding Challenges");
  result.clear();
  vm.execute(script);
  CHECK(result == "Hello, Coding Challenges");
}
