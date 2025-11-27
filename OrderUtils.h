#include <cstdint>
#include <queue>
#include <deque>
#include <chrono>

using namespace std;

enum class Side {
    BUY,
    SELL
};

struct PriceLevel {
    uint32_t price = 0;
    deque<uint64_t> order_ids;
    uint32_t total_quantity = 0;
};

struct Trade {
    uint64_t buy_order_id;
    uint64_t sell_order_id;
    uint32_t price;
    uint32_t quantity;

    Trade() = default;

    Trade(uint64_t buy_order_id, uint64_t sell_order_id, uint32_t price, uint32_t quantity)
        : buy_order_id(buy_order_id), sell_order_id(sell_order_id), price(price), quantity(quantity) {}
};

struct Order {
    uint64_t order_id;
    Side side;
    uint32_t price;
    uint32_t quantity;
    bool deleted = false;

    Order(uint64_t order_id, Side side, uint32_t price, uint32_t quantity)
        : order_id(order_id), side(side), price(price), quantity(quantity), deleted(false) {}
};

struct Quote {
    uint32_t bid_price;
    uint32_t bid_quantity;
    uint32_t ask_price;
    uint32_t ask_quantity;

    Quote(uint32_t bid_price, uint32_t bid_quantity, uint32_t ask_price, uint32_t ask_quantity)
    {
        this->bid_price = bid_price;
        this->bid_quantity = bid_quantity;
        this->ask_price = ask_price;
        this->ask_quantity = ask_quantity;
    }
};
