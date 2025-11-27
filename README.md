# High-Performance Limit Order Book

A C++ implementation of a limit-order book with **sub-microsecond** latency for all operations. 

## Performance Summary

| Operation | Latency | Throughput |
|-----------|---------|------------|
| Add Order | 0.16 μs | 6,135,722 orders/s |
| Get Quote | 0.30 ns | 3,299,426,230 quotes/s |
| Cancel Order| 0.02 μs | 47,393,365 orders/s |
| Modify Order | 0.01 μs | 108,695,652 orders/s |
| Match Orders | 0.10 μs | 9,823,183 fills/s |
---------------------------------------------

*Benchmarked on M4 Mac (ARM), compiled with -O3*

## Key Features

- **FIFO price-time priority** - orders execute in their placed order
- **Efficient matching engine** - handles multiple partial executions at different price levels
- **O(1) best bid/ask** - instant quotes
- **Sub-microsecond operations** - suitable for low-latency trading

## Performance Details

See [Performance.md](Performance.md) for optimization journey and benchmarks.
