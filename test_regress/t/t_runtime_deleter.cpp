// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

#include "verilated.h"

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <thread>
#include <vector>

#include VM_PREFIX_INCLUDE

namespace {

std::atomic<int> s_destroyed{0};

class Chain final : public VlDeletable {
    VlDeleter& m_deleter;
    const int m_depth;

public:
    Chain(VlDeleter& deleter, int depth)
        : m_deleter{deleter}
        , m_depth{depth} {}
    ~Chain() override {
        ++s_destroyed;
        if (m_depth > 0) m_deleter.put(new Chain{m_deleter, m_depth - 1});
    }
};

void check(bool ok, const char* what) {
    if (!ok) {
        std::printf("%%Error: %s (destroyed=%d)\n", what, s_destroyed.load());
        std::exit(1);
    }
}

void testDeleter() {
    VlDeleter deleter;

    deleter.deleteAll();
    check(s_destroyed == 0, "empty deleteAll destroyed something");

    deleter.put(new Chain{deleter, 4});
    deleter.deleteAll();
    check(s_destroyed == 5, "destructor-enqueued objects not deleted by the same deleteAll");

    deleter.deleteAll();
    check(s_destroyed == 5, "repeated deleteAll changed count");

    s_destroyed = 0;
    constexpr int THREADS = 4;
    constexpr int PER_THREAD = 1000;
    std::vector<std::thread> threads;
    for (int t = 0; t < THREADS; ++t) {
        threads.emplace_back([&deleter]() {
            for (int i = 0; i < PER_THREAD; ++i) deleter.put(new Chain{deleter, i % 3});
        });
    }
    std::atomic<bool> stop{false};
    std::thread sweeper{[&deleter, &stop]() {
        while (!stop) deleter.deleteAll();
    }};
    for (std::thread& thread : threads) thread.join();
    stop = true;
    sweeper.join();
    deleter.deleteAll();
    int expected = 0;
    for (int i = 0; i < PER_THREAD; ++i) expected += 1 + i % 3;
    check(s_destroyed == THREADS * expected, "objects put by other threads not deleted");

    s_destroyed = 0;
    {
        VlDeleter scoped;
        scoped.put(new Chain{scoped, 2});
    }
    check(s_destroyed == 3, "VlDeleter destructor did not delete pending objects");
}

}  // namespace

int main(int argc, char** argv) {
    testDeleter();

    const std::unique_ptr<VerilatedContext> contextp{new VerilatedContext};
    contextp->commandArgs(argc, argv);
    const std::unique_ptr<VM_PREFIX> topp{new VM_PREFIX{contextp.get()}};
    topp->clk = 0;
    while (!contextp->gotFinish() && contextp->time() < 1000) {
        contextp->timeInc(1);
        topp->clk = !topp->clk;
        topp->eval();
    }
    check(contextp->gotFinish(), "model did not finish");
    topp->final();
    return 0;
}
