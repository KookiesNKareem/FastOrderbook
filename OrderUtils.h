#include <cstdint>
#include <queue>
#include <list>
#include <map>
#include <chrono>

using namespace std;

// Forward declarations and type definitions must come first
enum class Side {
    BUY,
    SELL
};

struct PriceLevel {
    uint32_t price;
    list<uint64_t> order_ids;
    uint32_t total_quantity;
};

struct Trade {
    uint64_t buy_order_id;
    uint64_t sell_order_id;
    uint32_t price;              // Execution price (resting order's price)
    uint32_t quantity;         // Amount traded
    uint64_t timestamp;        // When trade occurred

    Trade(uint64_t buy_order_id, uint64_t sell_order_id, uint32_t price, uint32_t quantity)
    {
        this->buy_order_id = buy_order_id;
        this->sell_order_id = sell_order_id;
        this->price = price;
        this->quantity = quantity;
        this->timestamp = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
    }
};

class Order {
    public:
        uint64_t order_id;
        Side side;
        uint32_t price;
        uint32_t quantity;
        uint64_t timestamp;
        map<uint32_t, PriceLevel>::iterator price_level_it;  
        list<uint64_t>::iterator order_it;                

        Order(uint64_t order_id, Side side, uint32_t price, uint32_t quantity,
                map<uint32_t, PriceLevel>::iterator price_level_it,
                list<uint64_t>::iterator order_it) {
            this->order_id = order_id;
            this->side = side;
            this->price = price;
            this->quantity = quantity;
            this->price_level_it = price_level_it;
            this->order_it = order_it;
        }

        Side get_side() const { return side; }
        uint32_t get_price() const { return price; }
        uint32_t get_quantity() const { return quantity; }
        map<uint32_t, PriceLevel>::iterator get_price_level_it() { return price_level_it; }
        list<uint64_t>::iterator get_order_it() { return order_it; }

        void Modify(uint32_t new_quantity)
        {
            this->quantity = new_quantity;
        }
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
