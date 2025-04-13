import matplotlib.pyplot as plt
import os

# Folder containing your data files
data_folder = "../data"

# Helper to read data from file and return list of floats
def read_file(filename):
    with open(os.path.join(data_folder, filename)) as f:
        return [float(line.strip()) for line in f.readlines() if line.strip()]

# Time axis (assuming same length for all time-based metrics)
def time_axis(length):
    return list(range(1, length + 1))

# Read all required files
TVL = read_file("TVL.txt")
fees_A = read_file("fees_of_A.txt")
fees_B = read_file("fees_of_B.txt")
swapped_A = read_file("swapped_A.txt")
swapped_B = read_file("swapped_B.txt")
spot_prices = read_file("spot_prices.txt")
slippages = read_file("slippages.txt")
trade_lot_fractions = read_file("trade_lot_fractions.txt")

# Read LP token balances
lp_balances = []
num_lps = 5

for i in range(1, num_lps + 1):
    lp_balances.append(read_file(f"lp_{i}.txt"))

# Plot 1: Total Value Locked (TVL) vs Time
plt.figure(figsize=(8, 4))
plt.plot(time_axis(len(TVL)), TVL, label="TVL", color='darkblue')
plt.title("Total Value Locked (TVL) over Time")
plt.xlabel("Time Step")
plt.ylabel("TVL (USD)")
plt.grid(True)
plt.tight_layout()
plt.savefig("TVL_vs_time.png")
plt.show()

# Plot 2: Fees Collected vs Time
plt.figure(figsize=(8, 4))
plt.plot(time_axis(len(fees_A)), fees_A, label="Fees in Token A", color='green')
plt.plot(time_axis(len(fees_B)), fees_B, label="Fees in Token B", color='orange')
plt.title("Fees Accumulated over Time")
plt.xlabel("Time Step")
plt.ylabel("Fees")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig("fees_vs_time.png")
plt.show()

# Plot 3: Swap Volume vs Time
plt.figure(figsize=(8, 4))
plt.plot(time_axis(len(swapped_A)), swapped_A, label="Swapped A", color='red')
plt.plot(time_axis(len(swapped_B)), swapped_B, label="Swapped B", color='purple')
plt.title("Swap Volume over Time")
plt.xlabel("Time Step")
plt.ylabel("Amount Swapped")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig("swap_volume_vs_time.png")
plt.show()

# Plot 4: Spot Price vs Time
plt.figure(figsize=(8, 4))
plt.plot(time_axis(len(spot_prices)), spot_prices, label="Spot Price (A/B)", color='teal')
plt.title("Spot Price over Time")
plt.xlabel("Time Step")
plt.ylabel("Price (TokenA / TokenB)")
plt.grid(True)
plt.tight_layout()
plt.savefig("spot_price_vs_time.png")
plt.show()

# Plot 5: Slippage vs Time
plt.figure(figsize=(8, 4))
plt.plot(time_axis(len(slippages)), slippages, label="Slippage (%)", color='maroon')
plt.title("Slippage over Time")
plt.xlabel("Time Step")
plt.ylabel("Slippage (%)")
plt.grid(True)
plt.tight_layout()
plt.savefig("slippage_vs_time.png")
plt.show()

# Plot 6: Slippage vs Trade Lot Fraction
plt.figure(figsize=(8, 4))
plt.scatter(trade_lot_fractions, slippages, color='brown', s=15)
plt.title("Slippage vs Trade Lot Fraction")
plt.xlabel("Trade Lot Fraction (x/reserve_x)")
plt.ylabel("Slippage (%)")
plt.grid(True)
plt.tight_layout()
plt.savefig("slippage_vs_trade_lot_fraction.png")
plt.show()

# Plot 7a: LP1 Token Balance in separate plot
plt.figure(figsize=(8, 4))
plt.plot(time_axis(len(lp_balances[0])), lp_balances[0], label="LP 1", color='blue')
plt.title("LP 1 Token Balance over Time")
plt.xlabel("Time Step")
plt.ylabel("LP Token Balance")
plt.grid(True)
plt.tight_layout()
plt.savefig("lp_owner.png")
plt.show()

# Plot 7b: LP2–LP5 Token Balances in a single plot
plt.figure(figsize=(10, 5))
for i in range(1, num_lps):
    plt.plot(time_axis(len(lp_balances[i])), lp_balances[i], label=f"LP {i+1}")
plt.title("LP 2–5 Token Balances over Time")
plt.xlabel("Time Step")
plt.ylabel("LP Token Balance")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig("lp_others.png")
plt.show()
