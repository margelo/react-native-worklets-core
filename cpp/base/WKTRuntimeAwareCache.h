#pragma once

#include <jsi/jsi.h>

#include <memory>
#include <mutex>
#include <unordered_map>
#include <utility>
#include <vector>

#include "WKTRuntimeLifecycleMonitor.h"

namespace RNWorklet {

namespace jsi = facebook::jsi;

/**
 * Provides a way to keep data specific to a jsi::Runtime instance that gets
 * cleaned up when that runtime is destroyed. This is necessary because JSI does
 * not allow for its associated objects to be retained past the runtime
 * lifetime. If an object (e.g. jsi::Values or jsi::Function instances) is kept
 * after the runtime is torn down, its destructor (once it is destroyed
 * eventually) will result in a crash (JSI objects keep a pointer to memory
 * managed by the runtime, accessing that portion of the memory after runtime is
 * deleted is the root cause of that crash).
 */
template <typename T>
class RuntimeAwareCache : public RuntimeLifecycleListener {

public:
  void onRuntimeDestroyed(jsi::Runtime *rt) override {
    // A runtime has been destroyed, so destroy the related cache.
    // The entry is destroyed after releasing the lock.
    std::unique_lock<std::mutex> lock(_mutex);
    auto node = _runtimeCaches.extract(rt);
    lock.unlock();
  }

  ~RuntimeAwareCache() {
    // Unregister without holding our lock: the monitor calls
    // onRuntimeDestroyed with its lock held, so lock order is monitor -> cache.
    std::vector<jsi::Runtime *> runtimes;
    {
      std::lock_guard<std::mutex> lock(_mutex);
      runtimes.reserve(_runtimeCaches.size());
      for (auto &cache : _runtimeCaches) {
        runtimes.push_back(cache.first);
      }
    }
    for (auto rt : runtimes) {
      // remove all `onRuntimeDestroyed` listeners.
      RuntimeLifecycleMonitor::removeListener(*rt, this);
    }
  }

  // Called from several runtimes on different threads. The returned reference
  // stays valid after unlocking since unordered_map never moves its elements.
  T &get(jsi::Runtime &rt) {
    {
      std::lock_guard<std::mutex> lock(_mutex);
      auto cache = _runtimeCaches.find(&rt);
      if (cache != _runtimeCaches.end()) {
        return cache->second;
      }
    }
    // This is the first time this Runtime has been accessed.
    // We set up a `onRuntimeDestroyed` listener for it and
    // initialize the cache map.
    RuntimeLifecycleMonitor::addListener(rt, this);

    std::lock_guard<std::mutex> lock(_mutex);
    return _runtimeCaches.try_emplace(&rt).first->second;
  }

private:
  std::mutex _mutex;
  std::unordered_map<jsi::Runtime *, T> _runtimeCaches;
};

} // namespace RNWorklet
