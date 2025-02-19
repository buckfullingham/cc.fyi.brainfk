#include <catch2/catch_all.hpp>
#include <fakeit.hpp>

#include "repl.hpp"
#include "util.hpp"

#include <thread>
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
      if (errno == EAGAIN) {
        break;
      } else {
        throw std::system_error(errno, std::system_category());
      }
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

struct fixture_t {
  fixture_t()
      : stdout_pipe_(make_pipe()), outstream_(make_file(stdout_pipe_[1], "w")) {
    using namespace fakeit;
    When(Method(mock_, outstream)).AlwaysReturn(outstream_);
    When(Method(mock_, add_history)).AlwaysDo([&](auto line) {
      history_.emplace_back(line);
    });
    When(Method(mock_, on_new_line)).AlwaysReturn();
  }

  ~fixture_t() {
    fclose(outstream_);
    close(stdout_pipe_[0]);
    close(stdout_pipe_[1]);
  }

  std::array<int, 2> stdout_pipe_;
  FILE *outstream_;
  fakeit::Mock<brainfk::readline_t> mock_;
  std::vector<std::string> history_;
};

} // namespace

TEST_CASE_METHOD(fixture_t,
                 "repl_main echoes back its input and exits on quit") {
  using namespace std::literals;
  using namespace fakeit;

  char script[] = "++++++++++[>+>+++>+++++++>++++++++++<<<<-]>>>++.>+++++.<<<.";
  char quit[] = "quit";

  When(Method(mock_, readline)).Return(strdup(script)).Return(strdup(quit));

  const char *argv[] = {"repl"};
  brainfk::repl_main(1, argv, mock_.get());

  CHECK(drain(stdout_pipe_[0]) == std::string{script} + "\n");
  CHECK(history_ == std::vector<std::string>{script, quit});
}
