#ifndef NETCHAT_THREAD_SAFE_QUEUE_HPP
#define NETCHAT_THREAD_SAFE_QUEUE_HPP
#include <condition_variable>
#include <mutex>
#include <queue>

template <typename T>
class TSQueue {
 public:
  template <typename U>
  void push(U&& val) {
    std::unique_lock lock{mutex_};
    queue_.push(std::forward<U>(val));
    cv_.notify_one();
  }
  T pop() {
    std::unique_lock lock{mutex_};
    cv_.wait(lock, [this] { return !queue_.empty(); });
    T val = std::move(queue_.front());
    queue_.pop();
    return val;
  }

 private:
  std::condition_variable cv_;
  std::mutex mutex_;
  std::queue<T> queue_;
};

#endif // NETCHAT_THREAD_SAFE_QUEUE_HPP
