async function calculate_fees(tokenA, tokenB, dex, dexAddress){
    let totA = await tokenA.methods.balanceOf(dexAddress).call();
    let resA = await dex.methods.get_reserveA().call();

    let totB = await tokenB.methods.balanceOf(dexAddress).call();
    let resB = await dex.methods.get_reserveB().call();
    return {amountA: totA - resA, amountB: totB - resB};
}

async function simulateDEX() {
    console.log("Starting DEX Simulation...");

    const accounts = await web3.eth.getAccounts();
    const owner = accounts[0];
    const LPs = [owner, ...accounts.slice(1, 5)];   // owner + 4 LPs (5 total LPs)
    const traders = accounts.slice(5, 13);          // 8 traders
    const spotPrices = [];                          // To store N+1 spot prices
    const TVL = [];                                 // N+1 vals
    const slippages = [];                           // To store <=N slippage values (only for swaps)
    const trade_lot_fractions = [];                 // To store <=N tlf values (only for swaps)
    const lpTokenBalances = {};                     // user => [array of lp token balances], N+1 values in each
    const earnings = {};                            // final earnings one value in each
    const swapped_A = [0];                          // N+1 values (even if swap doesn't happen it will stay same for that transaction)
    const swapped_B = [0];                          // N+1 values (even if swap doesn't happen it will stay same for that transaction)
    const fees_of_A = [0];                          // N+1 values (even if swap doesn't happen it will stay same for that transaction)    
    const fees_of_B = [0];                          // N+1 values (even if swap doesn't happen it will stay same for that transaction)    
    let tot_swap_A = 0n, tot_swap_B = 0n;           // Storing total tokens swapped at any point of time
    let tot_fee_A = 0n, tot_fee_B = 0n;             // Storing total fees collected by the DEX at any time

    const N = 100;                                  // Number of transactions to simulate
    const SCALE = BigInt(1e18);

    // ---------------------- Load ABIs -------------------------
    const tokenMetadata = JSON.parse(await remix.call('fileManager', 'getFile', 'browser/contracts/artifacts/Token.json'));
    const dexMetadata = JSON.parse(await remix.call('fileManager', 'getFile', 'browser/contracts/artifacts/DEX.json'));

    const tokenABI = tokenMetadata.abi;
    const dexABI = dexMetadata.abi;

    // --------------------- Deploying Contracts --------------------------------
    const tokenAFactory = new web3.eth.Contract(tokenABI);
    const tokenBFactory = new web3.eth.Contract(tokenABI);
    const dexFactory = new web3.eth.Contract(dexABI);

    const tokenA = await tokenAFactory.deploy({
        data: tokenMetadata.data.bytecode.object,
        arguments: [(BigInt(400000) * SCALE).toString()]
    }).send({ from: owner, gas: 50000000 });
    
    const tokenB = await tokenBFactory.deploy({
        data: tokenMetadata.data.bytecode.object,
        arguments: [(BigInt(400000) * SCALE).toString()]
    }).send({ from: owner, gas: 50000000 });

    const dex = await dexFactory.deploy({
        data: dexMetadata.data.bytecode.object,
        arguments: [tokenA.options.address.toString(), tokenB.options.address.toString()]
    }).send({ from: owner, gas: 50000000 });

    const dexAddress = dex.options.address.toString();

    // ---------------------- Deployed Contract Addresses -------------------------
    // const tokenAAddress = "0x81f6D0417aCCaFEabf107Bd28c078702C44D7863";
    // const tokenBAddress = "0xd3bB49346E4C34B9dB2841706A88B477A9b5F720";
    // const dexAddress = "0xfc79Aa9DfabF722A72643d5597a8911e481bAd08";

    // const tokenA = new web3.eth.Contract(tokenABI, tokenAAddress);
    // const tokenB = new web3.eth.Contract(tokenABI, tokenBAddress);
    // const dex = new web3.eth.Contract(dexABI, dexAddress);

    console.log("Loaded contracts successfully.");

    // ---------------------- Mint Tokens to Users -------------------------
    console.log("Minting tokens to LPs and Traders...");

    for (let user of [...LPs, ...traders]) {
        if(user === owner) continue;
        try {
            let max_amount = BigInt(await tokenA.methods.balanceOf(owner).call({from: owner})) / 20n;
            let amount = BigInt(Math.floor(Number(max_amount) * Math.random()));
            while(amount === 0n){
                amount = BigInt(Math.floor(Number(max_amount) * Math.random()));
            }

            await tokenA.methods.transfer(user, amount.toString()).send({ from: owner });
            console.log(`\u2705 ${amount} TokenA minted to ${user}`);
        } catch (err) {
            console.error(`\u274C TokenA transfer failed for ${user}: ${err.message}`);
        }

        try {
            let max_amount = BigInt(await tokenB.methods.balanceOf(owner).call({from: owner})) / 20n;
            let amount = BigInt(Math.floor(Number(max_amount) * Math.random()));
            while(amount === 0n){
                amount = BigInt(Math.floor(Number(max_amount) * Math.random()));
            }

            await tokenB.methods.transfer(user, amount.toString()).send({ from: owner });
            console.log(`\u2705 ${amount} TokenB minted to ${user}`);
        } catch (err) {
            console.error(`\u274C TokenB transfer failed for ${user}: ${err.message}`);
        }
    }

    // ---------------------- LPs Add Liquidity -------------------------
    console.log("LPs adding liquidity...");

    for (let i = 0; i < LPs.length; i++) {
        const user = LPs[i];
        earnings[user] = {amountA: 0, amountB: 0};
        lpTokenBalances[user] = [];
        let amountA, amountB;

        // Get current reserves from DEX
        const reserveA = BigInt(await dex.methods.get_reserveA().call());
        const reserveB = BigInt(await dex.methods.get_reserveB().call());

        if ((reserveA === 0n) && (reserveB === 0n)) {
            // First LP can choose arbitrary values
            let max_amountA = BigInt(await tokenA.methods.balanceOf(user).call({from: user})) / 10n;
            amountA = BigInt(Math.floor(Number(max_amountA) * Math.random()));
            while(amountA === 0n){
                amountA = BigInt(Math.floor(Number(max_amountA) * Math.random()));
            }

            let max_amountB = BigInt(await tokenB.methods.balanceOf(user).call({from: user})) / 10n;
            amountB = BigInt(Math.floor(Number(max_amountB) * Math.random()));
            while(amountB === 0n){
                amountB = BigInt(Math.floor(Number(max_amountB) * Math.random()));
            }

        } else {
            // Choose an amountA and calculate amountB according to the ratio
            let balA = BigInt(await tokenA.methods.balanceOf(user).call({from: user})) / 10n;
            let balB = BigInt(await tokenB.methods.balanceOf(user).call({from: user})) / 10n;

            amountA = BigInt(Math.floor(Number(balA) * Math.random()));
            while(amountA === 0n){
                amountA = BigInt(Math.floor(Number(balA) * Math.random()));
            }
            amountB = (amountA * reserveB) / reserveA;
            if(amountB >= 10n * balB){
                amountB = BigInt(Math.floor(Number(balB) * Math.random()));
                while(amountB === 0n){
                    amountB = BigInt(Math.floor(Number(balB) * Math.random()));
                }
                amountA = (amountB * reserveA) / reserveB;
            }
        }

        await tokenA.methods.approve(dexAddress, amountA.toString()).send({ from: user });
        await tokenB.methods.approve(dexAddress, amountB.toString()).send({ from: user });

        try{
            await dex.methods.addLiquidity(amountA.toString(), amountB.toString()).send({ from: user });
            console.log(`\u2705 LP ${user} added liquidity: A=${Number(amountA) / 1e18}, B=${Number(amountB) / 1e18}`);
        } catch(err){
            console.error(`${user}, ${balA * 10n}, ${amountA}, ${balB * 10n}, ${amountB}, ${reserveA} , ${reserveB}`);
        }

        let lp_bal = BigInt(await dex.methods.get_lp_tokens(user).call());
        lpTokenBalances[user].push(Number(lp_bal)/1e18);
    }
    // ---------------------- Simulation Loop -------------------------
    console.log("Starting random simulation...");

    let sp = await dex.methods.spotPrice().call();
    spotPrices.push(Number(sp)/1e18);
    let resA = await dex.methods.get_reserveA().call();
    TVL.push(Number(2*resA)/1e18);

    for (let i = 0; i < N; i++) {
        console.log(i);
        const isLPAction = Math.random() < 0.4;
        const userPool = isLPAction ? LPs : traders;
        const user = userPool[Math.floor(Math.random() * userPool.length)];
        const reserveA = BigInt(await dex.methods.get_reserveA().call());
        const reserveB = BigInt(await dex.methods.get_reserveB().call());
        try{
            if (isLPAction) {
                const is_withdraw = Math.random() < 0.5;
                if(is_withdraw){
                    // LP withdraws liquidity
                    const lpBal = BigInt(await dex.methods.get_lp_tokens(user).call());
                    if (lpBal > 0n) {
                        const maxwithdrawAmount = lpBal / 10n;
                        let withdrawAmount = BigInt(Math.floor(Number(maxwithdrawAmount) * Math.random()));
                        while(withdrawAmount === 0n){
                            withdrawAmount = BigInt(Math.floor(Number(maxwithdrawAmount) * Math.random()));
                        }

                        let f1 = await calculate_fees(tokenA,tokenB,dex,dexAddress);
                        await dex.methods.removeLiquidity(withdrawAmount.toString()).send({ from: user });
                        let f2 = await calculate_fees(tokenA,tokenB,dex,dexAddress);
                        earnings[user].amountA += f1.amountA - f2.amountA;
                        earnings[user].amountB += f1.amountB - f2.amountB;
                        console.log(`\u{1F504} LP ${user} removed liquidity: ${Number(withdrawAmount) / 1e18} LPT`);
                    }
                    else console.log(`lpBal 0 for ${i} user: ${user}`);
                }else{
                    // LP adds liquidity
                    // Choose an amountA and calculate amountB according to the ratio
                    let balA = BigInt(await tokenA.methods.balanceOf(user).call({from: user})) / 10n;
                    let balB = BigInt(await tokenB.methods.balanceOf(user).call({from: user})) / 10n;

                    let amountA = BigInt(Math.floor(Number(balA) * Math.random()));
                    while(amountA === 0n){
                        amountA = BigInt(Math.floor(Number(balA) * Math.random()));
                    }

                    let amountB = (amountA * reserveB) / reserveA;
                    if(amountB > 10n * balB){
                        amountB = BigInt(Math.floor(Number(balB) * Math.random()));
                        while(amountB === 0n){
                            amountB = BigInt(Math.floor(Number(balB) * Math.random()));
                        }
                        amountA = (amountB * reserveA) / reserveB;
                    }

                    await tokenA.methods.approve(dexAddress, amountA.toString()).send({ from: user });
                    await tokenB.methods.approve(dexAddress, amountB.toString()).send({ from: user });

                    try{
                        await dex.methods.addLiquidity(amountA.toString(), amountB.toString()).send({ from: user });
                        console.log(`\u2705 LP ${user} added liquidity: A=${Number(amountA) / 1e18}, B=${Number(amountB) / 1e18}`);
                    } catch(err){
                        console.error(`error in random add liquidity ${balA * 10n}, ${amountA}, ${balB * 10n}, ${amountB}, ${reserveA} , ${reserveB}`);
                    }
                }
            } else {
                // Trader swap
                const tokenChoice = Math.random() < 0.5 ? 'A' : 'B';
                let slip,token1,token2,ratio,tlf;

                if (tokenChoice === 'A') {
                    const balance = BigInt(await tokenA.methods.balanceOf(user).call());
                    const max_amount = balance < (reserveA / 10n) ? balance : (reserveA / 10n);
                    let amount = BigInt(Math.floor(Number(max_amount) * Math.random()));
                    while(amount === 0n){
                        amount = BigInt(Math.floor(Number(max_amount) * Math.random()));
                    }

                    tot_swap_A += amount;
                    tot_fee_A += (3n*amount)/1000n;
                    token1 = Number(amount);
                    ratio = Number(reserveB) / Number(reserveA);
                    let ini = BigInt(await tokenB.methods.balanceOf(user).call());
                    tlf = Number(amount)/Number(reserveA);

                    await tokenA.methods.approve(dexAddress, amount.toString()).send({ from: user });
                    await dex.methods.swap_A_to_B(amount.toString()).send({ from: user });
                    console.log(`\u{1F501} Trader ${user} swapped ${Number(amount) / 1e18} A for B`);

                    let fini = BigInt(await tokenB.methods.balanceOf(user).call());
                    token2 = Number(fini - ini);
                } else {
                    const balance = BigInt(await tokenB.methods.balanceOf(user).call());
                    const max_amount = balance < (reserveB / 10n) ? balance : (reserveB / 10n);
                    const amount = BigInt(Math.floor(Number(max_amount) * Math.random()));
                    while(amount === 0n){
                        amount = BigInt(Math.floor(Number(max_amount) * Math.random()));
                    }
                    tot_swap_B += amount;
                    tot_fee_B += (3n*amount)/1000n;
                    token1 = Number(amount);    
                    ratio = Number(reserveA)/Number(reserveB);
                    let ini = BigInt(await tokenA.methods.balanceOf(user).call());
                    tlf = Number(amount)/Number(reserveB);

                    await tokenB.methods.approve(dexAddress, amount.toString()).send({ from: user });
                    await dex.methods.swap_B_to_A(amount.toString()).send({ from: user });
                    console.log(`\u{1F501} Trader ${user} swapped ${Number(amount) / 1e18} B for A`);
                    

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
            TVL.push(Number(2*resA)/1e18);
            swapped_A.push(Number(tot_swap_A)/1e18);
            swapped_B.push(Number(tot_swap_B)/1e18);
            fees_of_A.push(Number(tot_fee_A)/1e18);
            fees_of_B.push(Number(tot_fee_B)/1e18);

            for (const lp of LPs) {
                const lpBal = BigInt(await dex.methods.get_lp_tokens(lp).call());
                lpTokenBalances[lp].push(Number(lpBal) / 1e18); // Save in human-readable format
            }

        }catch(err){
            console.error(`Error at i = ${i}, is_LP_action: ${isLPAction}, ${err.message}`);
        }
    }

    console.log("\n\u{1F4B8} LPs removing all liquidity to realize fees...");
    for (const user of LPs) {
        const lpBal = BigInt(await dex.methods.get_lp_tokens(user).call());
        if (lpBal > 0n) {
            // Remove all liquidity
            try{
                let f1 = await calculate_fees(tokenA,tokenB,dex,dexAddress);
                await dex.methods.removeLiquidity(lpBal.toString()).send({ from: user });
                let f2 = await calculate_fees(tokenA,tokenB,dex,dexAddress);
                console.log(`\u{1F4DC} ${user} removed ${Number(lpBal) / 1e18} LPT`);
                earnings[user].amountA += f1.amountA - f2.amountA;
                earnings[user].amountB += f1.amountB - f2.amountB;
            }catch(err){
                console.error(`${lpBal}`);
            }
        }
    }

    console.log("\n--- \u{1F4B0} LP Earnings from Fees ---");    
    for (const user of LPs) {
        try{
            let earnedA = earnings[user].amountA;
            let earnedB = earnings[user].amountB;

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

    const TVLText = TVL.join('\n');
    await remix.call('fileManager', 'writeFile', 'browser/TVL.txt', TVLText);
    console.log("\u{1F4C4} TVL written to browser/TVL.txt");

    const slippagesText = slippages.join('\n');
    await remix.call('fileManager', 'writeFile', 'browser/slippages.txt', slippagesText);
    console.log("\u{1F4C4} Slippages written to browser/slippages.txt");

    const trade_lot_fractionsText = trade_lot_fractions.join('\n');
    await remix.call('fileManager', 'writeFile', 'browser/trade_lot_fractions.txt', trade_lot_fractionsText);
    console.log("\u{1F4C4} Trade lot fractions written to browser/trade_lot_fractions.txt");

    let lpIndex = 1;
    for (const lp of LPs) {
        const balancesText = lpTokenBalances[lp].join('\n'); 
        const fileName = `browser/lp_${lpIndex}.txt`; 
        await remix.call('fileManager', 'writeFile', fileName, balancesText);
        console.log(`\u{1F4C4} LP ${lp} balances written to ${fileName}`);
        lpIndex++;
    }

    const swapped_AText = swapped_A.join('\n');
    await remix.call('fileManager', 'writeFile', 'browser/swapped_A.txt', swapped_AText);
    console.log("\u{1F4C4} Total tokens of A swapped written to browser/swapped_A.txt");

    const swapped_BText = swapped_B.join('\n');
    await remix.call('fileManager', 'writeFile', 'browser/swapped_B.txt', swapped_BText);
    console.log("\u{1F4C4} Total tokens of B swapped written to browser/swapped_B.txt");

    const fees_of_AText = fees_of_A.join('\n');
    await remix.call('fileManager', 'writeFile', 'browser/fees_of_A.txt', fees_of_AText);
    console.log("\u{1F4C4} Total fees collected in A written to browser/fees_of_A.txt");

    const fees_of_BText = fees_of_B.join('\n');
    await remix.call('fileManager', 'writeFile', 'browser/fees_of_B.txt', fees_of_BText);
    console.log("\u{1F4C4} Total fees collected in B written to browser/fees_of_B.txt");

    console.log(`JUST TO VERIFY THAT ALL TOKENS & RESERVES are now empty`);
    let _ = await dex.methods.get_reserveA().call();
    console.log(`RESERVES of A: ${Number(_)/1e18}`);

    _ = await dex.methods.get_reserveB().call();
    console.log(`RESERVES of B: ${Number(_)/1e18}`);

    _ = await tokenA.methods.balanceOf(dexAddress).call();
    console.log(`Tokens of A with DEX: ${Number(_)/1e18}`);

    _ = await tokenB.methods.balanceOf(dexAddress).call();
    console.log(`Tokens of B with DEX: ${Number(_)/1e18}`);

    console.log("\u2705 Simulation complete.");
}

simulateDEX();
