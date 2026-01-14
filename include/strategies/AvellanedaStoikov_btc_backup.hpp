//so we got our mathematics coefficients from the train_model.py now we are going to use to create our brain.....
//Author : Movazed
#pragma once
#include <cmath>
#include <algorithm>
#include <iostream>
#include <limits> 
#include <fstream> // Added for file reading
#include <string>  // Added for string manipulation
#include <sstream> // Added for parsing

namespace strategy {

class AvellanedaStoikov {
    private:
    // ml Coefficients from python train model...
    // removed constexpr. Now these are variables that can be updated by the JSON loader.
    // We keep the old values as "Safety Defaults" in case the file is missing.
    double model_bias_ = -0.275567;
    double model_weight_ = 333.739206;  

        // parameter tunings ("zen config to avoid toxic flow....")
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
        // Constructor: Automatically attempts to load the latest brain from Python
        AvellanedaStoikov() {
            load_model("/home/arya6/HFT_Engine/Quant_Research/models/latest_model.json");
        }

        // Simple JSON Parser (No external libraries required)
        void load_model(const std::string& filepath) {
            std::ifstream file(filepath);
            if (!file.is_open()) {
                std::cerr << "[CPP WARNING] Could not open model file: " << filepath 
                          << ". Using default safety values." << std::endl;
                return;
            }

            std::string line;
            while (std::getline(file, line)) {
                // look for "model_bias": some val xx
                if (line.find("\"model_bias\"") != std::string::npos) {
                    model_bias_ = extract_value(line);
                }
                // Look for "model_weight": some val xx
                if (line.find("\"model_weight\"") != std::string::npos) {
                    model_weight_ = extract_value(line);
                }
            }
            std::cout << "[CPP FUNC] Loaded Model -> Bias: " << model_bias_ 
                      << ", Weight: " << model_weight_ << std::endl;
        }

        // Helper to parse numbers from JSON line
        double extract_value(std::string line) {
            size_t colon_pos = line.find(':');
            if (colon_pos == std::string::npos) return 0.0;
            
            std::string val_str = line.substr(colon_pos + 1);
            // Clean up trailing commas if present
            size_t comma_pos = val_str.find(',');
            if (comma_pos != std::string::npos) {
                val_str = val_str.substr(0, comma_pos);
            }
            return std::stod(val_str);
        }

        struct Quote{
            double bid_price;
            double ask_price;
            bool panic_mode;
        };

        bool is_high_risk(double rolling_volatility){
            // Use the loaded member variables instead of static constants
            double z = model_bias_ + (model_weight_ * rolling_volatility);
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
            // DYNAMIC SPREAD LOGIC (Edit 15)
            // Volatility Coefficient: 200,000.0
            // Logic:
            // If Vol is 0.00001 (Calm), Spread = ~2.00
            // If Vol is 0.00004 (Storm), Spread = ~8.00
            // If Vol is 0.00010 (Crash), Spread = ~20.00
            
            double calculated_spread = current_volatility * 200000.0; //wanna play safe use 200k 
            
            // Floor the spread at 2.0 so we don't quote 0.0 in dead markets
            double spread = std::max(2.0, calculated_spread);

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
// we made a profit of 21,434.01 on bad market using the good market ALgorithm  less goooooooo
// Edit 16 : Brain Transplant. Removed constexpr. Engine now loads 'latest_model.json' from Python automatically.

// we are going to now make it play on live market.....