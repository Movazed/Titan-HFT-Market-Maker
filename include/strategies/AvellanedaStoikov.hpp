//so we got our mathematics coefficients from the train_model.py now we are going to use to create our brain.....
//Author : Movazed
#pragma once
#include <cmath>
#include <algorithm>
#include <iostream>

namespace strategy {

class AvellanedaStoikov {
    private:
    // we are going to use coefficients which we got from train_model.py and combining with the brain from python
    constexpr static double MODEL_BIAS = -0.275567;
    constexpr static double MODEL_WEIGHT = 333.739206;  
    // we got this value from train_model.py we will be using cpp for not staggering process we can change accordingly

    // if the volatility changes the data set or synthetic market data will change so these values might possible be edited

    // now we will use Stoikov model
        double gamma_ = 0.5;
        double k_ = 1.5;
        double sigma_ = 0.1;
        double T_ = 1.0;


    public:
        struct Quote{
            double bid_price;
            double ask_price;
            bool panic_mode;
        };
        // above strcuture checks if market is in panic mode using our ML weights
        // P(Panic) = 1 / (1 + e ^ -(bias + weight * vol))  e is exponents or to the power  and vol is volatility if u were confused incase

        bool is_high_risk(double rolling_volatility){
            //using Linear equation for line z = mx + c
            double z = MODEL_BIAS + (MODEL_WEIGHT * rolling_volatility);

            // lets say if z > 0 the probability panic is > 50%
            return z > 0;
        }

        // now lets say we are going to calculate Bid / Ask quotes based on inventory & volatility

        Quote calculate_quote(double mid_price, int inventory, double current_volatility){
            sigma_ = current_volatility;     //setting current volatility

            bool danger = is_high_risk(current_volatility);     // we are asking the CU / Brain for predictions
            
            double reservation_price = mid_price - (inventory * gamma_ * std::pow(sigma_, 2) * T_);
            
            // what we are actually doing above?? -> we are calculating reservation price formula: r = s - (q * gamma * sigma^2 * T) , 
            // but if we have inventory (i > 0), we lower prices to sell

            double spread = danger ? 2.0 : 0.5; // what we did here was calculate the spread to protect ourselves during Panic mode or risky market ..

            Quote q;
            q.bid_price = reservation_price - (spread  / 2.0);
            q.ask_price = reservation_price + (spread / 2.0);
            q.panic_mode = danger;

            return q;
        }
    };
}  

// Now we are going to implement the new startegy to the main engine.... so go back to main.cpp located src /engine