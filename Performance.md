[11/23/2025]

Replaced hashmap with pre-allocated array of prices to make level lookup faster O(log n) vs O(1).

Hashmap implementation - ~100 KB memory, 1.81 μs/match
Array - 6.4 MB memory, 0.63 μs/match

Memory is much cheaper than lost time.

This approach does slow down add and cancel order operations but still within sub-microsecond speeds.

[11/23/2025]

Fixed huge performance hog with returning filled_quantity. Was previously recalculating via an O(n) loop through trades. Now passing filled_quantity via reference and updating it directly. 

Similar improvement for the trades vector.