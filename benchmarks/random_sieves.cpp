#include<benchmark/benchmark.h>

#include<coro>
#include<array>
#include<vector>
#include<random>

constexpr size_t steps = 20000;
constexpr size_t worker_count = 10;


/*********
 * SETUP *
 *********/
template<typename T>
struct prefetchable : std::suspend_always {
    prefetchable(const T *ptr) { __builtin_prefetch((const void *) (ptr), 0, 0); }
};

template<typename T, class ForwardIt>
static inline void rand_fill_on_host(ForwardIt first, ForwardIt last, T max) {
    std::mt19937 engine(0);
    auto generator = [&]() {
        std::uniform_int_distribution<T> distribution(0, max);
        return distribution(engine);
    };
    std::generate(first, last, generator);
}

static inline std::vector<size_t> generate_sieve(size_t size) {
    std::vector<size_t> out(size);
    rand_fill_on_host(out.begin(), out.end(), size - 1);
    return out;
}


/**
 * Regular random walk
 */
static inline size_t do_random_walk(const size_t *data, size_t start, size_t step_count) {
    size_t current = start;
    for (size_t c = 0; c < step_count; ++c) {
        current = data[current];
    }
    return current;
}


/**
 * Coroutine version of the random walk that suspends on every prefetch
 */
static inline single_task<size_t> coro_random_walk(const size_t *data, size_t start, size_t step_count) {
    size_t current = start;
    for (size_t c = 0; c < step_count; ++c) {
        current = data[current];
        co_await prefetchable(data + current);
    }
    co_return current;
}

/**
 * Benchmarks
 */
template<size_t worker_count>
static inline std::array<size_t, worker_count> run_standard(const std::vector<size_t> &sieve, size_t step_count) {
    auto results = std::array<size_t, worker_count>{};
    for (size_t i = 0; i < worker_count; ++i) {
        results[i] = do_random_walk(sieve.data(), i, step_count);
    }
    return results;
}


template<size_t worker_count>
static inline std::array<size_t, worker_count> run_coro(const std::vector<size_t> &sieve, size_t step_count) {
    auto results = std::array<size_t, worker_count>{};
    auto coroutines = std::array<single_task<size_t>, worker_count>{};

    //initialising the coroutines
    for (size_t i = 0; i < worker_count; ++i) {
        coroutines[i] = coro_random_walk(sieve.data(), i, step_count);
    }

    for (size_t i = 0; i < step_count; ++i) {
#pragma unroll
        for (auto &runner: coroutines) {
            runner();
        }
    }

    for (size_t i = 0; i < worker_count; ++i) {
        results[i] = *coroutines[i]();
    }

    return results;
}

void regular_sieve(benchmark::State &state) {
    auto size = static_cast<size_t>(state.range(0));
    auto vec = generate_sieve(size);
    size_t processed_items = 0;
    size_t acc = 0;
    for (auto _: state) {
        auto res = run_standard<worker_count>(vec, steps);
        acc = std::accumulate(res.begin(), res.end(), 0UL);
        processed_items += steps * worker_count;
    }

    state.SetLabel(std::string(std::to_string(acc)));
    state.SetItemsProcessed(static_cast<int64_t>(processed_items));
}


void coro_sieve(benchmark::State &state) {
    auto size = static_cast<size_t>(state.range(0));
    auto vec = generate_sieve(size);
    size_t processed_items = 0;
    size_t acc = 0;
    for (auto _: state) {
        auto res = run_coro<worker_count>(vec, steps);
        acc = std::accumulate(res.begin(), res.end(), 0UL);
        processed_items += steps * worker_count;
    }

    state.SetLabel(std::string(std::to_string(acc)));
    state.SetItemsProcessed(static_cast<int64_t>(processed_items));
}

BENCHMARK(regular_sieve)->Unit(benchmark::kMillisecond)->RangeMultiplier(2)->Range(2000000, 200000000);
BENCHMARK(coro_sieve)->Unit(benchmark::kMillisecond)->RangeMultiplier(2)->Range(2000000, 200000000);

BENCHMARK_MAIN();