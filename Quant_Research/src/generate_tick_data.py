#so ur biggest question will be why we will not be using synthetic market data because it acted as texted book and we got our coefficients from it now we generating a new batch
#which tests our engine fall off testing with higher resilution and latency...
#technically we are working on realtime data.....

import pandas
import numpy as np
import os

def generate_hft_data():
    print("[DATA] Generating approx a million high frequency ticks...")

    #first we gonna buildup a settings of 100ms resolution
    num_ticks = 100000
    start_price = 45000.00
    volatility = 0.00005

    #now lets generate time and prices....
    