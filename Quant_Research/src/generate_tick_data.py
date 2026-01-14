#so ur biggest question will be why we will not be using synthetic market data because it acted as texted book and we got our coefficients from it now we generating a new batch
#which tests our engine fall off testing with higher resilution and latency...
#technically we are working on realtime data.....

import pandas as pd
import numpy as np
import os

def generate_hft_data():
    print("[DATA] Generating approx a million high frequency ticks...")

    #first we gonna buildup a settings of 100ms resolution
    num_ticks = 100000
    start_price = 45000.00
    volatility = 0.00005

    #now lets generate time and prices....

    timestamps = pd.date_range(start="2024-01-01 09:30:00", periods=num_ticks, freq="100ms")
    returns = np.random.normal(0, volatility, num_ticks)
    price_path = start_price * np.exp(np.cumsum(returns))

    # create dataframe with microstrcuture noise
    df = pd.DataFrame({"timestamp": timestamps, "mid_price": price_path})
    df['mid_price'] += np.random.normal(0, 0.05, num_ticks)

    output_dir = "../data"
    if not os.path.exists(output_dir): os.makedirs(output_dir)
    df.to_csv("../data/hft_tick_data.csv", index=False)
    print("[SUCCESS] Data Ready...")

if __name__ == "__main__":
    generate_hft_data()

#go to backtester.py for checking on 100ms res dataset.....

#we actually generated a rigged data set and the loss was 78.29 whihc is good for safer options...
#we are going to write a another script for no rigged data set

# source ~/HFT_Engine/Quant_Research/venv/bin/activate activation of env