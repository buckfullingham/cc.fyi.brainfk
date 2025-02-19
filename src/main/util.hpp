#ifndef BRAINFK_UTIL_HPP
#define BRAINFK_UTIL_HPP

#include <cstdlib>
#include <memory>
#include <system_error>
#include <type_traits>

#include <cerrno>

namespace brainfk {

template <typename T>
using c_ptr = std::unique_ptr<T, std::remove_cvref_t<decltype(&std::free)>>;

template <typename T> c_ptr<T> adopt_c_ptr(T *ptr) { return {ptr, &std::free}; }

template <typename Api, typename... Args>
auto posix(Api api, Args &&...args) -> std::invoke_result_t<Api, Args...> {
  using result_t = std::invoke_result_t<Api, Args...>;
  result_t result;
  TEMP_FAILURE_RETRY(result = api(std::forward<Args>(args)...));
  if (result == result_t(-1))
    throw std::system_error(errno, std::system_category());
  return result;
}

} // namespace brainfk

#endif // BRAINFK_UTIL_HPP
