#include "mutex.hpp"

#include <chrono>
#include <thread>  // Include thread header for std::this_thread::sleep_for
#include <catch2/catch_all.hpp>

using namespace std::chrono_literals;

TEST_CASE("mutex", "[mutex,unit]") {
  SECTION("simple") {
    stdlike::Mutex mutex;
    mutex.Lock();
    mutex.Unlock();
  }

  SECTION("lock&unlock") {
    stdlike::Mutex mutex;
    mutex.Lock();
    mutex.Unlock();
    mutex.Lock();
    mutex.Unlock();
  }

  SECTION("no shared location") {
    stdlike::Mutex mutex;
    mutex.Lock();

    stdlike::Mutex mutex2;
    mutex2.Lock();
    mutex2.Unlock();

    mutex.Unlock();
  }

  SECTION("mutual exclusion") {
    stdlike::Mutex mutex;
    bool cs = false;

    std::thread locker([&]() {
      mutex.Lock();
      cs = true;
      std::this_thread::sleep_for(3s); // FIXED: Explicitly use std::this_thread::sleep_for
      cs = false;
      mutex.Unlock();
    });

    std::this_thread::sleep_for(1s); // FIXED
    mutex.Lock();
    REQUIRE(!cs);
    mutex.Unlock();

    locker.join();
  }

  SECTION("blocking") {
    stdlike::Mutex mutex;

    // Warmup
    mutex.Lock();
    mutex.Unlock();

    std::thread sleeper([&]() {
      mutex.Lock();
      std::this_thread::sleep_for(3s); // FIXED
      mutex.Unlock();
    });

    std::thread waiter([&]() {
      std::this_thread::sleep_for(1s); // FIXED

      auto start_execute_function = std::chrono::high_resolution_clock::now();

      mutex.Lock();
      mutex.Unlock();

      auto end_execute_function = std::chrono::high_resolution_clock::now();
      auto duration_execute1 =
          std::chrono::duration_cast<std::chrono::milliseconds>(
              end_execute_function - start_execute_function);

      REQUIRE(duration_execute1.count() < 200);
    });

    sleeper.join();
    waiter.join();
  }
}
