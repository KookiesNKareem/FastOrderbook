#ifndef ORDERBOOK_H
#define ORDERBOOK_H

#include "OrderUtils.h"
#include <iostream>
#include <unordered_map>
#include <map>
#include <vector>
#include <algorithm>

using namespace std;

unordered_map<uint64_t, Order> orders;

map<uint32_t, PriceLevel> buy_side;
map<uint32_t, PriceLevel> sell_side;

Quote get_quote()
{
    uint32_t bid_price = 0;
    uint32_t bid_quantity = 0;
    uint32_t ask_price = 0;
    uint32_t ask_quantity = 0;

    if (!buy_side.empty()) {
        auto best_bid_it = buy_side.rbegin();
        bid_price = best_bid_it->first;
        bid_quantity = best_bid_it->second.total_quantity;
    }

    if (!sell_side.empty()) {
        auto best_ask_it = sell_side.begin();
        ask_price = best_ask_it->first;
        ask_quantity = best_ask_it->second.total_quantity;
    }

    return Quote(bid_price, bid_quantity, ask_price, ask_quantity);
}


vector<Trade> fill_order(uint64_t order_id, Side side, uint32_t price, uint32_t quantity, uint32_t& filled_quantity)
{
    vector<Trade> trades;
    uint32_t remaining = quantity;
    if (side == Side::BUY)
    {
        if (sell_side.empty())
        {
            return trades;
        }
        auto best_ask_it = sell_side.begin();

        while (remaining > 0 && best_ask_it != sell_side.end() && !best_ask_it->second.order_ids.empty())
        {
            if (best_ask_it->first > price)
            {
                return trades;
            }
            uint64_t resting_order_id = best_ask_it->second.order_ids.front();
            Order& resting_order = orders.at(resting_order_id);

            uint32_t filled_quantity = min(remaining, resting_order.quantity);
            trades.emplace_back(order_id, resting_order_id, best_ask_it->first, filled_quantity);
            remaining -= filled_quantity;

            resting_order.quantity -= filled_quantity;
            if(resting_order.quantity <= 0)
            {
                orders.erase(resting_order_id);
                best_ask_it->second.order_ids.remove(resting_order_id);
            }
            best_ask_it->second.total_quantity -= filled_quantity;
            if (best_ask_it->second.total_quantity <= 0)
            {
                best_ask_it = sell_side.erase(best_ask_it);
            }
        }
    }
    else{
        if (buy_side.empty())
        {
            return trades;
        }

        // have to use different iterator system for sell-side
        auto best_bid_it = --buy_side.end();

        while (remaining > 0)
        {
            if (best_bid_it == buy_side.end() || best_bid_it->first < price) break;

            if (best_bid_it->second.order_ids.empty()) {
                if (best_bid_it == buy_side.begin()) break;
                --best_bid_it;
                continue;
            }

            uint64_t resting_order_id = best_bid_it->second.order_ids.front();
            Order& resting_order = orders.at(resting_order_id);

            uint32_t filled_quantity = min(remaining, resting_order.quantity);
            resting_order.quantity -= filled_quantity;
            best_bid_it->second.total_quantity -= filled_quantity;

            trades.emplace_back(order_id, resting_order_id, best_bid_it->first, filled_quantity);
            remaining -= filled_quantity;

            if (resting_order.quantity == 0)
            {
                best_bid_it->second.order_ids.remove(resting_order_id);
                orders.erase(resting_order_id);
            }

            // handle iterator edge cases
            if (best_bid_it->second.total_quantity == 0) {
                if (best_bid_it == buy_side.begin()) {
                    buy_side.erase(best_bid_it);
                    break;
                }
                auto temp_it = best_bid_it;
                --best_bid_it;
                buy_side.erase(temp_it);
            }
        }

    }
    return trades;
}

void add_order(uint64_t order_id, Side side, uint32_t price, uint32_t quantity)
{
    vector<Trade> trades;
    uint32_t filled_quantity;

    // passing filled_quantity via reference eliminates O(n) loop
    trades = fill_order(order_id, side, price, quantity, filled_quantity);

    if (filled_quantity >= quantity)
    {
        return;
    }
    uint32_t remaining_quantity = quantity - filled_quantity;

    map<uint32_t, PriceLevel>& price_levels = (side == Side::BUY) ? buy_side : sell_side;

    // more efficient initialization
    auto [price_level_it, inserted] = price_levels.emplace(price, PriceLevel{});
    PriceLevel& level = price_level_it->second;

    if (level.price == 0) {
        level.price = price;
    }

    level.order_ids.push_back(order_id);
    level.total_quantity += remaining_quantity;

    auto order_it = --level.order_ids.end();

    orders.emplace(order_id, Order(order_id, side, price, remaining_quantity, price_level_it, order_it));
}

void cancel_order(uint64_t order_id)
{
    auto it = orders.find(order_id);
    if (it == orders.end()) {
        return;
    }

    // order must be removed from price level queue and from orderbook
    Order& order = it->second;
    auto price_level_it = order.get_price_level_it();
    auto order_it = order.get_order_it();

    PriceLevel& level = price_level_it->second;
    level.order_ids.erase(order_it);
    level.total_quantity -= order.get_quantity();

    if (level.order_ids.empty()) {
        if (order.get_side() == Side::BUY) {
            buy_side.erase(price_level_it);
        } else {
            sell_side.erase(price_level_it);
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
    uint32_t old_quantity = order.get_quantity();

    auto price_level_it = order.get_price_level_it();
    PriceLevel& level = price_level_it->second;

    level.total_quantity = level.total_quantity - old_quantity + new_quantity;
    order.quantity = new_quantity;
}

void clear_orderbook() {
    orders.clear();
    buy_side.clear();
    sell_side.clear();
}

#endif // ORDERBOOK_H
