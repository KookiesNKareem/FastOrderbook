#include "testing.h"
#include <iostream>

using namespace std;

int main()
{
    cout << "=== Orderbook Performance Benchmarks ===" << endl << endl;

    benchmark_add_orders(100000);
    benchmark_get_quote(1000000);
    benchmark_cancel_orders(10000);
    benchmark_modify_orders(10000);
    benchmark_order_matching(5000);
    benchmark_mixed_workload(50000);

    cout << "=== Benchmarks Complete ===" << endl;

    return 0;
}
