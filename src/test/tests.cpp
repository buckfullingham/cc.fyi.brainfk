#include <catch2/catch_all.hpp>
#include <fakeit.hpp>

#include "repl.hpp"
#include "util.hpp"

#include <filesystem>
#include <fstream>
#include <random>
#include <vector>

#include <fcntl.h>
#include <handrolled_machine.hpp>
#include <llvm_machine.hpp>
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

struct machine_fixture_t {
  void exec(std::string_view program) {
    auto executable = machine_->compile(program);
    machine_->execute(
        executable, memory_.get(), [&](std::byte c) { output_ += char(c); },
        [&]() {
          assert(!input_.empty());
          auto result = input_[0];
          input_ = input_.substr(1);
          return std::byte(result);
        });
  }

  std::unique_ptr<brainfk::machine_t> machine_;
  std::unique_ptr<std::byte[]> memory_ = std::make_unique<std::byte[]>(30'000);
  std::string input_;
  std::string output_;
};

struct llvm_fixture_t : machine_fixture_t {
  llvm_fixture_t() { machine_ = std::make_unique<brainfk::llvm_machine_t>(); }
};

struct handrolled_fixture_t : machine_fixture_t {
  handrolled_fixture_t() {
    machine_ = std::make_unique<brainfk::handrolled_machine_t>();
  }
};

} // namespace

TEST_CASE_METHOD(fixture_t, "repl_main executes a script and exits on quit",
                 "[repl]") {
  using namespace std::literals;
  using namespace fakeit;

  char script[] = "++++++++++[>+>+++>+++++++>++++++++++<<<<-]>>>++.>+++++.<<<.";
  char quit[] = "quit";

  When(Method(mock_, readline))
      .Return(strdup(script))
      .Return(strdup(""))
      .Return(strdup(quit));

  const char *argv[] = {"repl"};
  brainfk::repl_main(1, argv, mock_.get());

  CHECK(drain(stdout_pipe_[0]) == "Hi\n\n"sv);
  CHECK(history_ == std::vector<std::string>{script, quit});
}

TEST_CASE_METHOD(fixture_t, "repl executes a file provided on the command line",
                 "[repl]") {
  using namespace std::literals;

  std::mt19937 prng{Catch::rngSeed()};
  auto [fpath, fstream] = make_temp_file(prng);
  brainfk::guard fguard{[&]() {
    fstream.close();
    std::filesystem::remove(fpath);
  }};

  char script[] = "++++++++++[>+>+++>+++++++>++++++++++<<<<-]>>>++.>+++++.<<<.";

  fstream << script << '\n';
  CHECK(!fstream.fail());
  fstream.close();

  const char *argv[] = {"repl", fpath.c_str()};
  CHECK(brainfk::repl_main(2, argv, mock_.get()) == EXIT_SUCCESS);

  CHECK(drain(stdout_pipe_[0]) == "Hi\n"sv);
  CHECK(history_.empty());
}

TEST_CASE_METHOD(handrolled_fixture_t, "vm can execute the cc test script",
                 "[brainfk][vm]") {
  std::string result;

  brainfk::handrolled_machine_t vm;

  char script[] = R"xx(
    This is a test Brainf*ck script written
    for Coding Challenges!
    ++++++++++[>+>+++>+++++++>++++++++++<<<
    <-]>>>++.>+.+++++++..+++.<<++++++++++++
    ++.------------.>-----.>.-----------.++
    +++.+++++.-------.<<.>.>+.-------.+++++
    ++++++..-------.+++++++++.-------.--.++
    ++++++++++++. What does it do?
  )xx";

  exec(script);
  CHECK(output_ == "Hello, Coding Challenges");
}

TEST_CASE_METHOD(handrolled_fixture_t, "exception on unmatched left bracket",
                 "[brainfk][vm][compile]") {
  CHECK_THROWS_AS(exec("["), std::runtime_error);
}

TEST_CASE_METHOD(handrolled_fixture_t, "exception on unmatched right bracket",
                 "[brainfk][vm][compile]") {
  CHECK_THROWS_AS(exec("]"), std::runtime_error);
}

TEST_CASE_METHOD(handrolled_fixture_t, "input and output", "[brainfk][vm]") {
  input_ = "hello.";
  std::string output;

  // echo back every character in input until you reach a '.'
  exec("+[,.----------------------------------------------]");

  CHECK(output_ == "hello.");
}

TEST_CASE_METHOD(handrolled_fixture_t, "zero compound instruction",
                 "[brainfk][vm][compile]") {
  // increment x4; set to 0; increment to 32 (' '); output
  exec("++++[-]++++++++++++++++++++++++++++++++.");

  CHECK(output_ == " ");
}

TEST_CASE_METHOD(handrolled_fixture_t, "multi zero compound instruction",
                 "[brainfk][vm][compile]") {
  exec("+>++>+++");
  CHECK(memory_[0] == std::byte(1));
  CHECK(memory_[1] == std::byte(2));
  CHECK(memory_[2] == std::byte(3));
  CHECK(memory_[3] == std::byte(0));
  exec("+>++>+++<<[-]>[-]>[-]");
  CHECK(memory_[0] == std::byte(0));
  CHECK(memory_[1] == std::byte(0));
  CHECK(memory_[2] == std::byte(0));
  CHECK(memory_[3] == std::byte(0));
}

TEST_CASE_METHOD(handrolled_fixture_t, "zjmp instruction skips a block",
                 "[brainfk][vm][compile]") {
  exec("[++>]+");
  CHECK(memory_[0] == std::byte(1));
}

TEST_CASE_METHOD(llvm_fixture_t, "llvm +") {
  exec("+");
  CHECK(memory_[0] == std::byte(1));
}

TEST_CASE_METHOD(llvm_fixture_t, "llvm -") {
  exec("-");
  CHECK(memory_[0] == std::byte(255));
}

TEST_CASE_METHOD(llvm_fixture_t, "llvm >+") {
  exec(">+");
  CHECK(memory_[0] == std::byte(0));
  CHECK(memory_[1] == std::byte(1));
}

TEST_CASE_METHOD(llvm_fixture_t, "llvm ><+") {
  exec("><+");
  CHECK(memory_[0] == std::byte(1));
  CHECK(memory_[1] == std::byte(0));
}

TEST_CASE_METHOD(llvm_fixture_t, "llvm +[-]>+") {
  exec("+[-]>+");
  CHECK(memory_[0] == std::byte(0));
  CHECK(memory_[1] == std::byte(1));
}

TEST_CASE_METHOD(llvm_fixture_t, "llvm .") {
  exec("+.");
  CHECK(memory_[0] == std::byte(1));
  REQUIRE(!output_.empty());
  CHECK(int(output_[0]) == 1);
}

TEST_CASE_METHOD(llvm_fixture_t, "llvm ,") {
  input_ = "B";
  exec(",.");
  CHECK(memory_[0] == std::byte('B'));
  CHECK(output_[0] == 'B');
}

TEST_CASE_METHOD(llvm_fixture_t, "llvm Hi") {
  exec("++++++++++[>+>+++>+++++++>++++++++++<<<<-]>>>++.>+++++.<<<.");
  CHECK(output_ == "Hi\n");
}

TEST_CASE_METHOD(llvm_fixture_t, "llvm cc script") {
  exec(R"xx(This is a test Brainf*ck script written
    for Coding Challenges!
    ++++++++++[>+>+++>+++++++>++++++++++<<<
    <-]>>>++.>+.+++++++..+++.<<++++++++++++
    ++.------------.>-----.>.-----------.++
    +++.+++++.-------.<<.>.>+.-------.+++++
    ++++++..-------.+++++++++.-------.--.++
    ++++++++++++. What does it do?)xx");
  CHECK(output_ == "Hello, Coding Challenges");
}

TEST_CASE_METHOD(handrolled_fixture_t, "handrolled +") {
  exec("+");
  CHECK(memory_[0] == std::byte(1));
}

TEST_CASE_METHOD(handrolled_fixture_t, "handrolled -") {
  exec("-");
  CHECK(memory_[0] == std::byte(255));
}

TEST_CASE_METHOD(handrolled_fixture_t, "handrolled >+") {
  exec(">+");
  CHECK(memory_[0] == std::byte(0));
  CHECK(memory_[1] == std::byte(1));
}

TEST_CASE_METHOD(handrolled_fixture_t, "handrolled ><-") {
  exec("><-");
  CHECK(memory_[0] == std::byte(255));
  CHECK(memory_[1] == std::byte(0));
}

TEST_CASE_METHOD(handrolled_fixture_t, "handrolled ><+") {
  exec("><+");
  CHECK(memory_[0] == std::byte(1));
  CHECK(memory_[1] == std::byte(0));
}

TEST_CASE_METHOD(handrolled_fixture_t, "handrolled ,.") {
  input_ = 'B';
  exec(",.");
  CHECK(memory_[0] == std::byte('B'));
  CHECK(output_ == "B");
}
