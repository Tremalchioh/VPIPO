#pragma once

#include "future.hpp"

namespace stdlike {

template <typename T>
class Promise {
 public:
  Promise()
      : mutex_(std::make_shared<std::mutex>()),
        condition_(std::make_shared<std::condition_variable>()),
        result_(std::make_shared<std::optional<std::variant<T, std::exception_ptr>>>())
  {}

  Promise(const Promise&) = delete;
  Promise& operator=(const Promise&) = delete;

  Promise(Promise&&) = default;
  Promise& operator=(Promise&&) = default;

  Future<T> MakeFuture() {
    return Future<T>(mutex_, condition_, result_);
  }

  void SetValue(T value) {
    {
      std::lock_guard<std::mutex> lock(*mutex_);
      if (result_->has_value()) {
        throw std::runtime_error("Promise already fulfilled");
      }
      *result_ = std::move(value);
    }
    condition_->notify_all();
  }

  void SetException(std::exception_ptr ex) {
    {
      std::lock_guard<std::mutex> lock(*mutex_);
      if (result_->has_value()) {
        throw std::runtime_error("Promise already fulfilled");
      }
      *result_ = std::move(ex);
    }
    condition_->notify_all();
  }

 private:
  std::shared_ptr<std::mutex> mutex_;
  std::shared_ptr<std::condition_variable> condition_;
  std::shared_ptr<std::optional<std::variant<T, std::exception_ptr>>> result_;
};

}  // namespace stdlike