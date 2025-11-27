#ifndef ORDERBOOK_H
#define ORDERBOOK_H

#include "OrderUtils.h"
#include <iostream>
#include <unordered_map>
#include <algorithm>

using namespace std;

// array-based price levels for O(1) access
constexpr uint32_t MAX_PRICE = 100000;

unordered_map<uint64_t, Order> orders;

PriceLevel buy_side[MAX_PRICE];
PriceLevel sell_side[MAX_PRICE];

// track best prices for speed
uint32_t best_bid = 0;
uint32_t best_ask = MAX_PRICE;

// static trade buffer to avoid allocations in hot path
constexpr uint32_t MAX_TRADES = 256;
Trade trade_buffer[MAX_TRADES];
uint32_t trade_count = 0;

// bitmaps for O(1) best bid/ask lookup
constexpr uint32_t BITMAP_SIZE = (MAX_PRICE + 63) / 64;
uint64_t bid_bitmap[BITMAP_SIZE] = {0};
uint64_t ask_bitmap[BITMAP_SIZE] = {0};

inline void set_level_active(uint32_t price, bool is_bid) {
    uint64_t* bmp = is_bid ? bid_bitmap : ask_bitmap;
    bmp[price / 64] |= (1ULL << (price % 64));
}

inline void set_level_inactive(uint32_t price, bool is_bid) {
    uint64_t* bmp = is_bid ? bid_bitmap : ask_bitmap;
    bmp[price / 64] &= ~(1ULL << (price % 64));
}

inline uint32_t find_best_bid() {
    for (int i = BITMAP_SIZE - 1; i >= 0; --i) {
        if (bid_bitmap[i]) {
            // count leading zeros
            return i * 64 + (63 - __builtin_clzll(bid_bitmap[i]));
        }
    }
    return 0;
}

inline uint32_t find_best_ask() {
    for (uint32_t i = 0; i < BITMAP_SIZE; ++i) {
        if (ask_bitmap[i]) {
            // count trailing zeroes
            return i * 64 + __builtin_ctzll(ask_bitmap[i]);
        }
    }
    return MAX_PRICE;
}

Quote get_quote()
{
    uint32_t bid_price = 0;
    uint32_t bid_quantity = 0;
    uint32_t ask_price = 0;
    uint32_t ask_quantity = 0;

    if (best_bid > 0 && buy_side[best_bid].total_quantity > 0) {
        bid_price = best_bid;
        bid_quantity = buy_side[best_bid].total_quantity;
    }

    if (best_ask < MAX_PRICE && sell_side[best_ask].total_quantity > 0) {
        ask_price = best_ask;
        ask_quantity = sell_side[best_ask].total_quantity;
    }

    return Quote(bid_price, bid_quantity, ask_price, ask_quantity);
}


void fill_order(uint64_t order_id, Side side, uint32_t price, uint32_t quantity, uint32_t& filled_quantity)
{
    uint32_t remaining = quantity;
    filled_quantity = 0;
    trade_count = 0;

    if (side == Side::BUY)
    {
        // match against sells
        while (remaining > 0 && best_ask < MAX_PRICE && best_ask <= price)
        {
            PriceLevel& level = sell_side[best_ask];
            if (level.order_ids.empty()) {
                set_level_inactive(best_ask, false);
                best_ask = find_best_ask();
                continue;
            }

            uint64_t resting_order_id = level.order_ids.front();
            auto order_iter = orders.find(resting_order_id);

            // skip deleted/stale orders
            if (order_iter == orders.end() || order_iter->second.deleted) {
                level.order_ids.pop_front();
                if (level.order_ids.empty()) {
                    set_level_inactive(best_ask, false);
                    best_ask = find_best_ask();
                }
                continue;
            }

            Order& resting_order = order_iter->second;

            uint32_t match_quantity = min(remaining, resting_order.quantity);
            if (trade_count < MAX_TRADES) {
                trade_buffer[trade_count++] = Trade(order_id, resting_order_id, best_ask, match_quantity);
            }
            remaining -= match_quantity;
            filled_quantity += match_quantity;

            resting_order.quantity -= match_quantity;
            level.total_quantity -= match_quantity;

            if (resting_order.quantity == 0)
            {
                resting_order.deleted = true;  // tombstone
                level.order_ids.pop_front();

                if (level.order_ids.empty()) {
                    set_level_inactive(best_ask, false);
                    best_ask = find_best_ask();
                }
            }
        }
    }
    else // SELL
    {
        // match against buys
        while (remaining > 0 && best_bid > 0 && best_bid >= price)
        {
            PriceLevel& level = buy_side[best_bid];
            if (level.order_ids.empty()) {
                set_level_inactive(best_bid, true);
                best_bid = find_best_bid();
                continue;
            }

            uint64_t resting_order_id = level.order_ids.front();
            auto order_iter = orders.find(resting_order_id);

            // skip deleted/stale orders
            if (order_iter == orders.end() || order_iter->second.deleted) {
                level.order_ids.pop_front();
                if (level.order_ids.empty()) {
                    set_level_inactive(best_bid, true);
                    best_bid = find_best_bid();
                }
                continue;
            }

            Order& resting_order = order_iter->second;

            uint32_t match_quantity = min(remaining, resting_order.quantity);
            if (trade_count < MAX_TRADES) {
                trade_buffer[trade_count++] = Trade(order_id, resting_order_id, best_bid, match_quantity);
            }
            remaining -= match_quantity;
            filled_quantity += match_quantity;

            resting_order.quantity -= match_quantity;
            level.total_quantity -= match_quantity;

            if (resting_order.quantity == 0)
            {
                resting_order.deleted = true;  // tombstone
                level.order_ids.pop_front();

                if (level.order_ids.empty()) {
                    set_level_inactive(best_bid, true);
                    best_bid = find_best_bid();
                }
            }
        }
    }
}

// batch cleanup of tombstoned orders
void cleanup_deleted_orders() {
    for (auto it = orders.begin(); it != orders.end(); ) {
        if (it->second.deleted) {
            it = orders.erase(it);
        } else {
            ++it;
        }
    }
}

void add_order(uint64_t order_id, Side side, uint32_t price, uint32_t quantity)
{
    // bounds check for price
    if (price >= MAX_PRICE) {
        return;
    }

    uint32_t filled_quantity = 0;

    // try to fill first
    fill_order(order_id, side, price, quantity, filled_quantity);

    if (filled_quantity >= quantity)
    {
        return; // fully filled
    }

    uint32_t remaining_quantity = quantity - filled_quantity;

    PriceLevel& level = (side == Side::BUY) ? buy_side[price] : sell_side[price];

    bool was_empty = level.order_ids.empty();

    if (level.price == 0) {
        level.price = price;
    }

    level.order_ids.push_back(order_id);
    level.total_quantity += remaining_quantity;

    orders.emplace(order_id, Order(order_id, side, price, remaining_quantity));

    // update bitmap and best bid/ask if this level just became active
    if (was_empty) {
        set_level_active(price, side == Side::BUY);
    }

    if (side == Side::BUY) {
        if (price > best_bid) {
            best_bid = price;
        }
    } else {
        if (price < best_ask) {
            best_ask = price;
        }
    }
}

void cancel_order(uint64_t order_id)
{
    auto it = orders.find(order_id);
    if (it == orders.end() || it->second.deleted) {
        return;
    }

    Order& order = it->second;
    order.deleted = true;  // tombstone
    uint32_t price = order.price;

    PriceLevel& level = (order.side == Side::BUY) ? buy_side[price] : sell_side[price];

    // find and remove order_id
    for (auto deque_it = level.order_ids.begin(); deque_it != level.order_ids.end(); ++deque_it) {
        if (*deque_it == order_id) {
            level.order_ids.erase(deque_it);
            break;
        }
    }

    level.total_quantity -= order.quantity;

    // update bitmap and best bid/ask if this level is now empty
    if (level.order_ids.empty()) {
        level.price = 0;
        level.total_quantity = 0;

        bool is_bid = (order.side == Side::BUY);
        set_level_inactive(price, is_bid);

        if (is_bid && price == best_bid) {
            best_bid = find_best_bid();
        } else if (!is_bid && price == best_ask) {
            best_ask = find_best_ask();
        }
    }

    orders.erase(it);
}

void modify_order(uint64_t order_id, uint32_t new_quantity)
{
    auto it = orders.find(order_id);
    if (it == orders.end()) {
        return;
    }

    Order& order = it->second;
    uint32_t old_quantity = order.quantity;
    uint32_t price = order.price;

    PriceLevel& level = (order.side == Side::BUY) ? buy_side[price] : sell_side[price];

    level.total_quantity = level.total_quantity - old_quantity + new_quantity;
    order.quantity = new_quantity;
}

void clear_orderbook() {
    orders.clear();

    // reset arrays
    for (uint32_t i = 0; i < MAX_PRICE; i++) {
        buy_side[i] = PriceLevel{};
        sell_side[i] = PriceLevel{};
    }

    // reset bitmaps
    for (uint32_t i = 0; i < BITMAP_SIZE; i++) {
        bid_bitmap[i] = 0;
        ask_bitmap[i] = 0;
    }

    // reset best prices
    best_bid = 0;
    best_ask = MAX_PRICE;
}

#endif // ORDERBOOK_H
