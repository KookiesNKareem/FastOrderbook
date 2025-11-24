# High-Performance Limit Order Book

A C++ implementation of a limit-order book with **sub-microsecond** latency for all operations. 

## Performance Summary

| Operation | Latency | Throughput |
|-----------|---------|------------|
| Add Order | 0.87 μs | 1,152,724 orders/s |
| Get Quote | 5.58 ns | 179,173,997 quotes/s |
| Cancel Order| 0.31 μs | 3,181,674 orders/s |
| Modify Order | 0.08 μs | 12,953,368 orders/s |
| Match Orders | 0.63 μs | 1,582,278 fills/s |
---------------------------------------------

*Benchmarked on M4 Mac (ARM), compiled with -O3*

## Key Features

- **FIFO price-time priority** - orders execute in their placed order
- **Efficient matching engine** - handles multiple partial executions at different price levels
- **O(1) best bid/ask** - instant quotes
- **Sub-microsecond operations** - suitable for low-latency trading

## Performance Details

See [PERFORMANCE.md](PERFORMANCE.md) for optimization journey and benchmarks.
