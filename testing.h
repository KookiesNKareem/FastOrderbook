#ifndef TESTING_H
#define TESTING_H

#include <iostream>
#include <chrono>
#include <random>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <numeric>
#include <cmath>
#include "Orderbook.h"

using namespace std;
using namespace std::chrono;

struct BenchmarkStats {
    double mean;
    double std_dev;
};

BenchmarkStats calculate_stats(const vector<double>& values) {
    BenchmarkStats stats;

    stats.mean = accumulate(values.begin(), values.end(), 0.0) / values.size();

    double sq_sum = 0.0;
    for (double v : values) {
        sq_sum += (v - stats.mean) * (v - stats.mean);
    }
    stats.std_dev = sqrt(sq_sum / values.size());

    return stats;
}

void print_stats(const string& metric_name, const BenchmarkStats& stats, const string& unit) {
    cout << "  " << metric_name << ": " << fixed << setprecision(2)
         << stats.mean << " " << unit << " (std: " << stats.std_dev << ")" << endl;
}

bool check_quote(const Quote& q, uint32_t exp_bid_price, uint32_t exp_bid_qty,
                 uint32_t exp_ask_price, uint32_t exp_ask_qty, const string& test_name) {
    bool passed = (q.bid_price == exp_bid_price && q.bid_quantity == exp_bid_qty &&
                   q.ask_price == exp_ask_price && q.ask_quantity == exp_ask_qty);

    cout << "  Actual:   Bid=$" << q.bid_price << "(" << q.bid_quantity << "), "
         << "Ask=$" << q.ask_price << "(" << q.ask_quantity << ")" << endl;
    cout << "  Expected: Bid=$" << exp_bid_price << "(" << exp_bid_qty << "), "
         << "Ask=$" << exp_ask_price << "(" << exp_ask_qty << ")" << endl;

    if (passed) {
        cout << "  ✓ PASS" << endl << endl;
    } else {
        cout << "  ✗ FAIL" << endl << endl;
    }
    return passed;
}

void benchmark_add_orders(int num_orders, int num_runs = 10) {
    vector<double> total_times;
    vector<double> avg_per_add;
    vector<double> adds_per_sec;

    for (int run = 0; run < num_runs; run++) {
        clear_orderbook();
        mt19937 gen(42);
        uniform_int_distribution<> price_dist(9000, 11000);
        uniform_int_distribution<> qty_dist(1, 100);
        uniform_int_distribution<> side_dist(0, 1);

        auto start = high_resolution_clock::now();

        for (int i = 0; i < num_orders; i++) {
            uint64_t order_id = i + 1;
            Side side = side_dist(gen) == 0 ? Side::BUY : Side::SELL;
            uint32_t price = price_dist(gen);
            uint32_t quantity = qty_dist(gen);
            add_order(order_id, side, price, quantity);
        }

        auto end = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(end - start);

        total_times.push_back(duration.count());
        avg_per_add.push_back((double)duration.count() / num_orders);
        adds_per_sec.push_back((num_orders * 1000000.0) / duration.count());
    }

    cout << "Add Orders Benchmark (" << num_orders << " orders, " << num_runs << " runs):" << endl;
    print_stats("Total time", calculate_stats(total_times), "μs");
    print_stats("Avg per add", calculate_stats(avg_per_add), "μs");
    print_stats("Adds/sec", calculate_stats(adds_per_sec), "ops");
    cout << endl;
}

void benchmark_get_quote(int num_iterations, int num_runs = 10) {
    vector<double> total_times;
    vector<double> avg_per_quote;
    vector<double> quotes_per_sec;

    for (int run = 0; run < num_runs; run++) {
        clear_orderbook();

        // Setup orderbook with some orders
        for (int i = 0; i < 1000; i++) {
            add_order(i + 1, Side::BUY, 10000 - i, 100);
            add_order(i + 1001, Side::SELL, 10001 + i, 100);
        }

        auto start = high_resolution_clock::now();

        Quote q(0, 0, 0, 0);
        for (int i = 0; i < num_iterations; i++) {
            q = get_quote();
            volatile uint32_t prevent_opt = q.bid_price;
        }

        auto end = high_resolution_clock::now();
        auto duration = duration_cast<nanoseconds>(end - start);

        total_times.push_back(duration.count());
        avg_per_quote.push_back((double)duration.count() / num_iterations);
        quotes_per_sec.push_back((num_iterations * 1000000000.0) / duration.count());
    }

    cout << "Get Quote Benchmark (" << num_iterations << " iterations, " << num_runs << " runs):" << endl;
    print_stats("Total time", calculate_stats(total_times), "ns");
    print_stats("Avg per quote", calculate_stats(avg_per_quote), "ns");
    print_stats("Quotes/sec", calculate_stats(quotes_per_sec), "ops");
    cout << endl;
}

void benchmark_cancel_orders(int num_orders, int num_runs = 10) {
    vector<double> total_times;
    vector<double> avg_per_cancel;
    vector<double> cancels_per_sec;

    for (int run = 0; run < num_runs; run++) {
        clear_orderbook();

        // Add orders first
        for (int i = 0; i < num_orders; i++) {
            add_order(i + 1, Side::BUY, 10000, 100);
        }

        auto start = high_resolution_clock::now();

        for (int i = 0; i < num_orders; i++) {
            cancel_order(i + 1);
        }

        auto end = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(end - start);

        total_times.push_back(duration.count());
        avg_per_cancel.push_back((double)duration.count() / num_orders);
        cancels_per_sec.push_back((num_orders * 1000000.0) / duration.count());
    }

    cout << "Cancel Orders Benchmark (" << num_orders << " orders, " << num_runs << " runs):" << endl;
    print_stats("Total time", calculate_stats(total_times), "μs");
    print_stats("Avg per cancel", calculate_stats(avg_per_cancel), "μs");
    print_stats("Cancels/sec", calculate_stats(cancels_per_sec), "ops");
    cout << endl;
}

void benchmark_modify_orders(int num_orders, int num_runs = 10) {
    vector<double> total_times;
    vector<double> avg_per_modify;
    vector<double> modifies_per_sec;

    for (int run = 0; run < num_runs; run++) {
        clear_orderbook();
        mt19937 gen(42 + run);
        uniform_int_distribution<> qty_dist(1, 100);

        // Add orders first
        for (int i = 0; i < num_orders; i++) {
            add_order(i + 1, Side::BUY, 10000, 100);
        }

        auto start = high_resolution_clock::now();

        for (int i = 0; i < num_orders; i++) {
            modify_order(i + 1, qty_dist(gen));
        }

        auto end = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(end - start);

        total_times.push_back(duration.count());
        avg_per_modify.push_back((double)duration.count() / num_orders);
        modifies_per_sec.push_back((num_orders * 1000000.0) / duration.count());
    }

    cout << "Modify Orders Benchmark (" << num_orders << " orders, " << num_runs << " runs):" << endl;
    print_stats("Total time", calculate_stats(total_times), "μs");
    print_stats("Avg per modify", calculate_stats(avg_per_modify), "μs");
    print_stats("Modifies/sec", calculate_stats(modifies_per_sec), "ops");
    cout << endl;
}

void benchmark_order_matching(int num_orders, int num_runs = 10) {
    vector<double> total_times;
    vector<double> avg_per_match;
    vector<double> matches_per_sec;

    for (int run = 0; run < num_runs; run++) {
        clear_orderbook();

        // Add resting sell orders
        for (int i = 0; i < num_orders; i++) {
            add_order(i + 1, Side::SELL, 10000 + i, 100);
        }

        auto start = high_resolution_clock::now();

        // Add aggressive buy orders that will match
        for (int i = 0; i < num_orders; i++) {
            add_order(i + num_orders + 1, Side::BUY, 10000 + i, 100);
        }

        auto end = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(end - start);

        total_times.push_back(duration.count());
        avg_per_match.push_back((double)duration.count() / num_orders);
        matches_per_sec.push_back((num_orders * 1000000.0) / duration.count());
    }

    cout << "Order Matching Benchmark (" << num_orders << " matches, " << num_runs << " runs):" << endl;
    print_stats("Total time", calculate_stats(total_times), "μs");
    print_stats("Avg per match", calculate_stats(avg_per_match), "μs");
    print_stats("Matches/sec", calculate_stats(matches_per_sec), "ops");
    cout << endl;
}

void benchmark_mixed_workload(int num_operations, int num_runs = 10) {
    vector<double> total_times;
    vector<double> avg_per_op;
    vector<double> ops_per_sec;

    for (int run = 0; run < num_runs; run++) {
        clear_orderbook();
        mt19937 gen(42 + run);
        uniform_int_distribution<> op_dist(0, 3);  // 0=add, 1=cancel, 2=modify, 3=quote
        uniform_int_distribution<> price_dist(9900, 10100);
        uniform_int_distribution<> qty_dist(1, 100);
        uniform_int_distribution<> side_dist(0, 1);

        uint64_t next_order_id = 1;
        vector<uint64_t> active_orders;

        // Pre-populate with some orders
        for (int i = 0; i < 100; i++) {
            add_order(next_order_id, Side::BUY, 9950 - i, 100);
            active_orders.push_back(next_order_id++);
            add_order(next_order_id, Side::SELL, 10050 + i, 100);
            active_orders.push_back(next_order_id++);
        }

        auto start = high_resolution_clock::now();

        for (int i = 0; i < num_operations; i++) {
            int op = op_dist(gen);

            if (op == 0) {  // Add order
                Side side = side_dist(gen) == 0 ? Side::BUY : Side::SELL;
                uint32_t price = price_dist(gen);
                uint32_t quantity = qty_dist(gen);
                add_order(next_order_id, side, price, quantity);
                active_orders.push_back(next_order_id++);
            } else if (op == 1 && !active_orders.empty()) {  // Cancel order
                uniform_int_distribution<> order_dist(0, active_orders.size() - 1);
                int idx = order_dist(gen);
                cancel_order(active_orders[idx]);
                active_orders.erase(active_orders.begin() + idx);
            } else if (op == 2 && !active_orders.empty()) {  // Modify order
                uniform_int_distribution<> order_dist(0, active_orders.size() - 1);
                int idx = order_dist(gen);
                modify_order(active_orders[idx], qty_dist(gen));
            } else {  // Get quote
                Quote q = get_quote();
                volatile uint32_t prevent_opt = q.bid_price;
            }
        }

        auto end = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(end - start);

        total_times.push_back(duration.count());
        avg_per_op.push_back((double)duration.count() / num_operations);
        ops_per_sec.push_back((num_operations * 1000000.0) / duration.count());
    }

    cout << "Mixed Workload Benchmark (" << num_operations << " operations, " << num_runs << " runs):" << endl;
    print_stats("Total time", calculate_stats(total_times), "μs");
    print_stats("Avg per op", calculate_stats(avg_per_op), "μs");
    print_stats("Operations/sec", calculate_stats(ops_per_sec), "ops");
    cout << endl;
}

#endif // TESTING_H