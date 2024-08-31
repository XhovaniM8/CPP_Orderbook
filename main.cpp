#include <iostream>      // Provides facilities for input and output, including cin, cout, cerr, etc.
#include <map>           // Implements the std::map container, which is an associative array.
#include <set>           // Implements the std::set container, which is a collection of unique elements.
#include <list>          // Implements the std::list container, which is a doubly linked list.
#include <cmath>         // Provides mathematical functions, such as sqrt(), sin(), cos(), etc.
#include <ctime>         // Provides functions for manipulating date and time information.
#include <deque>         // Implements the std::deque container, which is a double-ended queue.
#include <queue>         // Implements the std::queue and std::priority_queue containers.
#include <stack>         // Implements the std::stack container, which is a last-in-first-out (LIFO) stack.
#include <limits>        // Provides information about the properties of fundamental types.
#include <string>        // Implements the std::string class for working with strings.
#include <vector>        // Implements the std::vector container, which is a dynamic array.
#include <numeric>       // Provides numeric operations like accumulate(), inner_product(), etc.
#include <algorithm>     // Provides algorithms for operations on containers, like sort(), find(), etc.
#include <unordered_map> // Implements the std::unordered_map container, which is a hash table.
#include <memory>        // Provides facilities for dynamic memory management, including smart pointers.
#include <variant>       // Implements the std::variant type, which can hold a value of one of several types.
#include <optional>      // Implements the std::optional type, which represents a value that may or may not be present.
#include <tuple>         // Implements the std::tuple class template for working with fixed-size collections of heterogeneous values.
#include <format>        // Provides facilities for formatted output (similar to printf-style formatting but more type-safe and flexible).
#include <sstream>

// Enum representing different order types
enum class OrderType
{
    GoodTillCancel,
    FillAndKill
};

// Enum representing order sides (buy or sell)
enum class Side{
    Buy,
    Sell
};

// Type aliases for better readability and type safety
using Price = std::int32_t;     // Can be negative, so make it an int
using Quantity = std::uint32_t; // Can't be negative so make it unsigned
using OrderId = std::uint64_t;  // Unique identifier for orders

// Structure to hold information about a price level
struct LevelInfo
{
    Price price_;
    Quantity quantity_;
};

using LevelInfos = std::vector<LevelInfo>; // Alias for a collection of LevelInfo

// Class representing the order book levels (bids and asks)
class OrderbookLevelInfos
{
public:
    OrderbookLevelInfos(const LevelInfos& bids, const LevelInfos& asks)
            : bids_{ bids }
            , asks_{ asks }
    { }

    const LevelInfos& GetBids() const { return bids_; }
    const LevelInfos& GetAsks() const { return asks_; }

private:
    LevelInfos bids_;
    LevelInfos asks_;
};

// Class representing an order
class Order{
public:
    Order(OrderType orderType, OrderId orderId, Side side, Price price, Quantity quantity)
            : orderType_ { orderType }
            , orderId_ { orderId }
            , side_ { side }
            , price_ { price }
            , initialQuantity_ { quantity }
            , remainingQuantity_ { quantity }
    { }

    OrderId GetOrderId() const {return orderId_;}
    Side GetSide() const { return side_; }
    Price GetPrice()const {return price_; }
    OrderType GetOrderType() const {return orderType_; }
    Quantity GetInitialQuantity() const { return initialQuantity_; }
    Quantity GetRemainingQuantity() const { return remainingQuantity_; }
    Quantity GetFilledQuantity() const { return GetInitialQuantity() - GetRemainingQuantity(); }
    void Fill(Quantity quantity)
    {
        if (quantity > GetRemainingQuantity()) {
            std::stringstream ss;
            ss << "Order (" << GetOrderId() << ") cannot be filled for more than its remaining quantity.";
            throw std::logic_error(ss.str());
        }

        remainingQuantity_ -= quantity;
    }

    bool IsFilled() const { return remainingQuantity_ == 0; } // Added for checking if the order is filled

private:
    OrderType orderType_;
    OrderId orderId_;
    Side side_;
    Price price_;
    Quantity initialQuantity_;
    Quantity remainingQuantity_;
};

using OrderPointer = std::shared_ptr<Order>;   // Alias for shared pointer to an Order
using OrderPointers = std::list<OrderPointer>; // Alias for a list of shared pointers to Orders

// Class representing an order modification
class OrderModify {
public:
    OrderModify(OrderId orderId, Side side, Price price, Quantity quantity)
            : orderId_ { orderId }
            , price_ { price }
            , side_ { side }
            , quantity_ { quantity }
    { }

    OrderId GetOrderId() const { return orderId_; }
    Price GetPrice() const { return price_; }
    Side GetSide() const { return side_; }
    Quantity GetQuantity() const { return quantity_; }

    OrderPointer ToOrderPointer(OrderType type) const
    {
        return std::make_shared<Order>(type, GetOrderId(), GetSide(), GetPrice(), GetQuantity());
    }

private:
    OrderId orderId_;
    Price price_;
    Side side_;
    Quantity quantity_;
};

// Structure to hold information about a trade
struct TradeInfo
{
    OrderId orderId_;
    Price price_;
    Quantity quantity_;
};

// Class representing a trade
class Trade{
public:
    Trade(const TradeInfo& bidTrade, const TradeInfo& askTrade)
            : bidTrade_{ bidTrade }
            , askTrade_{ askTrade }
    { }

    const TradeInfo& GetBidTrade() const { return bidTrade_; }
    const TradeInfo& GetAskTrade() const { return askTrade_; }

private:
    TradeInfo bidTrade_;
    TradeInfo askTrade_; // Fixed typo: changed from TeadeInfo to TradeInfo
};

using Trades = std::vector<Trade>; // Alias for a collection of trades

// Class representing the order book
class Orderbook
{
private:
    struct OrderEntry
    {
        OrderPointer order_{ nullptr };
        OrderPointers::iterator location_;
    };

    std::map<Price, OrderPointers, std::greater<Price>> bids_; // Map of bid prices to orders, sorted by price descending
    std::map<Price, OrderPointers, std::less<Price>> asks_;    // Map of ask prices to orders, sorted by price ascending
    std::unordered_map<OrderId, OrderEntry> orders_;           // Unordered map of order IDs to order entries

    // Checks if an order can be matched based on its side and price
    bool CanMatch(Side side, Price price) const {
        if (side == Side::Buy)
        {
            if (asks_.empty())
                return false;

            const auto& [bestAsk, _] = *asks_.begin();
            return price >= bestAsk;
        }
        else
        {
            if (bids_.empty())
                return false;

            const auto& [bestBid, _] = *bids_.begin();
            return price <= bestBid;
        }
    }

    // Matches orders in the order book
    Trades MatchOrders()
    {
        Trades trades;
        trades.reserve(orders_.size());

        while (true)
        {
            if (bids_.empty() || asks_.empty())
                break;

            auto& [bidPrice, bids] = *bids_.begin();
            auto& [askPrice, asks] = *asks_.begin();

            if (bidPrice < askPrice)
                break;

            while (!bids.empty() && !asks.empty())
            {
                auto& bid = bids.front();
                auto& ask = asks.front();

                Quantity quantity = std::min(bid->GetRemainingQuantity(), ask->GetRemainingQuantity());

                bid->Fill(quantity);
                ask->Fill(quantity);

                if (bid->IsFilled())
                {
                    bids.pop_front();
                    orders_.erase(bid->GetOrderId());
                }

                if (ask->IsFilled())
                {
                    asks.pop_front();
                    orders_.erase(ask->GetOrderId());
                }

                if (bids.empty())
                    bids_.erase(bidPrice);

                if (asks.empty())
                    asks_.erase(askPrice);

                trades.push_back(Trade{
                        TradeInfo{ bid->GetOrderId(), bid->GetPrice(), quantity},
                        TradeInfo{ ask->GetOrderId(), ask->GetPrice(), quantity}
                });
            }

            // Call the function to check and cancel FillAndKill orders
            CheckAndCancelFillAndKillOrders();

            return trades;
        }
    }

    // Function to check and cancel FillAndKill orders
    void CheckAndCancelFillAndKillOrders()
    {
        auto cancelFillAndKillOrder = [this](auto& orderContainer)
        {
            if (!orderContainer.empty())
            {
                auto& [_, orders] = *orderContainer.begin();
                auto& order = orders.front();
                if (order->GetOrderType() == OrderType::FillAndKill)
                    CancelOrder(order->GetOrderId());
            }
        };

        cancelFillAndKillOrder(bids_);
        cancelFillAndKillOrder(asks_);
    }

public:
    Trades AddOrder(OrderPointer order)
    {
        if (orders_.contains(order->GetOrderId()))
            return ( );

        if (order->GetOrderType() == OrderType::FillAndKill && !CanMatch(order->GetSide(), order ->GetPrice()))
            return { };

        OrderPointers::iterator iterator;

        if (order->GetSide() == Side::Buy)
        {
            auto& orders = bids_[order->GetPrice()];
            order.push_back(order);
            iterator = std::next(orders.begin(), orders.size() - 1);
        }
        else
        {
            auto& orders = bids_[order->GetPrice()];
            orders.push_back(order);
            iterator = std::next(orders.begin(), orders.size() - 1);
        }
        orders_.insert({ order->GetOrderId(), OrderEntry{ order, iterator }});
        return MatchOrders();
    }

    void CancelOrder(OrderId orderId)
    {
        if (!orders_.contains(orderId))
            return;

        const auto& [order, iterator] = orders_.at(orderId);
        orders_.erase(orderId);

        if (order->GetSide() == Side::Sell)
        {
            auto price = order->GetPrice();
            auto& orders = asks_.at(price);
            orders.erase(iterator);
            if (orders.empty())
                asks_.erase(price);
        }
        else
        {
            auto price = order->GetPrice();
            auto& orders = bids_.at(price);
            orders.erase(iterator);
            if (orders.empty())
                bids_.erase(price);
        }
    }

    Trades MatchOrder(OrderModify order)
    {
        if (!orders_.contains(order.GetOrderId()))
            return { };

        const auto& [existingOrder, _] = orders_.at(order.GetOrderId());
        CancelOrder(order.GetOrderId());
        return AddOrder(order.ToOrderPointer(existingOrder->GetOrderType()));
    }

    std::size_t Size() const { return orders_.size(); }

    OrderbookLevelInfos GetOrderInfos() const
    {
        LevelInfos bidInfos, askInfos;
        bidInfos.reserve(order_.size());
        askInfos.reserve(order_.size());

        auto CreateLevelInfos = [](Price price, const OrderPointers& orders) {
            return LevelInfo{price, std::accumulate(orders.begin(), orders.end(), (Quantity) 0,
                                                    [](std::size_t runningSum, const OrderPointers &order) {
                                                        return runningSum + order->GetRemainingQuantity(); }) };
        };
        for (const auto&)
    }
};

int main() {
    // Main function for testing the order book and order processing

    return 0;
}
