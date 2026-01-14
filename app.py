import streamlit as st
import pandas as pd
import plotly.graph_objects as go
import time

st.set_page_config(page_title="Live DashBoard", layout="wide")
st.title("Titan Live Market Maker Dashboard")

placeholder = st.empty()

def load_data():
    try:
        # Read only the last 200 rows to keep it fast
        df = pd.read_csv("live_data.csv")
        return df.tail(200)
    except:
        return pd.DataFrame(columns=["timestamp", "price", "pnl", "inventory", "bid", "ask"])

while True:
    df = load_data()
    
    if not df.empty:
        last_row = df.iloc[-1]
        
        # Create a unique ID for this update cycle
        unique_id = time.time()
        
        with placeholder.container():
            # KPI Metrics
            kpi1, kpi2, kpi3 = st.columns(3)
            pnl_val = last_row['pnl']
            
            kpi1.metric(label="Profit/Loss (USDT)", value=f"${pnl_val:.2f}", delta=f"{pnl_val:.2f}")
            kpi2.metric(label="BTC Price", value=f"${last_row['price']:.2f}")
            kpi3.metric(label="Inventory (mBTC)", value=f"{int(last_row['inventory'])}")

            # Charts Row
            col1, col2 = st.columns([3, 1])

            with col1:
                st.subheader("Market Depth & Spread")
                fig = go.Figure()
                fig.add_trace(go.Scatter(x=df['timestamp'], y=df['price'], mode='lines', name='Price', line=dict(color='white')))
                fig.add_trace(go.Scatter(x=df['timestamp'], y=df['bid'], mode='lines', name='My Bid', line=dict(color='green', dash='dot')))
                fig.add_trace(go.Scatter(x=df['timestamp'], y=df['ask'], mode='lines', name='My Ask', line=dict(color='red', dash='dot')))
                fig.update_layout(template="plotly_dark", margin=dict(l=0, r=0, t=30, b=0), height=400)
                
                st.plotly_chart(fig, key=f"price_chart_{unique_id}", use_container_width=True)

            with col2:
                st.subheader("PnL Curve")
                fig_pnl = go.Figure()
                color = 'green' if pnl_val >= 0 else 'red'
                fig_pnl.add_trace(go.Scatter(x=df['timestamp'], y=df['pnl'], mode='lines', name='PnL', line=dict(color=color, width=2)))
                fig_pnl.update_layout(template="plotly_dark", margin=dict(l=0, r=0, t=30, b=0), height=400)
                

                st.plotly_chart(fig_pnl, key=f"pnl_chart_{unique_id}", use_container_width=True)

    time.sleep(0.5)