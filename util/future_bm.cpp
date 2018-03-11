/**
 *    Copyright (C) 2018 MongoDB Inc.
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#include "mongo/platform/basic.h"

#include <benchmark/benchmark.h>

#include "mongo/bson/inline_decls.h"
#include "mongo/util/future.h"

namespace mongo {

NOINLINE_DECL int makeReadyInt() {
    benchmark::ClobberMemory();
    return 1;
}

void BM_plainIntReady(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(makeReadyInt() + 1);
    }
}

NOINLINE_DECL Future<int> makeReadyFut() {
    benchmark::ClobberMemory();
    return Future<int>::makeReady(1);
}

void BM_futureIntReady(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(makeReadyFut().get() + 1);
    }
}

void BM_futureIntReadyThen(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(makeReadyFut().then([](int i) { return i + 1; }).get());
    }
}

NOINLINE_DECL Future<int> makeReadyFutWithPromise() {
    benchmark::ClobberMemory();
    Promise<int> p;
    p.emplaceValue(1);  // before getFuture().
    return p.getFuture();
}

void BM_futureIntReadyWithPromise(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(makeReadyFutWithPromise().get() + 1);
    }
}

void BM_futureIntReadyWithPromiseThen(benchmark::State& state) {
    for (auto _ : state) {
        int i = makeReadyFutWithPromise().then([](int i) { return i + 1; }).get();
        benchmark::DoNotOptimize(i);
    }
}

NOINLINE_DECL Future<int> makeReadyFutWithPromise2() {
    // This is the same as makeReadyFutWithPromise() except that this gets the Future first.
    benchmark::ClobberMemory();
    Promise<int> p;
    auto fut = p.getFuture();
    p.emplaceValue(1);  // after getFuture().
    return fut;
}

void BM_futureIntReadyWithPromise2(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(makeReadyFutWithPromise().then([](int i) { return i + 1; }).get());
    }
}

void BM_futureIntDeferredThen(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::ClobberMemory();
        Promise<int> p;
        auto fut = p.getFuture().then([](int i) { return i + 1; });
        p.emplaceValue(1);
        benchmark::DoNotOptimize(std::move(fut).get());
    }
}

void BM_futureIntDeferredThenImmediate(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::ClobberMemory();
        Promise<int> p;
        auto fut = p.getFuture().then([](int i) { return Future<int>::makeReady(i + 1); });
        p.emplaceValue(1);
        benchmark::DoNotOptimize(std::move(fut).get());
    }
}


void BM_futureIntDeferredThenReady(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::ClobberMemory();
        Promise<int> p1;
        auto fut = p1.getFuture().then([&](int i) { return makeReadyFutWithPromise(); });
        p1.emplaceValue(1);
        benchmark::DoNotOptimize(std::move(fut).get());
    }
}

void BM_futureIntDoubleDeferredThen(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::ClobberMemory();
        Promise<int> p1;
        Promise<int> p2;
        auto fut = p1.getFuture().then([&](int i) { return p2.getFuture(); });
        p1.emplaceValue(1);
        p2.emplaceValue(1);
        benchmark::DoNotOptimize(std::move(fut).get());
    }
}

void BM_futureInt3xDeferredThenNested(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::ClobberMemory();
        Promise<int> p1;
        Promise<int> p2;
        Promise<int> p3;
        auto fut = p1.getFuture().then(
            [&](int i) { return p2.getFuture().then([&](int) { return p3.getFuture(); }); });
        p1.emplaceValue(1);
        p2.emplaceValue(1);
        p3.emplaceValue(1);
        benchmark::DoNotOptimize(std::move(fut).get());
    }
}

void BM_futureInt3xDeferredThenChained(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::ClobberMemory();
        Promise<int> p1;
        Promise<int> p2;
        Promise<int> p3;
        auto fut = p1.getFuture().then([&](int i) { return p2.getFuture(); }).then([&](int i) {
            return p3.getFuture();
        });
        p1.emplaceValue(1);
        p2.emplaceValue(1);
        p3.emplaceValue(1);
        benchmark::DoNotOptimize(std::move(fut).get());
    }
}


void BM_futureInt4xDeferredThenNested(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::ClobberMemory();
        Promise<int> p1;
        Promise<int> p2;
        Promise<int> p3;
        Promise<int> p4;
        auto fut = p1.getFuture().then([&](int i) {
            return p2.getFuture().then(
                [&](int) { return p3.getFuture().then([&](int) { return p4.getFuture(); }); });
        });
        p1.emplaceValue(1);
        p2.emplaceValue(1);
        p3.emplaceValue(1);
        p4.emplaceValue(1);
        benchmark::DoNotOptimize(std::move(fut).get());
    }
}

void BM_futureInt4xDeferredThenChained(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::ClobberMemory();
        Promise<int> p1;
        Promise<int> p2;
        Promise<int> p3;
        Promise<int> p4;
        auto fut = p1.getFuture()  //
                       .then([&](int i) { return p2.getFuture(); })
                       .then([&](int i) { return p3.getFuture(); })
                       .then([&](int i) { return p4.getFuture(); });
        p1.emplaceValue(1);
        p2.emplaceValue(1);
        p3.emplaceValue(1);
        p4.emplaceValue(1);
        benchmark::DoNotOptimize(std::move(fut).get());
    }
}


BENCHMARK(BM_plainIntReady);
BENCHMARK(BM_futureIntReady);
BENCHMARK(BM_futureIntReadyThen);
BENCHMARK(BM_futureIntReadyWithPromise);
BENCHMARK(BM_futureIntReadyWithPromiseThen);
BENCHMARK(BM_futureIntReadyWithPromise2);
BENCHMARK(BM_futureIntDeferredThen);
BENCHMARK(BM_futureIntDeferredThenImmediate);
BENCHMARK(BM_futureIntDeferredThenReady);
BENCHMARK(BM_futureIntDoubleDeferredThen);
BENCHMARK(BM_futureInt3xDeferredThenNested);
BENCHMARK(BM_futureInt3xDeferredThenChained);
BENCHMARK(BM_futureInt4xDeferredThenNested);
BENCHMARK(BM_futureInt4xDeferredThenChained);

}  // namespace mongo
