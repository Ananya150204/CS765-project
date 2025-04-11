async function simulateDEX() {
    console.log("Starting DEX Simulation...");

    // Get all accounts from Remix
    const accounts = await web3.eth.getAccounts();
    const owner = accounts[0];
    const LPs = [owner, ...accounts.slice(1, 5)]; // owner + 5 LPs
    const traders = accounts.slice(5, 13);  // 8 traders
    const initialBalances = {}; // user => { amountA: BigInt, amountB: BigInt }
    const spotPrices = []; // To store 61 spot prices
    const total_reservesA = [];     //61 vals
    const total_reservesB = [];     // 61 vals
    const slippages = []; // To store <=60 slippage values (only for swaps)
    const trade_lot_fractions = [];
    const lpTokenBalances = {}; // user => [array of lp token balances]

    const N = 60; // Number of transactions to simulate
    const SCALE = BigInt(1e18);

    // ---------------------- Load ABIs -------------------------
    const tokenMetadata = JSON.parse(await remix.call('fileManager', 'getFile', 'browser/contracts/artifacts/Token.json'));
    const dexMetadata = JSON.parse(await remix.call('fileManager', 'getFile', 'browser/contracts/artifacts/DEX.json'));

    const tokenABI = tokenMetadata.abi;
    const dexABI = dexMetadata.abi;

    // ---------------------- Deployed Contract Addresses -------------------------
    const tokenAAddress = "0xdB9B39b354c01Eb512189e187FF5dD7A1d05797e";
    const tokenBAddress = "0xF67ABE5f81F856c69869381fa46c094aF1855DbB";
    const dexAddress = "0xD435321abf961D86fDb9b1054344df9Fe17eE485";

    const tokenA = new web3.eth.Contract(tokenABI, tokenAAddress);
    const tokenB = new web3.eth.Contract(tokenABI, tokenBAddress);
    const dex = new web3.eth.Contract(dexABI, dexAddress);

    console.log("Loaded contracts successfully.");

    // ---------------------- Mint Tokens to Users -------------------------
    console.log("Minting tokens to LPs and Traders...");

    for (let user of [...LPs, ...traders]) {
        try {
            await tokenA.methods.transfer(user, (BigInt(2000) * SCALE).toString()).send({ from: owner });
            console.log(`\u2705 TokenA minted to ${user}`);
        } catch (err) {
            console.error(`\u274C TokenA transfer failed for ${user}: ${err.message}`);
        }

        try {
            await tokenB.methods.transfer(user, (BigInt(2000) * SCALE).toString()).send({ from: owner });
            console.log(`\u2705 TokenB minted to ${user}`);
        } catch (err) {
            console.error(`\u274C TokenB transfer failed for ${user}: ${err.message}`);
        }
    }

    // ---------------------- LPs Add Liquidity -------------------------
    console.log("LPs adding liquidity...");

    for (let i = 0; i < LPs.length; i++) {
        const user = LPs[i];
        lpTokenBalances[user] = [];
        let amountA, amountB;

        // Get current reserves from DEX
        const reserveA = BigInt(await dex.methods.get_reserveA().call());
        const reserveB = BigInt(await dex.methods.get_reserveB().call());
        if ((reserveA === 0n) && (reserveB === 0n)) {
            // First LP can choose arbitrary values
            let max_amountA = BigInt(await tokenA.methods.balanceOf(user).call({from: user})) / 10n;
            amountA = BigInt(Math.floor(Number(max_amountA) * Math.random()));

            let max_amountB = BigInt(await tokenB.methods.balanceOf(user).call({from: user})) / 10n;
            amountB = BigInt(Math.floor(Number(max_amountB) * Math.random()));
        } else {
            // Choose an amountA and calculate amountB according to the ratio
            let max_amountA = BigInt(await tokenA.methods.balanceOf(user).call({from: user})) / 10n;
            amountA = BigInt(Math.floor(Number(max_amountA) * Math.random()));
            amountB = (amountA * reserveB) / reserveA;
        }

        await tokenA.methods.approve(dexAddress, amountA.toString()).send({ from: user });
        await tokenB.methods.approve(dexAddress, amountB.toString()).send({ from: user });

        await dex.methods.addLiquidity(amountA.toString(), amountB.toString()).send({ from: user });
        console.log(`\u2705 ${user} added liquidity: A=${Number(amountA) / 1e18}, B=${Number(amountB) / 1e18}`);

        let lp_bal = BigInt(await dex.methods.get_lp_tokens(user).call());
        lpTokenBalances[user].push(Number(lp_bal)/1e18);

        amountA = BigInt(await tokenA.methods.balanceOf(user).call());
        amountB = BigInt(await tokenB.methods.balanceOf(user).call());
        initialBalances[user] = { amountA: amountA, amountB: amountB };
    }
    // ---------------------- Simulation Loop -------------------------
    console.log("Starting random simulation...");

    let sp = await dex.methods.spotPrice().call();
    spotPrices.push(Number(sp)/1e18);
    let resA = await dex.methods.get_reserveA().call();
    let resB = await dex.methods.get_reserveB().call();
    total_reservesA.push(Number(resA)/1e18);
    total_reservesB.push(Number(resB)/1e18);

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
                    const maxwithdrawAmount = lpBal / 10n;
                    const withdrawAmount = BigInt(Math.floor(Number(maxwithdrawAmount) * Math.random()));
                    await dex.methods.removeLiquidity(withdrawAmount.toString()).send({ from: user });
                    console.log(`\u{1F504} LP ${user} removed liquidity: ${Number(withdrawAmount) / 1e18} LPT`);
                }
            } else {
                // Trader swap
                const tokenChoice = Math.random() < 0.5 ? 'A' : 'B';
                const reserveA = BigInt(await dex.methods.get_reserveA().call());
                const reserveB = BigInt(await dex.methods.get_reserveB().call());
                let slip,token1,token2,ratio,tlf;

                if (tokenChoice === 'A') {
                    const balance = BigInt(await tokenA.methods.balanceOf(user).call());
                    const max_amount = balance < (reserveA / 10n) ? balance : (reserveA / 10n);
                    const amount = BigInt(Math.floor(Number(max_amount) * Math.random()));
                    token1 = Number(amount);
                    ratio = Number(reserveB) / Number(reserveA);
                    let ini = BigInt(await tokenB.methods.balanceOf(user).call());
                    tlf = Number(amount)/Number(reserveA);

                    if (amount > 0n) {
                        await tokenA.methods.approve(dexAddress, amount.toString()).send({ from: user });
                        await dex.methods.swap_A_to_B(amount.toString()).send({ from: user });
                        console.log(`\u{1F501} Trader ${user} swapped ${Number(amount) / 1e18} A for B`);
                    }
                    let fini = BigInt(await tokenB.methods.balanceOf(user).call());
                    token2 = Number(fini - ini);
                } else {
                    const balance = BigInt(await tokenB.methods.balanceOf(user).call());
                    const max_amount = balance < (reserveB / 10n) ? balance : (reserveB / 10n);
                    const amount = BigInt(Math.floor(Number(max_amount) * Math.random()));
                    token1 = Number(amount);
                    ratio = Number(reserveA)/Number(reserveB);
                    let ini = BigInt(await tokenA.methods.balanceOf(user).call());
                    tlf = Number(amount)/Number(reserveB);

                    if (amount > 0n) {
                        await tokenB.methods.approve(dexAddress, amount.toString()).send({ from: user });
                        await dex.methods.swap_B_to_A(amount.toString()).send({ from: user });
                        console.log(`\u{1F501} Trader ${user} swapped ${Number(amount) / 1e18} B for A`);
                    }
                    let fini = BigInt(await tokenA.methods.balanceOf(user).call());
                    token2 = Number(fini - ini);
                }

                slip = ((token2/token1) - ratio)*100/ratio;
                slippages.push(slip);

                trade_lot_fractions.push(tlf);
            }

            sp = await dex.methods.spotPrice().call();
            spotPrices.push(Number(sp)/1e18);
            resA = await dex.methods.get_reserveA().call();
            resB = await dex.methods.get_reserveB().call();
            total_reservesA.push(Number(resA)/1e18);
            total_reservesB.push(Number(resB)/1e18);

            for (const lp of LPs) {
                const lpBal = BigInt(await dex.methods.get_lp_tokens(lp).call());
                lpTokenBalances[lp].push(Number(lpBal) / 1e18); // Save in human-readable format
            }

        }catch(err){
            console.error(`Error at i = ${i}, ${err.message}`);
        }
    }

    console.log("\n\u{1F4B8} LPs removing all liquidity to realize fees...");
    const finalWithdrawals = {}; // user => { amountA, amountB }

    for (const user of LPs) {
        const lpBal = BigInt(await dex.methods.get_lp_tokens(user).call());
        if (lpBal > 0n) {
            // Remove all liquidity
            await dex.methods.removeLiquidity(lpBal.toString()).send({ from: user });
            console.log(`\u{1F4DC} ${user} removed ${Number(lpBal) / 1e18} LPT`);

            const finalA = BigInt(await tokenA.methods.balanceOf(user).call());
            const finalB = BigInt(await tokenB.methods.balanceOf(user).call());

            finalWithdrawals[user] = { amountA: finalA, amountB: finalB };
        }
    }

    console.log("\n--- \u{1F4B0} LP Earnings from Fees ---");
    for (const user of LPs) {
        try{
            const initial = initialBalances[user];
            const final = finalWithdrawals[user];

            if (!initial || !final) continue;

            let earnedA = final.amountA - initial.amountA;
            let earnedB = final.amountB - initial.amountB;

            // since owner initially minted money to all therefore he will be showing -26000
            // so just ignoring that 
            if (user === owner){
                earnedA += BigInt(26e21);
                earnedB += BigInt(26e21);
            }

            console.log(`\u{1F464} LP ${user}`);
            console.log(`   • TokenA Earned in Fees: ${Number(earnedA) / 1e18}`);
            console.log(`   • TokenB Earned in Fees: ${Number(earnedB) / 1e18}`);
        }catch(err){
            console.error(`${user}, ${err.message}`);
        }
    }


    // ---------------------- Final Metrics -------------------------
    console.log("Fetching DEX stats...");

    const spotPricesText = spotPrices.join('\n');
    await remix.call('fileManager', 'writeFile', 'browser/spot_prices.txt', spotPricesText);
    console.log("\u{1F4C4} Spot prices written to browser/spot_prices.txt");

    const total_reservesAText = total_reservesA.join('\n');
    await remix.call('fileManager', 'writeFile', 'browser/total_reservesA.txt', total_reservesAText);
    console.log("\u{1F4C4} Spot prices written to browser/total_reservesA.txt");

    const total_reservesBText = total_reservesB.join('\n');
    await remix.call('fileManager', 'writeFile', 'browser/total_reservesB.txt', total_reservesBText);
    console.log("\u{1F4C4} Spot prices written to browser/total_reservesB.txt");

    const slippagesText = slippages.join('\n');
    await remix.call('fileManager', 'writeFile', 'browser/slippages.txt', slippagesText);
    console.log("\u{1F4C4} Spot prices written to browser/slippages.txt");

    const trade_lot_fractionsText = trade_lot_fractions.join('\n');
    await remix.call('fileManager', 'writeFile', 'browser/trade_lot_fractions.txt', trade_lot_fractionsText);
    console.log("\u{1F4C4} Spot prices written to browser/trade_lot_fractions.txt");

    let lpTokenBalanceText = "";
    for (const lp of LPs) {
        lpTokenBalanceText += `LP ${lp}:\n` + lpTokenBalances[lp].join(', ') + "\n\n";
    }
    await remix.call('fileManager', 'writeFile', 'browser/lp_token_balances.txt', lpTokenBalanceText);
    console.log("\u{1F4C4} LP token balances written to browser/lp_token_balances.txt");

    
    const tot_A = await dex.methods.get_tot_feeA().call({ from: owner });
    const tot_B = await dex.methods.get_tot_feeB().call({ from: owner });

    const swap_A = await dex.methods.get_swapA().call({ from: owner });
    const swap_B = await dex.methods.get_swapB().call({ from: owner });
    
    console.log("Total fees of A collected: ",Number(tot_A)/1e18);
    console.log("Total fees of B collected: ",Number(tot_B)/1e18);

    console.log("Total tokens of A swapped: ",Number(swap_A)/1e18);
    console.log("Total tokens of B swapped: ",Number(swap_B)/1e18);

    console.log("\u2705 Simulation complete.");
}

simulateDEX();

