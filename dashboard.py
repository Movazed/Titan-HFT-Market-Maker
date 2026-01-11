#creating a dashboard for a live view of Profit and loss counter of our algorithm...
import sys
import time
from collections import deque

# We use plotext because it renders REAL graphs in the terminal (Headless friendly)
import plotext as plt 

# config
MAX_POINTS = 100 # Keep it fast for terminal rendering

# buffers
prices = deque(maxlen=MAX_POINTS)
pnls = deque(maxlen=MAX_POINTS)
bids = deque(maxlen=MAX_POINTS)
asks = deque(maxlen=MAX_POINTS)
timestamps = deque(maxlen=MAX_POINTS)

def main():
    # Clear screen initially
    plt.clt()
    
    print("\033[94m[DASHBOARD] Waiting for C++ Engine Data Stream...\033[0m")
    
    while True:
        try:
            # Read line from C++ Engine (Blocking)
            line = sys.stdin.readline()
            if not line: break
            
            # capturing data stream
            if line.startswith("DATA,"):
                parts = line.strip().split(',')
                # parsing: DATA, PRICE, PNL, INV, BID, ASK
                try:
                    price = float(parts[1])
                    pnl = float(parts[2])
                    inv = int(parts[3])
                    bid = float(parts[4])
                    ask = float(parts[5])

                    prices.append(price)
                    pnls.append(pnl)
                    timestamps.append(len(prices))

                    # rendering terminal 
                    plt.clt() # clear terminal ofc

                    # plt 1 btc price
                    plt.subplots(2, 1) # 2 Rows, 1 Column
                    
                    plt.subplot(1, 1)
                    plt.title(f"LIVE BTC: ${price:,.2f} | Inventory: {inv}")
                    plt.plot(list(prices), label="Price", color="green")
                    plt.theme("dark")
                    plt.grid(0, 1)

                    # profit and loss
                    plt.subplot(2, 1)
                    pnl_str = f"${pnl:,.2f}"
                    plt.title(f"PROFIT/LOSS: {pnl_str}")
                    plt.plot(list(pnls), label="PnL ($)", color="blue" if pnl >= 0 else "red")
                    plt.grid(0, 1)
                    
                    # force drawings
                    plt.show()

                    # print recent below graphs
                    print(f"Spread: {bid:.2f} <-> {ask:.2f} | Inv: {inv}")

                except ValueError:
                    pass
            
            # capture Trade Fills (Pass them to console)
            elif line.startswith(">>>"):
                # print Fills at the bottom
                if "BUY" in line:
                    print(f"\033[92m{line.strip()}\033[0m") # green for buy
                else:
                    print(f"\033[91m{line.strip()}\033[0m") # red for sell

        except KeyboardInterrupt:
            break
        except Exception as e:
            continue

if __name__ == "__main__":
    main()

    #ts on progress gng ...... =(