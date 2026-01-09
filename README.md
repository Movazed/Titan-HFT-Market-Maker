# 🚀 Hybrid HFT Engine (C++ & Python)

**High-Frequency Trading System with ML-Powered Market Making Strategies**

![License](https://img.shields.io/badge/License-MIT-yellow.svg)
![Python](https://img.shields.io/badge/python-3.8+-blue.svg)
![C++](https://img.shields.io/badge/C++-17-red.svg)

A complete high-frequency trading framework that combines Python's machine learning capabilities with C++'s execution speed. This project demonstrates a professional quant trading pipeline from research to execution.

---

## ✨ Key Features

* **Hybrid Architecture** — Python for ML/model training + C++ for low-latency execution
* **Multiple Strategies** — Avellaneda-Stoikov market making with adaptive parameters
* **ML Integration** — Logistic regression models for market regime detection
* **Professional Pipeline** — Data → Research → Training → Backtesting → Execution
* **Educational** — Well-documented tuning history and strategy evolution

---

## 📊 Performance Highlights

| Metric        | Result                               |
| ------------- | ------------------------------------ |
| Turning Point | **-$83,000 → +$21,434**              |
| Sharpe Ratio  | **1.53**                             |
| Strategy      | Adaptive spreads + dynamic inventory |
| Latency       | Optimized for 200ms simulation       |

---

## 📁 Project Structure

```
HFT_ENGINE/
├── build/
│   ├── hft_core.cpython-312-x86_64-linux-gnu.so
│   ├── hft_engine
│   └── Makefile
│
├── include/
│   ├── core/
│   │   ├── ArenaAllocator.hpp
│   │   └── LockFreeQueue.hpp
│   │
│   ├── engine/
│   │   └── OrderBook.hpp
│   │
│   └── strategies/
│       ├── AvellanedaStoikov_bm.hpp
│       ├── AvellanedaStoikov_gm.hpp
│       └── AvellanedaStoikov.hpp
│
├── Quant_Research/
│   ├── data/
│   │   ├── hft_tick_data.csv
│   │   ├── ou_tick_data.csv
│   │   └── synthetic_market_data.csv
│   │
│   ├── models/
│   │   ├── latest_model.json
│   │   └── pnl_curve.png
│   │
│   ├── src/
│   │   ├── backtester.py
│   │   ├── train_model.py
│   │   ├── market_sim.py
│   │   └── generate_tick_data.py
│   │
│   ├── venv/
│   └── requirements.txt
│
├── src/
│   ├── engine/
│   ├── main.cpp
│   ├── bindings.cpp
│   └── .gitignore
│
├── CMakeLists.txt
└── README.md
```

---

## 🚀 Quick Start Guide

### Step 1 — Activate Python Environment

```bash
cd ~/HFT_ENGINE
source Quant_Research/venv/bin/activate
```

You should see `(venv)` in your shell.

---

### Step 2 — Build C++ Engine

```bash
cd build
cmake ..
make -j4
```

⚠️ Rebuild every time you change C++ files.

---

### Step 3 — Train ML Model

```bash
cd ../Quant_Research/src
python3 train_model.py
```

Expected output:

```
[SUCCESS] Model saved to ../models/latest_model.json
```

---

### Step 4 — Run Backtest

```bash
python3 backtester.py
```

---

## 🏆 Strategy Evolution History

| Edit | Name          | P&L          | Key Change            |
| ---- | ------------- | ------------ | --------------------- |
| 1    | Baseline      | -$91k        | Spread too tight      |
| 2    | Tighter       | -$12k        | Still too tight       |
| 3    | Hard Limits   | -$3k         | Inventory drift fixed |
| 4    | Inventory Fix | -$185        | Low volume            |
| 5    | Sniper Mode   | $0           | Spread too wide       |
| 6    | Volume Mode   | -$1.4k       | Overpaying            |
| 7    | Goldilocks    | -$450        | Killed volume         |
| 8    | Quick Hands   | -$1,185      | Gamma 5.0             |
| 9    | Profit Mode   | **+$21,434** | Adaptive spreads      |

---

## 🧠 Technical Implementation

### ML Model (Python)

```python
# Logistic regression for panic detection
z = MODEL_BIAS + (MODEL_WEIGHT * volatility)
probability = 1 / (1 + exp(-z))
```

---

### Trading Strategy (C++)

```cpp
// Avellaneda-Stoikov market making
double reservation_price = mid_price - (inventory * gamma * sigma2 * T);
double spread = danger ? panic_spread : normal_spread;
```

---

## ⚙️ Current Optimal Parameters (Edit 9)

```cpp
double gamma_ = 1.5;           // Inventory aggressiveness
double normal_spread_ = 18.0;  // Default spread
double panic_spread_ = 50.0;   // Spread during panic
int MAX_INVENTORY = 40;        // Max position size
```

---

## 📊 Sample Backtest Output

```
==================================================
   Hybrid Engine BackTesting Report
==================================================
Total Profit & Loss:       $21434.01
Total Trades:             127
Final Inventory:          1
Sharpe Ratio:             1.53
==================================================
```

---

## ⚙️ Tuning Guide

### If Losing Money

* Increase spreads by 20–30%
* Reduce `MAX_INVENTORY` to 25–30
* Increase `panic_spread_` to 60.0
* Raise panic threshold to 70%

### If Not Enough Trades

* Decrease `normal_spread_` to 15.0
* Lower panic threshold to 60%
* Increase `MAX_INVENTORY` to 50

---

## 🚨 Troubleshooting

### Module not found / ImportError

```bash
source Quant_Research/venv/bin/activate
```

---

### C++ changes not working

```bash
cd build
make clean
make -j4
```

---

### Zero trades or $0 P&L

* Check `models/latest_model.json` exists
* Re-run `python3 train_model.py`
* Reduce spreads in strategy file

---

## ⚠️ Important Notes

* Educational software only
* Not financial advice
* Not production-ready
* Provided data is toxic (rigged against market makers)

---

## 📚 Learning Resources

* *Algorithmic and High-Frequency Trading* — Cartea, Jaimungal, Penalva
* Avellaneda & Stoikov (2008) Market Making Paper
* *Advances in Financial Machine Learning* — Marcos López de Prado

---

## 🎉 Congratulations!

You're now running a complete hybrid HFT research engine.

**Golden Rules:**

* Always rebuild C++ after changes
* Start conservative and tighten gradually
* Track Sharpe ratio more than raw P&L
* Every loss teaches something

Happy algorithmic trading! 🚀

---


