'''we will be training the model using Avellaneda-Stoikov strategy.... into C++ and combine them, this is for weights'''
'''we will check the last 50 ticks and will use the math coefficients in C++'''

import pandas as pd
import numpy as np
from sklearn.linear_model import LogisticRegression # we will be using Logictic Regression cuz it uses heat maps or boolean values to be precise YES/NO... gng
from sklearn.metrics import classification_report

def train():
    print("[ML] Loading all the rows .... ") # first we load the data from synthetic market data...
    try:
        df = pd.read_csv("../data/synthetic_market_data.csv")
    except FileNotFoundError:
        print("ERROR: Data file not found. Run market_sim.py first.")
        return

    print("[ML] Calculating features...")  # feature Engineering we calculate volatility on how C++ engine would do it
    df['returns'] = df['mid_price'].pct_change() # feature rolling volatality

    # FIX: Calculate the actual rolling volatility (Standard Deviation)
    df['rolling_vol'] = df['returns'].rolling(window=50).std()

    # FIX: Create the 'target' column (The Answer Key)
    df['target'] = (df['true_volatility'] > 0.1).astype(int)

    # now we are dropping first 50 rows...
    df.dropna(inplace=True)

    #Now we come to the training part , the input x is the rolling volatility we see and output y is the regime we are in..
    X = df[['rolling_vol']].values 
    y = df['target'].values

    print("[ML] Training Logistic Regression Model....")
    model = LogisticRegression()
    model.fit(X, y)

    # now the main thing the main part we are going to create a brain and import to C++ engine we are going to extract math coefficients from python in production
    intercept = model.intercept_[0]
    coef = model.coef_[0][0]   
      
    print("\n" + "="*50)
    print(" ||| Copy these lines for our C++ header |||")
    print("-" * 50)
    print(f"constexpr static double MODEL_BIAS = {intercept:.6f};")
    print(f"constexpr static double MODEL_WEIGHT = {coef:.6f};")
    print("="*50 + "\n")
    
if __name__ == "__main__":
    train()

    '''we can see that at the end we are going to find something like this ->>
            ==================================================
            ||| Copy these lines for our C++ header |||
            --------------------------------------------------
            constexpr static double MODEL_BIAS = some particular value;
            constexpr static double MODEL_WEIGHT = some particular value;
            ==================================================

            Reduced to 6 decimals, now we have to copy those lines with constexpr 

            So by this we conclude we got the weights =) now move to ./include/strategies/AvellanedaStoikov.hpp
    '''