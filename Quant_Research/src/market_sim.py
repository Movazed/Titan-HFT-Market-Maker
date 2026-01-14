# we are now generating data for testing this will generate 100000 in milisecs.....
import numpy as np
import pandas as pd

class Market_Simulator:
    ''' Generating systhentic (simulated).. high tick rates using Geometric Brownian Motion (GBM) with microservice noise....'''
    def __init__(self, start_price = 100.0, drift = 0.0, volatility = 0.1, dt = 1/23400):
        self.start_price = start_price  # dt = 1 second in trading day (approx 23400 seconds in 6.5 hrs)
        self.mu = drift
        self.sigma = volatility
        self.dt = dt

    def generate_tick_data(self, num_ticks=100000, seed=42):
        ''' Now we are simulating a price path.. which return dataframe with timestamp,and mid price'''
        np.random.seed(seed)

        # use wiener process for random shocks and std normal distribution
        shocks = np.random.normal(0, np.sqrt(self.dt), num_ticks)
        # calc path price GBM 
        # formula: P_t = P_{t-1} * exp((mu - 0.5*sigma^2)*dt + sigma*dW)

        drift_comp = (self.mu - 0.5 * self.sigma ** 2) * self.dt
        diffusion_comp = self.sigma * shocks
        
        # vectorized accumulation which has cumiliative prod which is faster than loops

        daily_returns = np.exp(drift_comp + diffusion_comp)
        price_path = self.start_price * np.cumprod(daily_returns)
        
        # adding up microstrcuture noise aka jitter, but real market bounces between Bid and Ask
        noise = np.random.normal(0, 0.005, num_ticks)
        noisy_price = price_path + noise

        # format by dataframe
        timestamps = pd.date_range(start="2024-01-01 09:30:00", periods=num_ticks, freq="100ms")
        df = pd.DataFrame({
            "timestamp": timestamps,
            "mid_price": noisy_price,
            "true_volatility": [self.sigma] * num_ticks  # This is the Answer key to our AI or data points
        })

        return df

if __name__ == "__main__":
    print("[SIM] Generating Synthetic Market Data...")

    # case 1 : Safe market (low volatility)
    sim_low = Market_Simulator(volatility=0.05)
    df_low = sim_low.generate_tick_data(num_ticks=750000)

    # case 2 : Panic Market (High Volatility)
    sim_high = Market_Simulator(volatility=0.2)
    df_high = sim_high.generate_tick_data(num_ticks=750000)

    #combining into single dataset
    df_combined = pd.concat([df_low, df_high]).reset_index(drop=True)

    #saving csv
    
    output_path = "../data/synthetic_market_data.csv"
    df_combined.to_csv(output_path, index=False)
    print(f"[SIM] Data saved to {output_path}")


#move to market_sim -> train_model for training script......

# source ~/HFT_Engine/Quant_Research/venv/bin/activate activation of env