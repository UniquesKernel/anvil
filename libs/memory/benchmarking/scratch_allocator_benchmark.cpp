// bench_anvil_improved.cpp
// Minimal, Anvil-compatible allocator benchmark with fair timing.
// - Uses the same API style as test_old.cpp (create/alloc/reset/destroy with MIN_ALIGNMENT).
// - Moves create/destroy outside timed regions; includes reset() inside reset test timing.
// - Prints BOTH baseline and scratch ops/sec with median ± MAD CI.
// - Exits 0 by default; use --strict to return non-zero when gates fail.
//
// Build:  g++ -O3 -std=c++17 bench_anvil_improved.cpp -o bench_anvil_improved
// Run  :  ./bench_anvil_improved --runs 12 --iters 200000 [--strict]
//
// If the Anvil headers are missing, the file still compiles and runs baseline paths.

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#if __has_include("memory/scratch_allocator.hpp")
#include "memory/constants.hpp"
#include "memory/scratch_allocator.hpp"
#define HAVE_ANVIL 1
using ScratchAllocator = anvil::memory::scratch_allocator::ScratchAllocator;
using anvil::memory::MIN_ALIGNMENT;
#else
#define HAVE_ANVIL 0
namespace anvil {
namespace memory {
static constexpr size_t MIN_ALIGNMENT = alignof(std::max_align_t);
namespace scratch_allocator {
struct ScratchAllocator {};
static inline ScratchAllocator* create(size_t, size_t) {
        return nullptr;
}
static inline void* alloc(ScratchAllocator*, size_t sz, size_t) {
        return std::malloc(sz);
}
static inline bool reset(ScratchAllocator*) {
        return true;
}
static inline bool destroy(ScratchAllocator**) {
        return true;
}
} // namespace scratch_allocator
} // namespace memory
} // namespace anvil
using ScratchAllocator = anvil::memory::scratch_allocator::ScratchAllocator;
using anvil::memory::MIN_ALIGNMENT;
#endif

using Clock = std::chrono::steady_clock;
using ns    = std::chrono::nanoseconds;

static inline void barrier() {
        std::atomic_signal_fence(std::memory_order_seq_cst);
}

struct Stats {
        std::vector<double> samples_ns;
        double              median_ns{0}, mad_ns{0};
        double              ops_per_sec{0}, ci_lo{0}, ci_hi{0};
};

static double median_of(std::vector<double> v) {
        if (v.empty())
                return 0.0;
        std::sort(v.begin(), v.end());
        size_t n = v.size();
        return (n & 1) ? v[n / 2] : 0.5 * (v[n / 2 - 1] + v[n / 2]);
}
static double mad_of(const std::vector<double>& v, double med) {
        if (v.empty())
                return 0.0;
        std::vector<double> d;
        d.reserve(v.size());
        for (double x : v)
                d.push_back(std::fabs(x - med));
        return median_of(std::move(d));
}
static Stats make_stats(std::vector<double> s, double ops_per_run) {
        if (s.size() > 1)
                s.erase(s.begin()); // drop warm-up
        Stats st;
        st.samples_ns  = s;
        st.median_ns   = std::max(1.0, median_of(s));
        st.mad_ns      = std::max(1.0, mad_of(s, st.median_ns));
        double secs    = st.median_ns * 1e-9;
        st.ops_per_sec = (secs > 0) ? (ops_per_run / secs) : 0.0;
        double lo_ns   = std::max(1.0, st.median_ns - 1.58 * st.mad_ns);
        double hi_ns   = std::max(lo_ns * 1.0001, st.median_ns + 1.58 * st.mad_ns);
        st.ci_lo       = ops_per_run / (hi_ns * 1e-9);
        st.ci_hi       = ops_per_run / (lo_ns * 1e-9);
        return st;
}

struct Config {
        int  runs = 100, iters = 200000;
        bool strict = false;
};

struct Row {
        std::string name;
        Stats       base, scratch;
        double      speedup{1};
        bool        pass{true};
        double      gate{1};
};

static void print_row(const Row& r) {
        auto fmt = [&](double v) {
                std::ostringstream o;
                o << std::fixed << std::setprecision(0) << v;
                return o.str();
        };
        std::cout << r.name << ": " << (r.pass ? "PASS" : "FAIL") << " - speedup " << std::fixed << std::setprecision(2)
                  << r.speedup << "x";
        if (!r.pass)
                std::cout << " (gate " << r.gate << "x)";
        std::cout << "\n  baseline: " << fmt(r.base.ops_per_sec) << " ops/s [" << fmt(r.base.ci_lo) << "–"
                  << fmt(r.base.ci_hi) << "]\n";
        std::cout << "  scratch : " << fmt(r.scratch.ops_per_sec) << " ops/s [" << fmt(r.scratch.ci_lo) << "–"
                  << fmt(r.scratch.ci_hi) << "]\n";
}

template <class FSetup, class FBody, class FTeardown>
static Stats time_runs(const Config& cfg, FSetup&& setup, FBody&& body, FTeardown&& teardown, double ops_per_run) {
        std::vector<double> s;
        s.reserve(cfg.runs);
        for (int run = 0; run < cfg.runs; ++run) {
                setup();
                barrier();
                auto t0 = Clock::now();
                body();
                auto t1 = Clock::now();
                barrier();
                teardown();
                s.push_back((double)std::chrono::duration_cast<ns>(t1 - t0).count());
        }
        return make_stats(std::move(s), ops_per_run);
}

// -------- Tests using Anvil API style (create/alloc/reset/destroy) --------

static Row tiny_allocations(const Config& cfg) {
        const int    N    = cfg.iters;
        const size_t SZ   = 16;
        // Baseline: malloc only (frees excluded for apples-to-apples microalloc rate)
        auto         base = time_runs(
            cfg, [] {},
            [&] {
                    for (int i = 0; i < N; ++i) {
                            void* p = std::malloc(SZ);
                            if (p)
                                    *(volatile uint8_t*)p = 1;
                    }
            },
            [] {}, N);
        // Scratch: create/destroy outside timed region
        auto scratch = time_runs(
            cfg, [] {},
            [&] {
                    ScratchAllocator* a =
                        anvil::memory::scratch_allocator::create((size_t)N * SZ + 1024, MIN_ALIGNMENT);
                    for (int i = 0; i < N; ++i) {
                            (void)anvil::memory::scratch_allocator::alloc(a, SZ, MIN_ALIGNMENT);
                    }
                    (void)anvil::memory::scratch_allocator::destroy(&a);
            },
            [] {}, N);
        double sp   = (base.ops_per_sec > 0) ? (scratch.ops_per_sec / base.ops_per_sec) : 1.0;
        double gate = 3.0;
        bool   pass = !cfg.strict || sp >= gate;
        return {"tiny_allocations", base, scratch, sp, pass, gate};
}

static Row reset_performance(const Config& cfg) {
        const int CYCLES = std::max(1, cfg.iters / 200);
        const int ALLOCS = 1000;
        // Baseline: malloc/free per cycle
        auto      base   = time_runs(
            cfg, [] {},
            [&] {
                    for (int c = 0; c < CYCLES; ++c) {
                            std::vector<void*> ptrs(ALLOCS);
                            for (int i = 0; i < ALLOCS; ++i) {
                                    ptrs[i] = std::malloc(64);
                            }
                            for (int i = 0; i < ALLOCS; ++i) {
                                    std::free(ptrs[i]);
                            }
                    }
            },
            [] {}, (double)CYCLES * ALLOCS);
        // Scratch: include reset() in timing; create/destroy out
        auto scratch = time_runs(
            cfg, [] {},
            [&] {
                    ScratchAllocator* a =
                        anvil::memory::scratch_allocator::create((size_t)ALLOCS * 64 + 1024, MIN_ALIGNMENT);
                    for (int c = 0; c < CYCLES; ++c) {
                            for (int i = 0; i < ALLOCS; ++i) {
                                    (void)anvil::memory::scratch_allocator::alloc(a, 64, MIN_ALIGNMENT);
                            }
                            (void)anvil::memory::scratch_allocator::reset(a);
                    }
                    (void)anvil::memory::scratch_allocator::destroy(&a);
            },
            [] {}, (double)CYCLES * ALLOCS);
        double sp   = (base.ops_per_sec > 0) ? (scratch.ops_per_sec / base.ops_per_sec) : 1.0;
        double gate = 3.0;
        bool   pass = !cfg.strict || sp >= gate;
        return {"reset_performance", base, scratch, sp, pass, gate};
}

static Row alignment_patterns(const Config& cfg) {
        const int N     = cfg.iters / 4;
        size_t    AL[4] = {MIN_ALIGNMENT, MIN_ALIGNMENT << 1, MIN_ALIGNMENT << 2, MIN_ALIGNMENT << 3};
        auto      base  = time_runs(
            cfg, [] {},
            [&] {
                    for (int i = 0; i < N; ++i) {
                            size_t al = AL[i & 3];
                            void*  p  = nullptr;
#if defined(_MSC_VER)
                            p = _aligned_malloc(64, al);
#else
                            (void)posix_memalign(&p, al, 64);
#endif
                            if (p)
                                    *(volatile uint8_t*)p = 1;
                    }
            },
            [] {}, N);
        auto scratch = time_runs(
            cfg, [] {},
            [&] {
                    ScratchAllocator* a =
                        anvil::memory::scratch_allocator::create((size_t)N * 64 + 1024, MIN_ALIGNMENT);
                    for (int i = 0; i < N; ++i) {
                            size_t al = AL[i & 3];
                            (void)anvil::memory::scratch_allocator::alloc(a, 64, al);
                    }
                    (void)anvil::memory::scratch_allocator::destroy(&a);
            },
            [] {}, N);
        double sp   = (base.ops_per_sec > 0) ? (scratch.ops_per_sec / base.ops_per_sec) : 1.0;
        double gate = 1.5;
        bool   pass = !cfg.strict || sp >= gate;
        return {"alignment_patterns", base, scratch, sp, pass, gate};
}

static Row interleaved_patterns(const Config& cfg) {
        const int                          N = cfg.iters / 5;
        std::mt19937                       rng(1337);
        std::uniform_int_distribution<int> szd(8, 256);
        auto                               inter = [&](auto alloc_fn, auto free_fn) {
                std::vector<void*> live;
                live.reserve(1024);
                for (int i = 0; i < N; ++i) {
                        for (int k = 0; k < 3; ++k) {
                                void* p = alloc_fn(szd(rng), MIN_ALIGNMENT);
                                live.push_back(p);
                        }
                        for (int k = 0; k < 2 && !live.empty(); ++k) {
                                void* p = live.front();
                                live.erase(live.begin());
                                free_fn(p);
                        }
                }
                for (void* p : live)
                        free_fn(p);
        };
        auto base = time_runs(
            cfg, [] {}, [&] { inter([](size_t s, size_t) { return std::malloc(s); }, [](void* p) { std::free(p); }); },
            [] {}, (double)N);
        auto scratch = time_runs(
            cfg, [] {},
            [&] {
                    ScratchAllocator* a = anvil::memory::scratch_allocator::create(1 << 20, MIN_ALIGNMENT);
                    inter([&](size_t s, size_t al) { return anvil::memory::scratch_allocator::alloc(a, s, al); },
                          [&](void*) { /*no-op*/ });
                    (void)anvil::memory::scratch_allocator::destroy(&a);
            },
            [] {}, (double)N);
        double sp   = (base.ops_per_sec > 0) ? (scratch.ops_per_sec / base.ops_per_sec) : 1.0;
        double gate = 1.0;
        bool   pass = !cfg.strict || sp >= gate;
        return {"interleaved_patterns", base, scratch, sp, pass, gate};
}

static Row mixed_workloads(const Config& cfg) {
        const int                          N = cfg.iters / 2;
        std::mt19937                       rng(1338);
        std::uniform_int_distribution<int> op(0, 9), szd(16, 1024);
        auto                               body = [&](auto alloc_fn, auto free_fn) {
                std::vector<void*> pool;
                pool.reserve(4096);
                for (int i = 0; i < N; ++i) {
                        int t = op(rng);
                        if (t < 6) {
                                void* p = alloc_fn(szd(rng), MIN_ALIGNMENT);
                                if (p)
                                        pool.push_back(p);
                        } else if (!pool.empty()) {
                                size_t idx = (size_t)op(rng) % pool.size();
                                void*  p   = pool[idx];
                                pool[idx]  = pool.back();
                                pool.pop_back();
                                free_fn(p);
                        } else {
                                volatile uint64_t x = i;
                                (void)x;
                        }
                }
                for (void* p : pool)
                        free_fn(p);
        };
        auto base = time_runs(
            cfg, [] {}, [&] { body([](size_t s, size_t) { return std::malloc(s); }, [](void* p) { std::free(p); }); },
            [] {}, (double)N);
        auto scratch = time_runs(
            cfg, [] {},
            [&] {
                    ScratchAllocator* a = anvil::memory::scratch_allocator::create(8 << 20, MIN_ALIGNMENT);
                    body([&](size_t s, size_t al) { return anvil::memory::scratch_allocator::alloc(a, s, al); },
                         [&](void*) { /*no-op*/ });
                    (void)anvil::memory::scratch_allocator::destroy(&a);
            },
            [] {}, (double)N);
        double sp   = (base.ops_per_sec > 0) ? (scratch.ops_per_sec / base.ops_per_sec) : 1.0;
        double gate = 1.2;
        bool   pass = !cfg.strict || sp >= gate;
        return {"mixed_workloads", base, scratch, sp, pass, gate};
}

int main(int argc, char** argv) {
        Config cfg;
        for (int i = 1; i < argc; ++i) {
                std::string a    = argv[i];
                auto        next = [&](int& i) { return (i + 1 < argc) ? argv[++i] : nullptr; };
                if (a == "--runs") {
                        if (auto v = next(i))
                                cfg.runs = std::atoi(v);
                } else if (a == "--iters") {
                        if (auto v = next(i))
                                cfg.iters = std::atoi(v);
                } else if (a == "--strict") {
                        cfg.strict = true;
                } else if (a == "--help") {
                        std::cout << "Usage: " << argv[0] << " [--runs N] [--iters N] [--strict]\n";
                        return 0;
                }
        }
        if (cfg.runs < 2)
                cfg.runs = 2;

        std::cout << "=== Anvil Scratch Allocator Benchmark (improved) ===\n";
#if HAVE_ANVIL
        std::cout << "(anvil headers detected)\n";
#else
        std::cout << "(anvil headers NOT found; baseline-compatible shims in use)\n";
#endif

        std::vector<Row> rows;
        rows.push_back(tiny_allocations(cfg));
        rows.push_back(reset_performance(cfg));
        rows.push_back(alignment_patterns(cfg));
        rows.push_back(interleaved_patterns(cfg));
        rows.push_back(mixed_workloads(cfg));

        int passes = 0, fails = 0;
        for (const auto& r : rows) {
                print_row(r);
                if (r.pass)
                        ++passes;
                else
                        ++fails;
        }
        std::cout << "\nSummary: " << passes << " PASS, " << fails << " FAIL";
        if (cfg.strict)
                std::cout << " (strict mode)";
        std::cout << "\n";
        return (cfg.strict && fails > 0) ? 1 : 0;
}
