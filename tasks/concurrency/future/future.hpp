#pragma once

#include <variant>
#include <optional>
#include <exception>
#include <condition_variable>
#include <mutex>
#include <memory>

namespace stdlike {

namespace detail {
template <typename T>
class SharedState {
 public:
  std::mutex mutex_;
  std::condition_variable condition_;
  std::optional<std::variant<T, std::exception_ptr>> result_;
};
}  // namespace detail

template <typename T>
class Future;

template <typename T>
class Promise {
 public:
  Promise() : state_(std::make_shared<detail::SharedState<T>>()) {}

  Promise(const Promise&) = delete;
  Promise& operator=(const Promise&) = delete;

  Promise(Promise&&) = default;
  Promise& operator=(Promise&&) = default;

  Future<T> MakeFuture() {
    return Future<T>(state_);
  }

  void SetValue(T value) {
    {
      std::lock_guard<std::mutex> lock(state_->mutex_);
      state_->result_ = std::move(value);
    }
    state_->condition_.notify_all();
  }

  void SetException(std::exception_ptr ex) {
    {
      std::lock_guard<std::mutex> lock(state_->mutex_);
      state_->result_ = std::move(ex);
    }
    state_->condition_.notify_all();
  }

 private:
  std::shared_ptr<detail::SharedState<T>> state_;
};

template <typename T>
class Future {
  friend class Promise<T>;

 public:
  Future(const Future&) = delete;
  Future& operator=(const Future&) = delete;

  Future(Future&&) = default;
  Future& operator=(Future&&) = default;

  T Get() {
    std::unique_lock<std::mutex> lock(state_->mutex_);
    state_->condition_.wait(lock, [&]() { return state_->result_.has_value(); });

    if (std::holds_alternative<std::exception_ptr>(state_->result_.value())) {
      std::rethrow_exception(std::get<std::exception_ptr>(state_->result_.value()));
    }

    return std::get<T>(state_->result_.value());
  }

 private:
  explicit Future(std::shared_ptr<detail::SharedState<T>> state) : state_(std::move(state)) {}

  std::shared_ptr<detail::SharedState<T>> state_;
};

}  // namespace stdlike
