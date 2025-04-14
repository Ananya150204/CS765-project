async function simulateArbitrage() {
    console.log("\u{1F680} Starting Arbitrage Simulation...");

    const accounts = await web3.eth.getAccounts();
    const arbitrageur = accounts[0];

    // --------- Load ABIs and Metadata ----------
    const tokenMetadata = JSON.parse(await remix.call('fileManager', 'getFile', 'browser/contracts/artifacts/Token.json'));
    const dexMetadata = JSON.parse(await remix.call('fileManager', 'getFile', 'browser/contracts/artifacts/DEX.json'));
    const arbMetadata = JSON.parse(await remix.call('fileManager', 'getFile', 'browser/contracts/artifacts/Arbitrage.json'));

    const tokenABI = tokenMetadata.abi;
    const dexABI = dexMetadata.abi;
    const arbABI = arbMetadata.abi;

    const SCALE = BigInt(1e18);

    // --------- Deploy Tokens ------------
    console.log(`Deploying Token A`);
    const tokenA = await new web3.eth.Contract(tokenABI).deploy({
        data: tokenMetadata.data.bytecode.object,
        arguments: [(BigInt(100000) * SCALE).toString()]
    }).send({ from: arbitrageur, gas: 60000000 });

    console.log(`Deploying Token B`);
    const tokenB = await new web3.eth.Contract(tokenABI).deploy({
        data: tokenMetadata.data.bytecode.object,
        arguments: [(BigInt(100000) * SCALE).toString()]
    }).send({ from: arbitrageur, gas: 60000000 });

    // --------- Deploy 2 DEXes ------------
    console.log(`Deploying DEX 1`);
    const dex1 = await new web3.eth.Contract(dexABI).deploy({
        data: dexMetadata.data.bytecode.object,
        arguments: [tokenA.options.address, tokenB.options.address]
    }).send({ from: arbitrageur, gas: 70000000 });

    console.log(`Deploying DEX 2`);
    const dex2 = await new web3.eth.Contract(dexABI).deploy({
        data: dexMetadata.data.bytecode.object,
        arguments: [tokenA.options.address, tokenB.options.address]
    }).send({ from: arbitrageur, gas: 70000000 });

    const dex1Address = dex1.options.address;
    const dex2Address = dex2.options.address;

    // --------- Add Liquidity ------------
    await tokenA.methods.approve(dex1Address, (1000n * SCALE).toString()).send({ from: arbitrageur });
    await tokenB.methods.approve(dex1Address, (2000n * SCALE).toString()).send({ from: arbitrageur });
    await dex1.methods.addLiquidity((1000n * SCALE).toString(), (2000n * SCALE).toString()).send({ from: arbitrageur });

    await tokenA.methods.approve(dex2Address, (1000n * SCALE).toString()).send({ from: arbitrageur });
    await tokenB.methods.approve(dex2Address, (3000n * SCALE).toString()).send({ from: arbitrageur });
    await dex2.methods.addLiquidity((1000n * SCALE).toString(), (3000n * SCALE).toString()).send({ from: arbitrageur });

    console.log("\u{1F4A7} Liquidity added to DEX1 and DEX2");

    // --------- Deploy Arbitrage Contract ------------
    console.log(`Deploying Arbitrage`);
    const arbitrage = await new web3.eth.Contract(arbABI).deploy({
        data: arbMetadata.data.bytecode.object,
        arguments: [dex1Address, dex2Address]
    }).send({ from: arbitrageur, gas: 7000000 });
    const arbAddress = arbitrage.options.address;


    // ----------------- Scenario 1: Profitable Arbitrage -----------------------
    console.log("\n\u{1F501} Trying Profitable Arbitrage: ");

    // Print reserves before arbitrage
    let resA1 = await dex1.methods.get_reserveA().call();
    let resB1 = await dex1.methods.get_reserveB().call();
    let resA2 = await dex2.methods.get_reserveA().call();
    let resB2 = await dex2.methods.get_reserveB().call();

    console.log(`\u{1F4CA} DEX1 Reserves - TokenA: ${resA1}, TokenB: ${resB1}`);
    console.log(`\u{1F4CA} DEX2 Reserves - TokenA: ${resA2}, TokenB: ${resB2}`);

    let initialBalanceA = BigInt(await tokenA.methods.balanceOf(arbitrageur).call());
    let initialBalanceB = BigInt(await tokenB.methods.balanceOf(arbitrageur).call());

    await tokenA.methods.approve(arbAddress, (initialBalanceA / 10n).toString()).send({ from: arbitrageur });
    console.log(`\u{1F4E5} Approved ${initialBalanceA / 10n} TokenA to Arbitrage Contract`);
    await tokenB.methods.approve(arbAddress, (initialBalanceB / 10n).toString()).send({ from: arbitrageur });
    console.log(`\u{1F4E5} Approved ${initialBalanceB / 10n} TokenB to Arbitrage Contract`);

    try{
        await arbitrage.methods.perform_arbitrage(
            (10n * SCALE).toString(),
            (initialBalanceB / 10n).toString(),
            (5n * BigInt(1e16)).toString()
        ).send({ from: arbitrageur, gas:70000000 });
    } catch(err){
        console.error(`Error in perform_arbitrage ${err.message}, ${initialBalanceA/10n}, ${initialBalanceB/10n}`);
    }

    let finalBalanceA = BigInt(await tokenA.methods.balanceOf(arbitrageur).call());
    let finalBalanceB = BigInt(await tokenB.methods.balanceOf(arbitrageur).call());
    let profitA = finalBalanceA - initialBalanceA;
    let profitB = finalBalanceB - initialBalanceB;

    console.log(`\u{1F4B8} Profit in A realized from arbitrage: ${Number(profitA)/1e18}`);
    console.log(`\u{1F4B8} Profit in B realized from arbitrage: ${Number(profitB)/1e18}`);

    // ----------------- Scenario 2: No Arbitrage -----------------------
    console.log("\n\u{1F501} Trying Non-Profitable Arbitrage: ");

    // Remove all liquidity from DEX2 to reset its ratio
    let lpBalance = await dex1.methods.get_lp_tokens(arbitrageur).call();
    await dex1.methods.removeLiquidity(lpBalance).send({ from: arbitrageur });
    console.log("\u{1F504} Removed all liquidity from DEX1");

    await tokenA.methods.approve(dex1Address, (1000n * SCALE).toString()).send({ from: arbitrageur });
    await tokenB.methods.approve(dex1Address, (2000n * SCALE).toString()).send({ from: arbitrageur });
    await dex1.methods.addLiquidity((1000n * SCALE).toString(), (2000n * SCALE).toString()).send({ from: arbitrageur });
    console.log("\u{1F4A7} Reset DEX1 with new liquidity ratio");

    // Remove all liquidity from DEX2 to reset its ratio
    lpBalance = await dex2.methods.get_lp_tokens(arbitrageur).call();
    await dex2.methods.removeLiquidity(lpBalance).send({ from: arbitrageur });
    console.log("\u{1F504} Removed all liquidity from DEX2");

    await tokenA.methods.approve(dex2Address, (1000n * SCALE).toString()).send({ from: arbitrageur });
    await tokenB.methods.approve(dex2Address, (2053n * SCALE).toString()).send({ from: arbitrageur });
    await dex2.methods.addLiquidity((1000n * SCALE).toString(), (2053n * SCALE).toString()).send({ from: arbitrageur });
    console.log("\u{1F4A7} Reset DEX2 with new liquidity ratio");

    // Print reserves before arbitrage
    resA1 = await dex1.methods.get_reserveA().call();
    resB1 = await dex1.methods.get_reserveB().call();
    resA2 = await dex2.methods.get_reserveA().call();
    resB2 = await dex2.methods.get_reserveB().call();

    console.log(`\u{1F4CA} DEX1 Reserves - TokenA: ${resA1}, TokenB: ${resB1}`);
    console.log(`\u{1F4CA} DEX2 Reserves - TokenA: ${resA2}, TokenB: ${resB2}`);

    initialBalanceA = BigInt(await tokenA.methods.balanceOf(arbitrageur).call());
    initialBalanceB = BigInt(await tokenB.methods.balanceOf(arbitrageur).call());

    await tokenA.methods.approve(arbAddress, (initialBalanceA / 10n).toString()).send({ from: arbitrageur });
    console.log("\u{1F4E5} Approved TokenA to Arbitrage Contract");
    await tokenB.methods.approve(arbAddress, (initialBalanceB / 10n).toString()).send({ from: arbitrageur });
    console.log("\u{1F4E5} Approved TokenB to Arbitrage Contract");

    try{
        await arbitrage.methods.perform_arbitrage(
            (10n * SCALE).toString(),
            (initialBalanceB / 10n).toString(),
            (5n * BigInt(1e16)).toString()
        ).send({ from: arbitrageur });
    }catch(err){
        console.error(`Error in perform_arbitrage ${err.message}`);
    }

    finalBalanceA = BigInt(await tokenA.methods.balanceOf(arbitrageur).call());
    finalBalanceB = BigInt(await tokenB.methods.balanceOf(arbitrageur).call());
    profitA = finalBalanceA - initialBalanceA;
    profitB = finalBalanceB - initialBalanceB;

    console.log(`\u{1F4B8} Profit in A realized from arbitrage: ${Number(profitA)/1e18}`);
    console.log(`\u{1F4B8} Profit in B realized from arbitrage: ${Number(profitB)/1e18}`);
}

simulateArbitrage();

