async function simulateDEX() {
    console.log("Starting DEX Simulation...");

    // Get all accounts from Remix
    const accounts = await web3.eth.getAccounts();
    const owner = accounts[0];
    const LPs = [owner, ...accounts.slice(1, 6)]; // owner + 5 LPs
    const traders = accounts.slice(6, 14);  // 8 traders
    const initialBalances = {}; // user => { amountA: BigInt, amountB: BigInt }
    const spotPrices = []; // To store 61 spot prices
    const slippages = []; // To store 60 slippage values

    const N = 60; // Number of transactions to simulate
    const SCALE = BigInt(1e18);

    // ---------------------- Load ABIs -------------------------
    const tokenMetadata = JSON.parse(await remix.call('fileManager', 'getFile', 'browser/contracts/artifacts/Token.json'));
    const dexMetadata = JSON.parse(await remix.call('fileManager', 'getFile', 'browser/contracts/artifacts/DEX.json'));

    const tokenABI = tokenMetadata.abi;
    const dexABI = dexMetadata.abi;

    // ---------------------- Deployed Contract Addresses -------------------------
    const tokenAAddress = "0x4FFd0210B68a0fE369B75798E4e2Eb9E9236678d";
    const tokenBAddress = "0x646EcD9A2aB1f35553F369d7710cB0bA5a6e12CF";
    const dexAddress = "0x35Fb2Eb1784c372DB0D1D402F45950d08cCE459C";

    const tokenA = new web3.eth.Contract(tokenABI, tokenAAddress);
    const tokenB = new web3.eth.Contract(tokenABI, tokenBAddress);
    const dex = new web3.eth.Contract(dexABI, dexAddress);

    console.log("Loaded contracts successfully.");

    // ---------------------- Mint Tokens to Users -------------------------
    console.log("Minting tokens to LPs and Traders...");

    for (let user of [...LPs, ...traders]) {
        try {
            await tokenA.methods.transfer(user, (BigInt(2000) * SCALE).toString()).send({ from: owner });
            console.log(`✅ TokenA minted to ${user}`);
        } catch (err) {
            console.error(`❌ TokenA transfer failed for ${user}: ${err.message}`);
        }

        try {
            await tokenB.methods.transfer(user, (BigInt(2000) * SCALE).toString()).send({ from: owner });
            console.log(`✅ TokenB minted to ${user}`);
        } catch (err) {
            console.error(`❌ TokenB transfer failed for ${user}: ${err.message}`);
        }

        const amountA = BigInt(await tokenA.methods.balanceOf(user).call());
        const amountB = BigInt(await tokenB.methods.balanceOf(user).call());
        initialBalances[user] = { amountA: amountA, amountB: amountB };
    }

    // ---------------------- LPs Add Liquidity -------------------------
    console.log("LPs adding liquidity...");

    for (let i = 0; i < LPs.length; i++) {
        const user = LPs[i];
        let amountA, amountB;

        // Get current reserves from DEX
        const reserveA = BigInt(await dex.methods.get_reserveA().call());
        const reserveB = BigInt(await dex.methods.get_reserveB().call());
        if ((reserveA === 0n) && (reserveB === 0n)) {
            // First LP can choose arbitrary values
            amountA = BigInt(100) * SCALE;
            amountB = BigInt(150) * SCALE;
        } else {
            // Choose an amountA and calculate amountB according to the ratio
            amountA = BigInt(100 + i * 10) * SCALE;
            amountB = (amountA * reserveB) / reserveA;
        }

        await tokenA.methods.approve(dexAddress, amountA.toString()).send({ from: user });
        await tokenB.methods.approve(dexAddress, amountB.toString()).send({ from: user });

        await dex.methods.addLiquidity(amountA.toString(), amountB.toString()).send({ from: user });
        console.log(`✅ ${user} added liquidity: A=${Number(amountA) / 1e18}, B=${Number(amountB) / 1e18}`);
    }
    // ---------------------- Simulation Loop -------------------------
    console.log("Starting random simulation...");

    let sp = await dex.methods.spotPrice().call();
    spotPrices.push(Number(sp)/1e18);
    for (let i = 0; i < N; i++) {
        try{
            console.log(i);
            const isLPAction = Math.random() < 0.4;
            const userPool = isLPAction ? LPs : traders;
            const user = userPool[Math.floor(Math.random() * userPool.length)];

            if (isLPAction) {
                // LP withdraws liquidity
                const lpBal = BigInt(await dex.methods.get_lp_tokens(user).call());
                if (lpBal > 0n) {
                    const withdrawAmount = lpBal / 10n;
                    await dex.methods.removeLiquidity(withdrawAmount.toString()).send({ from: user });
                    console.log(`🔄 LP ${user} removed liquidity: ${Number(withdrawAmount) / 1e18} LPT`);
                }
            } else {
                // Trader swap
                const tokenChoice = Math.random() < 0.5 ? 'A' : 'B';
                let ratio;

                if (tokenChoice === 'A') {
                    const balance = BigInt(await tokenA.methods.balanceOf(user).call());
                    const reserveA = BigInt(await dex.methods.get_reserveA().call());
                    const amount = balance < reserveA ? balance / 10n : reserveA / 10n;

                    if (amount > 0n) {
                        await tokenA.methods.approve(dexAddress, amount.toString()).send({ from: user });
                        await dex.methods.swap_A_to_B(amount.toString()).send({ from: user });
                        console.log(`🔁 Trader ${user} swapped ${Number(amount) / 1e18} A for B`);
                    }
                } else {
                    const balance = BigInt(await tokenB.methods.balanceOf(user).call());
                    const reserveB = BigInt(await dex.methods.get_reserveB().call());
                    const amount = balance < reserveB ? balance / 10n : reserveB / 10n;

                    if (amount > 0n) {
                        await tokenB.methods.approve(dexAddress, amount.toString()).send({ from: user });
                        await dex.methods.swap_B_to_A(amount.toString()).send({ from: user });
                        console.log(`🔁 Trader ${user} swapped ${Number(amount) / 1e18} B for A`);
                    }
                }
            }
            sp = await dex.methods.spotPrice().call();
            spotPrices.push(Number(sp)/1e18);
        }catch(err){
            console.error(`Error at i = ${i}, ${err.message}`);
        }
    }

    console.log("\n💸 LPs removing all liquidity to realize fees...");
    const finalWithdrawals = {}; // user => { amountA, amountB }

    for (const user of LPs) {
        const lpBal = BigInt(await dex.methods.get_lp_tokens(user).call());
        if (lpBal > 0n) {
            // Remove all liquidity
            await dex.methods.removeLiquidity(lpBal.toString()).send({ from: user });
            console.log(`🧾 ${user} removed ${Number(lpBal) / 1e18} LPT`);

            const finalA = BigInt(await tokenA.methods.balanceOf(user).call());
            const finalB = BigInt(await tokenB.methods.balanceOf(user).call());

            finalWithdrawals[user] = { amountA: finalA, amountB: finalB };
        }
    }

    console.log("\n--- 💰 LP Earnings from Fees ---");
    for (const user of LPs) {
        const initial = initialBalances[user];
        const final = finalWithdrawals[user];

        if (!initial || !final) continue;

        const earnedA = final.amountA - initial.amountA;
        const earnedB = final.amountB - initial.amountB;

        console.log(`👤 LP ${user}`);
        console.log(`   • TokenA Earned in Fees: ${Number(earnedA) / 1e18}`);
        console.log(`   • TokenB Earned in Fees: ${Number(earnedB) / 1e18}`);
    }


    // ---------------------- Final Metrics -------------------------
    console.log("Fetching DEX stats...");

    const spotPricesText = spotPrices.join('\n');
    await remix.call('fileManager', 'writeFile', 'browser/spot_prices.txt', spotPricesText);
    console.log("📄 Spot prices written to browser/spot_prices.txt");

    console.log("✅ Simulation complete.");
}

simulateDEX();

