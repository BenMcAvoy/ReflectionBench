#include <chrono>
#include <atomic>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>
#include <cstdio>

#include <rttr/registration>
#include <entt/entt.hpp>

using namespace std::chrono;

struct Payload {
    int a{1};
    double b{2.0};
    std::string s{"hello"};

    int mul(int x) const noexcept { return a * x; }
    void add_to_b(double x) noexcept { b += x; }
};

// --- RTTR registration ---
RTTR_REGISTRATION
{
    using namespace rttr;
    registration::class_<Payload>("Payload")
        .property("a", &Payload::a)
        .property("b", &Payload::b)
        .property("s", &Payload::s)
        .method("mul", &Payload::mul)
        .method("add_to_b", &Payload::add_to_b);
}

// --- EnTT meta registration ---
namespace meta_ids {
    // Numeric ids avoid string hashing in the hot path
    inline constexpr entt::id_type cls = entt::hashed_string{"Payload"}.value();
    inline constexpr entt::id_type a   = entt::hashed_string{"a"}.value();
    inline constexpr entt::id_type b   = entt::hashed_string{"b"}.value();
    inline constexpr entt::id_type s   = entt::hashed_string{"s"}.value();
    inline constexpr entt::id_type mul = entt::hashed_string{"mul"}.value();
    inline constexpr entt::id_type add = entt::hashed_string{"add_to_b"}.value();
}

static void register_entt_meta() {
    entt::meta_factory<Payload>()
        .type(meta_ids::cls)
        .data<&Payload::a>(meta_ids::a)
        .data<&Payload::b>(meta_ids::b)
        .data<&Payload::s>(meta_ids::s)
        .func<&Payload::mul>(meta_ids::mul)
        .func<&Payload::add_to_b>(meta_ids::add);
}

static inline void pin(const void* p) {
    // Prevent some forms of reordering/DCE around benchmark loops
    std::atomic_signal_fence(std::memory_order_seq_cst);
    (void)p;
}

// Calibrate loop count to ~50-150ms
std::uint64_t calibrate(auto&& fn) {
    std::uint64_t n = 1;
    while(true) {
        auto t0 = high_resolution_clock::now();
        double sink = 0.0;
        for(std::uint64_t i = 0; i < n; ++i) sink += fn();
        auto t1 = high_resolution_clock::now();
        auto ms = duration<double, std::milli>(t1 - t0).count();
        if(ms > 60.0) return n; // ~target
        n *= 2;
        if(n > (1ull<<32)) return n;
        pin(&sink);
    }
}

struct BenchResult {
    double ns_per_op;
    std::uint64_t iters;
    double checksum; // to defeat DCE
};

BenchResult run_bench(auto&& fn, std::uint64_t iters) {
    auto t0 = high_resolution_clock::now();
    double sink = 0.0;
    for(std::uint64_t i = 0; i < iters; ++i) sink += fn();
    auto t1 = high_resolution_clock::now();
    auto ns = duration<double, std::nano>(t1 - t0).count();
    return BenchResult{ns / static_cast<double>(iters), iters, sink};
}

struct DuoResult {
    const char* op;
    BenchResult rttr;
    BenchResult entt;
};

// Removed previous console pretty-print helpers; JSON output only.

int main() {
    // Ensure EnTT meta is registered
    register_entt_meta();

    Payload obj{};

    // Names used for cold lookups to ensure runtime costs
    std::string nm_a = "a";
    std::string nm_b = "b";
    std::string nm_mul = "mul";
    std::string nm_add = "add_to_b";

    const rttr::type t = rttr::type::get<Payload>();
    const auto prop_a = t.get_property("a");
    const auto prop_b = t.get_property("b");
    const auto meth_mul = t.get_method("mul");
    const auto meth_add = t.get_method("add_to_b");

    const entt::meta_type mt = entt::resolve<Payload>();
    const entt::meta_data md_a = mt.data(meta_ids::a);
    const entt::meta_data md_b = mt.data(meta_ids::b);
    const entt::meta_func mf_mul = mt.func(meta_ids::mul);
    const entt::meta_func mf_add = mt.func(meta_ids::add);

    if(!prop_a || !prop_b || !meth_mul || !meth_add) {
        std::cerr << "RTTR registration failed" << std::endl;
        return 1;
    }
    if(!md_a || !md_b || !mf_mul || !mf_add) {
        std::cerr << "EnTT meta registration failed" << std::endl;
        return 1;
    }

    // Hot paths below:

    // Field get (int a)
    auto rttr_get_a = [&]() -> double {
        auto var = prop_a.get_value(obj);
        return var.to_int();
    };
    auto entt_get_a = [&]() -> double {
        entt::meta_any any = md_a.get(obj);
        return any.cast<int>();
    };

    // Field set (double b)
    auto rttr_set_b = [&]() -> double {
        double v = 1.0;
        prop_b.set_value(obj, v);
        return obj.b;
    };
    auto entt_set_b = [&]() -> double {
        double v = 1.0;
        md_b.set(obj, v);
        return obj.b;
    };

    // Method call: int mul(int)
    auto rttr_call_mul = [&]() -> double {
        auto ret = meth_mul.invoke(obj, 3);
        return ret.to_int();
    };
    auto entt_call_mul = [&]() -> double {
        entt::meta_any ret = mf_mul.invoke(obj, 3);
        return ret.cast<int>();
    };

    // Method call: void add_to_b(double)
    auto rttr_call_add = [&]() -> double {
        meth_add.invoke(obj, 0.5);
        return obj.b;
    };
    auto entt_call_add = [&]() -> double {
        mf_add.invoke(obj, 0.5);
        return obj.b;
    };

    // Cold paths below

    auto rttr_cold_hash_get_a = [&]() -> double {
        auto p = t.get_property(nm_a);
        auto var = p.get_value(obj);
        return var.to_int();
    };
    auto entt_cold_hash_get_a = [&]() -> double {
        auto md = mt.data(entt::hashed_string{nm_a.c_str()}.value());
        entt::meta_any any = md.get(obj);
        return any.cast<int>();
    };

    auto rttr_cold_hash_set_b = [&]() -> double {
        double v = 1.0;
        auto p = t.get_property(nm_b);
        p.set_value(obj, v);
        return obj.b;
    };
    auto entt_cold_hash_set_b = [&]() -> double {
        double v = 1.0;
        auto md = mt.data(entt::hashed_string{nm_b.c_str()}.value());
        md.set(obj, v);
        return obj.b;
    };

    auto rttr_cold_hash_call_mul = [&]() -> double {
        auto m = t.get_method(nm_mul);
        auto ret = m.invoke(obj, 3);
        return ret.to_int();
    };
    auto entt_cold_hash_call_mul = [&]() -> double {
        auto mf = mt.func(entt::hashed_string{nm_mul.c_str()}.value());
        entt::meta_any ret = mf.invoke(obj, 3);
        return ret.cast<int>();
    };

    auto rttr_cold_hash_call_add = [&]() -> double {
        auto m = t.get_method(nm_add);
        m.invoke(obj, 0.5);
        return obj.b;
    };
    auto entt_cold_hash_call_add = [&]() -> double {
        auto mf = mt.func(entt::hashed_string{nm_add.c_str()}.value());
        mf.invoke(obj, 0.5);
        return obj.b;
    };

    // Calibrate to get comparable runtimes (per op family, reuse for both libs)
    // we do this because it means that our CPU is warmed up to what it will see in the benchmark
    const std::uint64_t it_get = calibrate(rttr_get_a);
    const std::uint64_t it_set = calibrate(rttr_set_b);
    const std::uint64_t it_mul = calibrate(rttr_call_mul);
    const std::uint64_t it_add = calibrate(rttr_call_add);
    const std::uint64_t it_hget = calibrate(rttr_cold_hash_get_a);
    const std::uint64_t it_hset = calibrate(rttr_cold_hash_set_b);
    const std::uint64_t it_hmul = calibrate(rttr_cold_hash_call_mul);
    const std::uint64_t it_hadd = calibrate(rttr_cold_hash_call_add);
    std::vector<DuoResult> hot, cold_hash; // result pairs
    hot.push_back({"get a", run_bench(rttr_get_a, it_get), run_bench(entt_get_a, it_get)});
    hot.push_back({"set b", run_bench(rttr_set_b, it_set), run_bench(entt_set_b, it_set)});
    hot.push_back({"mul(3)", run_bench(rttr_call_mul, it_mul), run_bench(entt_call_mul, it_mul)});
    hot.push_back({"add_to_b(0.5)", run_bench(rttr_call_add, it_add), run_bench(entt_call_add, it_add)});

    cold_hash.push_back({"get a", run_bench(rttr_cold_hash_get_a, it_hget), run_bench(entt_cold_hash_get_a, it_hget)});
    cold_hash.push_back({"set b", run_bench(rttr_cold_hash_set_b, it_hset), run_bench(entt_cold_hash_set_b, it_hset)});
    cold_hash.push_back({"mul(3)", run_bench(rttr_cold_hash_call_mul, it_hmul), run_bench(entt_cold_hash_call_mul, it_hmul)});
    cold_hash.push_back({"add_to_b(0.5)", run_bench(rttr_cold_hash_call_add, it_hadd), run_bench(entt_cold_hash_call_add, it_hadd)});

    // dump json for py script
    auto json_escape = [](const std::string &in) {
        std::string out;
        out.reserve(in.size() + 8);
        for(char c : in) {
            switch(c) {
                case '\\': out += "\\\\"; break;
                case '"':  out += "\\\""; break;
                case '\b': out += "\\b"; break;
                case '\f': out += "\\f"; break;
                case '\n': out += "\\n"; break;
                case '\r': out += "\\r"; break;
                case '\t': out += "\\t"; break;
                default:
                    if(static_cast<unsigned char>(c) < 0x20) {
                        char buf[7];
                        std::snprintf(buf, sizeof(buf), "\\u%04x", (unsigned)c);
                        out += buf;
                    } else {
                        out += c;
                    }
            }
        }
        return out;
    };

    auto print_bench_obj = [&](const BenchResult &br) {
        std::cout.setf(std::ios::fixed); std::cout << std::setprecision(6);
        std::cout << "{ \"ns_per_op\": " << br.ns_per_op << " }";
        std::cout.unsetf(std::ios::floatfield);
    };

    auto print_group = [&](const char* gname, const std::vector<DuoResult>& grp) {
        std::cout << "    {\n";
        std::cout << "      \"name\": \"" << json_escape(gname) << "\",\n";
        std::cout << "      \"rows\": [\n";
        for(size_t i = 0; i < grp.size(); ++i) {
            const auto &d = grp[i];
            std::cout << "        {\n";
            std::cout << "          \"operation\": \"" << json_escape(d.op ? std::string(d.op) : std::string("")) << "\",\n";
            std::cout << "          \"rttr\": "; print_bench_obj(d.rttr); std::cout << ",\n";
            std::cout << "          \"entt\": "; print_bench_obj(d.entt); std::cout << "\n";
            std::cout << "        }" << (i + 1 < grp.size() ? "," : "") << "\n";
        }
        std::cout << "      ]\n";
        std::cout << "    }";
    };

    std::cout << "{\n";
    std::cout << "  \"groups\": [\n";
    print_group("HOT (handles cached; no lookup/hash in loop)", hot); std::cout << ",\n";
    print_group("COLD (native per-iter lookup)", cold_hash); std::cout << "\n";
    std::cout << "  ]\n";
    std::cout << "}\n";

    return 0;
}
