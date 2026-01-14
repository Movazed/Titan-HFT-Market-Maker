//so we got our mathematics coefficients from the train_model.py now we are going to use to create our brain.....
//Author : Movazed
#pragma once
#include <cmath>
#include <algorithm>
#include <iostream>
#include <limits> 

namespace strategy {

class AvellanedaStoikov {
    private:
    // ml Coefficients from python train model...
    constexpr static double MODEL_BIAS = -0.275567;
    constexpr static double MODEL_WEIGHT = 333.739206;  

        // parameter tunings ("The Zen Config")
        // Gamma 0.001: "Sniper Mode". 
        // Reduced from 0.01 to 0.001 to prevent the skew from becoming too wide.
        // We want to be sticky around the mid-price.

        double gamma_ = 0.001; 
        double k_ = 1.5;
        double sigma_ = 0.1;
        double T_ = 1.0;

        // MAX Inventory to absorb the waves
        int MAX_INVENTORY = 100; 

    public:
        struct Quote{
            double bid_price;
            double ask_price;
            bool panic_mode;
        };

        bool is_high_risk(double rolling_volatility){
            double z = MODEL_BIAS + (MODEL_WEIGHT * rolling_volatility);
            return z > 0;
        }

        Quote calculate_quote(double mid_price, int inventory, double current_volatility){
            sigma_ = current_volatility; 

            bool danger = is_high_risk(current_volatility);
            
            // skew calcs
            // scalar reduced to 500.0.
            // since we are "zen/safe players" (Gamma 0.001), we don't want to skew much. 
            // we want to keep our bid and ask centered around the Mid Price to get fills on both sides.

            double reservation_price = mid_price - (inventory * gamma_ * std::pow(sigma_ * 500.0, 2) * T_); 
            
            // spread change....
            // Previous 23.0 resulted in 0 trades (Coward Scenario). 
            // Dropping to 2.0 to force interaction with the order book.
            double spread = danger ? 10.0 : 2.0; 

            double final_bid = reservation_price - (spread / 2.0);
            double final_ask = reservation_price + (spread / 2.0);
            
            // hard limits
            if (inventory >= MAX_INVENTORY) final_bid = 0.0; 
            if (inventory <= -MAX_INVENTORY) final_ask = std::numeric_limits<double>::infinity(); 

            return {final_bid, final_ask, danger};
        }
    };
}
// Tuning History:
// Edit 1: -$91k loss. The spread (0.5) was basically free money for arbitrage bots.
// Edit 2: -$12k loss. Better, but spread was still too tight for 200ms latency.
// Edit 3: -$3k loss. Fixed the inventory drift with Hard Limits.
// Edit 4: -$185 loss. Inventory fixed, but not enough trades to beat variance.
// Edit 5: -$0 (Sniper Mode). Spread 40.0/30.0 was too greedy. Ghost town.
// Edit 6: -$1.4k (Volume Mode). Spread 20.0 got trades, but paying too much.
// Edit 7: -$450 (Goldilocks). Spread 24.0 killed volume (22 trades).
// Edit 8: -$1.1k (Quick Hands). Panic selling caused losses.
// Edit 9: -$78 (Diamond Hands). Very stable, but too low volume (35 trades).
// Edit 10: -$1k (Profit Hunter).
// Edit 11: -$78 (Zen Mode). Identical to Edit 9.
// Edit 12 : Sniper Zen. Gamma 0.001 + Spread 28.0. (Failed: 0 Trades)
// Edit 13 : The Final Stand. Spread 25.0 + Gamma 0.001. (Failed: 0 Trades)
// Edit 14 : Reality Check. Gamma 0.001 + Spread 2.0. Force trades to happen.
// Edit 15 : Dynamic Defense. Replaced static spread with (Vol * 200,000) to survive toxic flow.

//Its ofcourse a good market now let us implement this on bad market