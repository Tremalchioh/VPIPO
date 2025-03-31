#include <catch2/catch_all.hpp>
#include "future.hpp"

#include <string>
#include <thread>
#include <chrono>

using namespace std::chrono_literals;

class TestException : public std::runtime_error {
public:
    TestException() : std::runtime_error("TestException") {}
};

TEST_CASE("Future", "[future,unit]") {
    SECTION("get value") {
        stdlike::Promise<int> p;
        stdlike::Future<int> f = p.MakeFuture();

        p.SetValue(42);
        REQUIRE(42 == f.Get());
    }

    SECTION("Throw Exception") {
        stdlike::Promise<int> p;
        auto f = p.MakeFuture();

        try {
            throw TestException();
        } catch (...) {
            p.SetException(std::current_exception());
        }

        REQUIRE_THROWS_AS(f.Get(), TestException);
    }

    SECTION("wait for value") {
        stdlike::Promise<std::string> p;
        auto f = p.MakeFuture();

        std::thread producer([p = std::move(p)]() mutable {
            std::this_thread::sleep_for(2s);
            p.SetValue("Hi");
        });

        REQUIRE("Hi" == f.Get());
        producer.join();
    }

    SECTION("wait for exception") {
        stdlike::Promise<std::string> p;
        auto f = p.MakeFuture();

        std::thread producer([p = std::move(p)]() mutable {
            std::this_thread::sleep_for(2s);
            try {
                throw TestException();
            } catch (...) {
                p.SetException(std::current_exception());
            }
        });

        REQUIRE_THROWS_AS(f.Get(), TestException);
        producer.join();
    }
}
