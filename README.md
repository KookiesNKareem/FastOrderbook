# High-Performance Limit Order Book

A C++ implementation of a limit-order book with **sub-microsecond** latency for all operations. 

## Performance Summary

| Operation | Latency | Throughput |
|-----------|---------|------------|
| Add Order | 0.28 μs | 3617160 orders/s |
| Get Quote | 70.81 ns | 14122997 quotes/s |
| Cancel Order| 0.16 μs | 6285355 orders/s |
| Modify Order | 0.08 μs | 12953368 orders/s |
| Match Orders | 

*Benchmarked on M4 Mac (ARM), compiled with -O3*

## Key Features

- **FIFO price-time priority** - orders execute in their placed order
- **Efficient matching engine** - handles multiple partial executions at different price levels
- **O(1) best bid/ask** - instant quotes
- **Sub-microsecond operations** - suitable for low-latency trading

## Architecture

See [DESIGN.md](DESIGN.md) for detailed architecture decisions.

## Performance Details

See [PERFORMANCE.md](PERFORMANCE.md) for optimization journey and benchmarks.
