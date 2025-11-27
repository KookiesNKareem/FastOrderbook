# High-Performance Limit Order Book

A C++ implementation of a limit-order book with **sub-microsecond** latency for all operations. 

## Performance Summary

| Operation | Latency | STD (n=10 runs) | Throughput |
|-----------|---------|------------|-----|
| Add Order | 0.12 μs | 0.02 μs | 8,315,591 orders/s |
| Get Quote | 0.24 ns | 0.01 ns | 4,187,266,748 quotes/s |
| Cancel Order| 0.02 μs | 0.00 μs | 47,393,365 orders/s |
| Modify Order | 0.01 μs | 0.00 μs | 187,167,197 orders/s |
| Match Orders | 0.08 μs | 0.02 μs | 13,514,155 fills/s |
---------------------------------------------

*Benchmarked on M4 Mac (ARM), compiled with -O3*

## Key Features

- **FIFO price-time priority** - orders execute in their placed order
- **Efficient matching engine** - handles multiple partial executions at different price levels
- **O(1) best bid/ask** - instant quotes
- **Sub-microsecond operations** - suitable for low-latency trading

## Performance Details

See [Performance.md](Performance.md) for optimization journey and benchmarks.
