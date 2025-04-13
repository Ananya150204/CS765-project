# CS765: Homework 3 - Building Your Own Decentralized Exchange (DEX)

## Contents

### Solidity Contracts

- `Token.sol` – ERC-20 token contract used to deploy TokenA and TokenB.
- `LPToken.sol` – ERC-20 compliant liquidity provider (LP) token contract, minted and burned by the DEX.
- `DEX.sol` – Core Automated Market Maker (AMM) DEX implementing the constant product formula (x * y = k).
- `arbitrage.sol` – Arbitrage smart contract that detects and executes profitable arbitrage opportunities across two DEXes.

### JavaScript Simulation

- `simulate_DEX.js` – Simulates the DEX with 5 LPs and 8 traders performing swaps, deposits, and withdrawals.
- `simulate_arbitrage.js` – Demonstrates both profitable and unprofitable arbitrage scenarios using deployed DEX contracts.

### Plotter

- `plot.py` – This reads the files from the `data` directory and generate plots in `outputs` directory.

### Data Files (in `data/` directory)

These files were generated during the simulation:

- `TVL.txt` – Total Value Locked over time.
- `fees_of_A.txt`, `fees_of_B.txt` – Accumulated fees in Token A and B.
- `swapped_A.txt`, `swapped_B.txt` – Token swap volumes.
- `spot_prices.txt` – Spot prices of Token A in terms of Token B.
- `slippages.txt` – Slippage for each swap.
- `trade_lot_fractions.txt` – Trade lot fractions for each swap.
- `lp_1.txt` to `lp_5.txt` – LP token balances for each of the five LPs.

### Plot Files

Generated using Python (`plot.py`):

- `TVL_vs_time.png`
- `fees_vs_time.png`
- `swap_volume_vs_time.png`
- `spot_price_vs_time.png`
- `slippage_vs_time.png`
- `slippage_vs_trade_lot_fraction.png`
- `lp_owner.png` – LP token balance for LP1
- `lp_others.png` – LP token balances for LP2 to LP5

### Report

- `report.pdf` – LaTeX report that includes:
  - Simulation plots and captions
  - Answers to theory questions
  - Mathematical derivation of slippage in terms of trade lot fraction

## Instructions

1. Open Remix IDE (https://remix.ethereum.org).
2. Copy all the `.sol` files in the contracts folder and the simulate js files in the tests folder.
3. Run `simulate_DEX.js` in the Remix Script Runner to simulate randomized user interactions.
4. Data will be saved to the Remix file system via `remix.call('fileManager', 'writeFile', ...)`.
5. Download the data files from Remix and place them into a local `data/` directory.
6. Run `plot_dex_metrics.py` using Python to generate visualizations from the data files.