#pragma once

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

class DispatchQueue {
  typedef std::function<void(void)> fp_t;

public:
  explicit DispatchQueue(std::string name);

  ~DispatchQueue();

  // dispatch and copy
  void dispatch(const fp_t &op);

  // dispatch and move
  void dispatch(fp_t &&op);

  // Deleted operations
  DispatchQueue(const DispatchQueue &rhs) = delete;

  DispatchQueue &operator=(const DispatchQueue &rhs) = delete;

  DispatchQueue(DispatchQueue &&rhs) = delete;

  DispatchQueue &operator=(DispatchQueue &&rhs) = delete;

private:
  std::string name_;
  std::mutex lock_;
  std::thread thread_;
  std::queue<fp_t> q_;
  std::condition_variable cv_;
  bool quit_ = false;

  void dispatch_thread_handler(void);
};
} // namespace RNWorklet
