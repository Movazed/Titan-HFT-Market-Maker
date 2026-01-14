# now in this script we are going to check in under simulation of real world or reality check on 200 ms
# source ~/HFT_Engine/Quant_Research/venv/bin/activate activation of env
import sys
import os
import pandas as pd
import numpy as np
import time

# Point to cpp build folder
build_path = os.path.abspath(os.path.join(os.path.dirname(__file__), "../../build"))
if os.path.exists(build_path):
    sys.path.insert(0, build_path)
else:
    print(f"Warning: Build path does not exist: {build_path}")

try:
    import hft_core
except ImportError:
    print("CRITICAL ERROR: Could not import 'hft_core'. Check your build_path.")
    sys.exit(1)

class Backtester:
    def __init__(self, data_path):
        print(f"[BACKTEST] Loading data from {data_path}...")
        self.df = pd.read_csv(data_path)
        
        # FIX: The new generator saves as 'price', but we need 'mid_price'
        if 'price' in self.df.columns:
            self.df.rename(columns={'price': 'mid_price'}, inplace=True)
        
        # initializing cpp strats......
        self.strategy = hft_core.Strategy()
        
        # accounting
        self.inventory = 0
        self.cash = 10000.0 
        self.pnl_history = []
        self.trade_count = 0
        
    def run(self, latency_ms=200):
        print(f"[BACKTEST] Running Simulation with {latency_ms}ms Latency...")
        start_time = time.time()
        
        # pre-calculate inputs for the engine
        self.df['returns'] = self.df['mid_price'].pct_change().fillna(0)
        self.df['rolling_vol'] = self.df['returns'].rolling(window=50).std().fillna(0)
        
        # simulation Loop (Limit to 100k ticks for speed)
        # Ensure we don't go out of bounds with the lookahead
        lookahead = 2 # representing the "200ms" latency as 2 ticks
        limit = min(len(self.df) - lookahead - 1, 100000)
        
        for i in range(50, limit): 
            row = self.df.iloc[i]
            mid_price = row['mid_price']
            volatility = row['rolling_vol']
            
            # ask cpp for quotes
            quote = self.strategy.get_quote(mid_price, self.inventory, volatility)
            
            # sanity check debug (prints only once)
            if i == 50:
                print(f"[DEBUG] First Quote -> Bid: {quote.get('bid', 0):.2f}, Ask: {quote.get('ask', 0):.2f}, Mid: {mid_price:.2f}, Vol: {volatility:.5f}")

            # simulating Latency (Look 2 rows ahead)
            future_row = self.df.iloc[i + lookahead] 
            future_price = future_row['mid_price']
            
            # matching the orders
            # Note: We check if future price crosses our limit levels
            if future_price >= quote['ask']: # buyer hitting our ask
                self.inventory -= 1
                self.cash += quote['ask']
                self.trade_count += 1
                
            elif future_price <= quote['bid']: # seller hitting our bid
                self.inventory += 1
                self.cash -= quote['bid']
                self.trade_count += 1
                
            # tracking the profit and loss
            current_val = self.cash + (self.inventory * mid_price)
            self.pnl_history.append(current_val - 10000.0)

        duration = time.time() - start_time
        print(f"[DONE] Processed {limit - 50} ticks in {duration:.2f}s")
        self.generate_report()
            
    def generate_report(self):
        print("\n" + "="*50)
        print("    Hybrid Engine BackTesting Report")
        print("="*50)
        
        if not self.pnl_history:
            print("No Data Processed.")
            return

        final_pnl = self.pnl_history[-1]
        print(f"Total Profit & Loss:       ${final_pnl:.2f}")
        print(f"Total Trades:            {self.trade_count}")
        print(f"Final Inventory:         {self.inventory}")
        
        # FIX: Handle division by zero for Sharpe Ratio
        std_pnl = np.std(self.pnl_history)
        if std_pnl > 0 and len(self.pnl_history) > 0:
            sharpe = np.mean(self.pnl_history) / std_pnl
        else:
            sharpe = 0.0
            
        print(f"Sharpe Ratio:            {sharpe:.4f}")
        print("="*50 + "\n")

if __name__ == '__main__': 
    # data_path = "../data/synthetic_market_data.csv" 
    # data_path = "../data/hft_tick_data.csv" #rigged dataset or we can say bad market dataset
    data_path = "../data/ou_tick_data.csv" 
    
    if os.path.exists(data_path):
        bt = Backtester(data_path)
        bt.run(latency_ms=200)
    else:
        print("ERROR: Data file missing. Run generate py files first....")