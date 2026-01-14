#now we are going to make a good market dataset.... so it returns profit tick_data was rigged market
import pandas as pd
import numpy as np
import os

# CONFIGURATION
NUM_TICKS = 100000
START_PRICE = 45000.0
VOLATILITY = 0.5    # noises
THETA = 0.1         # mean reversion Strength (The "Rubber Band")
MU = 45000.0        # the "fair price"

output_path = "../data/ou_tick_data.csv"

print(f"Generating {NUM_TICKS} ticks of Mean-Reverting Data (OU Process)...")

prices = [START_PRICE]
current_price = START_PRICE

# Ornstein-Uhlenbeck Process
for _ in range(NUM_TICKS):
    dx = THETA * (MU - current_price) + np.random.normal(0, VOLATILITY)
    current_price += dx
    prices.append(current_price)

# DataFrame creation
df = pd.DataFrame(prices, columns=["price"])
df["quantity"] = np.random.uniform(0.001, 0.5, size=len(prices))
df["timestamp"] = pd.date_range(start="2024-01-01", periods=len(prices), freq="100ms")

# Save to data folder
df.to_csv(output_path, index=False)
print(f"DONE! Saved new dataset to: {output_path}")
