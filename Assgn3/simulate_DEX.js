async function simulateDEX() {
    console.log("Starting DEX Simulation...");

    // Get all accounts from Remix
    const accounts = await web3.eth.getAccounts();
    const owner = accounts[0];
    const LPs = accounts.slice(1, 6);       // 5 LPs
    const traders = accounts.slice(6, 14);  // 8 traders

    const N = 60; // Number of transactions to simulate
    const SCALE = BigInt(1e18);

    // ---------------------- Load ABIs -------------------------
    const tokenMetadata = JSON.parse(await remix.call('fileManager', 'getFile', 'browser/contracts/artifacts/Token.json'));
    const dexMetadata = JSON.parse(await remix.call('fileManager', 'getFile', 'browser/contracts/artifacts/DEX.json'));

    const tokenABI = tokenMetadata.abi;
    const dexABI = dexMetadata.abi;

    // ---------------------- Deployed Contract Addresses -------------------------
    const tokenAAddress = "0x3DD1B5124E30867796004bB8e011e067bA3D2C4c";
    const tokenBAddress = "0x5510Ba53349CC2A13897aE08dF5888D4A4768c0A";
    const dexAddress = "0xdE82f047e97B5E09b52725A291A4dA76a347783a";

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
    }

    // ---------------------- LPs Add Liquidity -------------------------
    console.log("LPs adding liquidity...");

    for (let i = 0; i < LPs.length; i++) {
        const user = LPs[i];
        let amountA, amountB;

        // Get current reserves from DEX
        const reserveA = BigInt(await dex.methods.get_reserveA().call());
        const reserveB = BigInt(await dex.methods.get_reserveB().call());
        if (reserveA === 0n && reserveB === 0n) {
            // First LP can choose arbitrary values
            amountA = BigInt(100) * SCALE;
            amountB = BigInt(150) * SCALE;
            console.log("KYU AAAAAAAAAAAAAAAAAAAAAAAAAAYAAAAAAAAAAAAAAAAaa");
        } else {
            // Choose an amountA and calculate amountB according to the ratio
            amountA = BigInt(100 + i * 10) * SCALE;
            amountB = (amountA * reserveB) / reserveA;

            // Ensure no rounding issue
            if (reserveA * amountB !== reserveB * amountA) {
                console.warn(`⚠️ Rounded ratio mismatch for ${user}. Skipping this LP.`);
                continue;
            }
        }

        await tokenA.methods.approve(dexAddress, amountA.toString()).send({ from: user });
        await tokenB.methods.approve(dexAddress, amountB.toString()).send({ from: user });

        try {
            await dex.methods.addLiquidity(amountA.toString(), amountB.toString()).send({ from: user });
            console.log(`✅ ${user} added liquidity: A=${Number(amountA) / 1e18}, B=${Number(amountB) / 1e18}`);
        } catch (err) {
            console.error(`❌ Liquidity add failed for ${user}, ${amountA}, ${amountB}: ${err.message}`);
        }
    }
    // ---------------------- Simulation Loop -------------------------
    console.log("Starting random simulation...");

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
                    const withdrawAmount = lpBal / 2n;
                    console.log(withdrawAmount);
                    try {
                        await dex.methods.removeLiquidity(withdrawAmount.toString()).send({ from: user });
                        console.log(`🔄 LP ${user} removed liquidity: ${Number(withdrawAmount) / 1e18} LPT`);
                    } catch (err) {
                        console.error(`❌ LP withdraw failed for ${user}: ${err.message}`);
                    }
                }
            } else {
                // Trader swap
                const tokenChoice = Math.random() < 0.5 ? 'A' : 'B';

                try {
                    if (tokenChoice === 'A') {
                        const balance = BigInt(await tokenA.methods.balanceOf(user).call());
                        const reserveA = BigInt(await dex.methods.get_reserveA().call());
                        const amount = balance < reserveA ? balance / 10n : reserveA / 10n;

                        if (amount > 0n) {
                            try{
                                await tokenA.methods.approve(dexAddress, amount.toString()).send({ from: user });
                                await dex.methods.swap_A_to_B(amount.toString()).send({ from: user });
                                console.log(amount);
                                console.log(`🔁 Trader ${user} swapped ${Number(amount) / 1e18} A for B`);
                            }catch(err){
                                console.error(`❌ Swapping error A to B, ${err.message}`);
                            }
                        }
                    } else {
                        const balance = BigInt(await tokenB.methods.balanceOf(user).call());
                        const reserveB = BigInt(await dex.methods.get_reserveB().call());
                        const amount = balance < reserveB ? balance / 10n : reserveB / 10n;

                        if (amount > 0n) {
                            try{
                                await tokenB.methods.approve(dexAddress, amount.toString()).send({ from: user });
                                await dex.methods.swap_B_to_A(amount.toString()).send({ from: user });
                                console.log(`🔁 Trader ${user} swapped ${Number(amount) / 1e18} B for A`);
                            } catch(err){
                                console.error(`❌ Swapping error B to A, ${err.message}`);
                            }
                        }
                    }
                } catch (err) {
                    console.error(`❌ Swap failed for ${user}: ${err.message}`);
                }
            }
        }catch(err){
            console.error(`Error at i = ${i}, ${err.message}`);
        }
    }

    // ---------------------- Final Metrics -------------------------
    console.log("Fetching DEX stats...");

    const reserveA = BigInt(await dex.methods.get_reserveA().call());
    const reserveB = BigInt(await dex.methods.get_reserveB().call());
    const spotPrice = BigInt(await dex.methods.spotPrice().call());

    console.log(`\n--- 📊 DEX Final Stats ---`);
    console.log(`Reserve A: ${Number(reserveA) / 1e18}`);
    console.log(`Reserve B: ${Number(reserveB) / 1e18}`);
    console.log(`Spot Price (TokenA/TokenB): ${Number(spotPrice) / 1e18}`);

    for (let i = 0; i < LPs.length; i++) {
        const lpt = BigInt(await dex.methods.get_lp_tokens(LPs[i]).call());
        console.log(`LP ${LPs[i]} LPT balance: ${Number(lpt) / 1e18}`);
    }

    console.log("✅ Simulation complete.");
}

simulateDEX();

