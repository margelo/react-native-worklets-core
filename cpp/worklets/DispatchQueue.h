#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

// https://github.com/embeddedartistry/embedded-resources/blob/master/examples/cpp/dispatch.cpp
namespace RNWorklet {

class JsiDispatchQueue {
  typedef std::function<void(void)> fp_t;

public:
  JsiDispatchQueue(std::string name, size_t thread_cnt = 1);

  ~JsiDispatchQueue();

  // dispatch and copy
  void dispatch(const fp_t &op);

  // dispatch and move
  void dispatch(fp_t &&op);

  // Deleted operations
  JsiDispatchQueue(const JsiDispatchQueue &rhs) = delete;

  JsiDispatchQueue &operator=(const JsiDispatchQueue &rhs) = delete;

  JsiDispatchQueue(JsiDispatchQueue &&rhs) = delete;

  JsiDispatchQueue &operator=(JsiDispatchQueue &&rhs) = delete;

private:
  std::string name_;
  std::mutex lock_;
  std::vector<std::thread> threads_;
  std::queue<fp_t> q_;
  std::condition_variable cv_;
  bool quit_ = false;

  void dispatch_thread_handler(void);
};
} // namespace RNWorklet