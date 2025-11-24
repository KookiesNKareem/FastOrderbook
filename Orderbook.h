#ifndef ORDERBOOK_H
#define ORDERBOOK_H

#include "OrderUtils.h"
#include <iostream>
#include <unordered_map>
#include <vector>
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


vector<Trade> fill_order(uint64_t order_id, Side side, uint32_t price, uint32_t quantity, uint32_t& filled_quantity, vector<Trade>& trades)
{
    uint32_t remaining = quantity;
    filled_quantity = 0;

    if (side == Side::BUY)
    {
        // match against sells
        while (remaining > 0 && best_ask < MAX_PRICE && best_ask <= price)
        {
            PriceLevel& level = sell_side[best_ask];
            if (level.order_ids.empty()) {
                // find next ask
                best_ask++;
                while (best_ask < MAX_PRICE && sell_side[best_ask].order_ids.empty()) {
                    best_ask++;
                }
                continue;
            }

            uint64_t resting_order_id = level.order_ids.front();
            auto order_iter = orders.find(resting_order_id);

            Order& resting_order = order_iter->second;

            uint32_t match_quantity = min(remaining, resting_order.quantity);
            trades.emplace_back(order_id, resting_order_id, best_ask, match_quantity);
            remaining -= match_quantity;
            filled_quantity += match_quantity;

            resting_order.quantity -= match_quantity;
            level.total_quantity -= match_quantity;

            if (resting_order.quantity == 0)
            {
                orders.erase(order_iter);
                level.order_ids.pop_front();

                if (level.order_ids.empty()) {
                    // find next ask
                    best_ask++;
                    while (best_ask < MAX_PRICE && sell_side[best_ask].order_ids.empty()) {
                        best_ask++;
                    }
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
                // find next bid price
                best_bid--;
                while (best_bid > 0 && buy_side[best_bid].order_ids.empty()) {
                    best_bid--;
                }
                continue;
            }

            uint64_t resting_order_id = level.order_ids.front();
            auto order_iter = orders.find(resting_order_id);

            // check for stale order_ids
            if (order_iter == orders.end()) {
                level.order_ids.pop_front();
                if (level.order_ids.empty()) {
                    best_bid--;
                    while (best_bid > 0 && buy_side[best_bid].order_ids.empty()) {
                        best_bid--;
                    }
                }
                continue;
            }

            Order& resting_order = order_iter->second;

            uint32_t match_quantity = min(remaining, resting_order.quantity);
            trades.emplace_back(order_id, resting_order_id, best_bid, match_quantity);
            remaining -= match_quantity;
            filled_quantity += match_quantity;

            resting_order.quantity -= match_quantity;
            level.total_quantity -= match_quantity;

            if (resting_order.quantity == 0)
            {
                orders.erase(order_iter);
                level.order_ids.pop_front();

                if (level.order_ids.empty()) {
                    // find next bid price
                    best_bid--;
                    while (best_bid > 0 && buy_side[best_bid].order_ids.empty()) {
                        best_bid--;
                    }
                }
            }
        }
    }

    return trades;
}

void add_order(uint64_t order_id, Side side, uint32_t price, uint32_t quantity)
{
    // bounds check for price
    if (price >= MAX_PRICE) {
        return;
    }

    vector<Trade> trades;
    trades.reserve(100); // avoid re-allocations 99% of the time
    uint32_t filled_quantity = 0;

    // try to fill first
    trades = fill_order(order_id, side, price, quantity, filled_quantity, trades);

    if (filled_quantity >= quantity)
    {
        return; // fully filled
    }

    uint32_t remaining_quantity = quantity - filled_quantity;

    PriceLevel& level = (side == Side::BUY) ? buy_side[price] : sell_side[price];

    if (level.price == 0) {
        level.price = price;
    }

    level.order_ids.push_back(order_id);
    level.total_quantity += remaining_quantity;

    orders.emplace(order_id, Order(order_id, side, price, remaining_quantity));

    // update best bid/ask if necessary
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
    if (it == orders.end()) {
        return;
    }

    Order& order = it->second;
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

    // update best bid/ask if this was the best level and it's now empty
    if (level.order_ids.empty()) {
        level.price = 0;
        level.total_quantity = 0;

        if (order.side == Side::BUY && price == best_bid) {
            // find new best bid
            if (best_bid > 0) {
                best_bid--;
                while (best_bid > 0 && buy_side[best_bid].order_ids.empty()) {
                    best_bid--;
                }
            }
        } else if (order.side == Side::SELL && price == best_ask) {
            // find new best ask
            if (best_ask < MAX_PRICE - 1) {
                best_ask++;
                while (best_ask < MAX_PRICE && sell_side[best_ask].order_ids.empty()) {
                    best_ask++;
                }
            } else {
                best_ask = MAX_PRICE;
            }
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

    // reset best prices
    best_bid = 0;
    best_ask = MAX_PRICE;
}

#endif // ORDERBOOK_H
