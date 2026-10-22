#include "WKTRuntimeLifecycleMonitor.h"

#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace RNWorklet {

// Runtimes live on different threads, so `listeners` is guarded. Recursive
// because listener callbacks and GC finalizers can re-enter on the same thread.
static std::recursive_mutex listenersMutex;
static std::unordered_map<jsi::Runtime *,
                          std::unordered_set<RuntimeLifecycleListener *>>
    listeners;

struct RuntimeLifecycleMonitorObject : public jsi::HostObject {
  jsi::Runtime *_rt;
  explicit RuntimeLifecycleMonitorObject(jsi::Runtime *rt) : _rt(rt) {}
  ~RuntimeLifecycleMonitorObject() {
    std::lock_guard<std::recursive_mutex> lock(listenersMutex);
    auto listenersSet = listeners.find(_rt);
    if (listenersSet == listeners.end()) {
      return;
    }
    // Pop one at a time: a callback may call removeListener on this set.
    auto &set = listenersSet->second;
    while (!set.empty()) {
      auto listener = *set.begin();
      set.erase(set.begin());
      listener->onRuntimeDestroyed(_rt);
    }
    listeners.erase(_rt);
  }
};

void RuntimeLifecycleMonitor::addListener(jsi::Runtime &rt,
                                          RuntimeLifecycleListener *listener) {
  std::lock_guard<std::recursive_mutex> lock(listenersMutex);
  auto listenersSet = listeners.find(&rt);
  if (listenersSet == listeners.end()) {
    // We install a global host object in the provided runtime, this way we can
    // use that host object destructor to get notified when the runtime is being
    // terminated. We use a unique name for the object as it gets saved with the
    // runtime's global object.
    rt.global().setProperty(
        rt, "__rnwc_rt_lifecycle_monitor",
        jsi::Object::createFromHostObject(
            rt, std::make_shared<RuntimeLifecycleMonitorObject>(&rt)));
    std::unordered_set<RuntimeLifecycleListener *> newSet;
    newSet.insert(listener);
    listeners.emplace(&rt, std::move(newSet));
  } else {
    listenersSet->second.insert(listener);
  }
}

void RuntimeLifecycleMonitor::removeListener(
    jsi::Runtime &rt, RuntimeLifecycleListener *listener) {
  std::lock_guard<std::recursive_mutex> lock(listenersMutex);
  auto listenersSet = listeners.find(&rt);
  if (listenersSet == listeners.end()) {
    // nothing to do here
  } else {
    listenersSet->second.erase(listener);
  }
}

} // namespace RNWorklet
