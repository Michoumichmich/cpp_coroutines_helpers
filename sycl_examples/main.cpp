/**
 * @author Michel Migdal (@mgdl.fr)
 * 
 * Doing more work in parallel on a single device/several devices with nice semantics. 
 * Otherwise you would have to use vectors of states and sycl::buffer or sycl::events + states + USM. 
 * Demo using hipSYCL: https://github.com/Michoumichmich/hipSYCL/commit/5cf4359b1abc9cf48ac37fd2f90c1d91a65e1a89
 */

#include <iostream>
#include <deque>
#include <coro>
#include <sycl/sycl.hpp>

template<typename T = void>
using sycl_task = single_task<T, true, true>;

template <typename T = void>
using sycl_pipeline = std::deque<sycl_task<T>>;


/**
 * Blocks on the oldest element of the pipeline to get a result, might return std::nullopt if the oldest job isn't done.
 */
template<typename T>
auto advance_pipeline(sycl_pipeline<T> &work_pipeline) -> std::optional<T> {
    for (auto& item : work_pipeline) {
        item.resume(); /* Everyone moves forward */
    }
    /* Each call to resume will result in a wait on the event that was in the co_await */
    if (auto result = work_pipeline.front().get()) {
        work_pipeline.pop_front(); /* Returning the first ready result */
        return result;
    }
    return {};
}


/**
 * sycl_task<T> runs until the first co_await/return and propagates exceptions
 * When co_await returns, the event has finished.
 */
template<typename T>
auto sycl_task_flow_example(T in, sycl::queue q) -> sycl_task<T> {
    auto *dev_ptr = sycl::malloc_device<unsigned>(1U, q);
    /* Set up the context of the computation, heavy */
    co_await q.copy(&in, dev_ptr, 1U);
    co_await q.single_task([=]() { *dev_ptr += 1U; }); /* STAGE 1: Computation launched in background */
    unsigned i = rand() >> 20; /* Get some data from a file? */
    auto evt = q.single_task([=]() { *dev_ptr *= 2U + i; });
    co_await q.copy(dev_ptr, &in, 1U, evt); /* STAGE 2: Finishing the computation */
    sycl::free(dev_ptr, q);
    co_return (in / (2U + i)) - 1U; /* Should return 'in' */
}


int main() {
    auto work_pipeline = sycl_pipeline<unsigned>{};
    for (unsigned i = 0; i < 20; ++i) {
        work_pipeline.emplace_back(sycl_task_flow_example(i, sycl::queue{}));
        if (auto result = advance_pipeline(work_pipeline)) {
            std::cout << "Result: " << *result << ", pipeline depth" << work_pipeline.size() << std::endl; /* Process the result, some heavy computation */
        }
    }

    /* Flushing remaining work */
    while (!work_pipeline.empty()) {
        advance_pipeline(work_pipeline);
    }
}

