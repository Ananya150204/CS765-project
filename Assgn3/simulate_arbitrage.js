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
    const tokenA = await new web3.eth.Contract(tokenABI).deploy({
        data: tokenMetadata.data.bytecode.object,
        arguments: [(BigInt(1000000) * SCALE).toString()]
    }).send({ from: arbitrageur, gas: 60000000 });

    const tokenB = await new web3.eth.Contract(tokenABI).deploy({
        data: tokenMetadata.data.bytecode.object,
        arguments: [(BigInt(1000000) * SCALE).toString()]
    }).send({ from: arbitrageur, gas: 60000000 });

    // --------- Deploy 2 DEXes ------------
    const dex1 = await new web3.eth.Contract(dexABI).deploy({
        data: dexMetadata.data.bytecode.object,
        arguments: [tokenA.options.address, tokenB.options.address]
    }).send({ from: arbitrageur, gas: 70000000 });

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
    const arbitrage = await new web3.eth.Contract(arbABI).deploy({
        data: arbMetadata.data.bytecode.object,
        arguments: [dex1Address, dex2Address]
    }).send({ from: arbitrageur, gas: 70000000 });

    const arbAddress = arbitrage.options.address;

    // --------- Fund arbitrage contract ------------
    await tokenA.methods.approve(arbAddress, (10n * SCALE).toString()).send({ from: arbitrageur });
    console.log("\u{1F4E5} Approved 10 TokenA to Arbitrage Contract");

    // --------- Scenario 1: Profitable Arbitrage using TokenA ------------
    console.log("\n\u{1F501} Trying Profitable Arbitrage: TokenA as Input");

    // Print reserves before arbitrage
    let resA1 = await dex1.methods.get_reserveA().call();
    let resB1 = await dex1.methods.get_reserveB().call();
    let resA2 = await dex2.methods.get_reserveA().call();
    let resB2 = await dex2.methods.get_reserveB().call();

    console.log(`\u{1F4CA} DEX1 Reserves - TokenA: ${resA1}, TokenB: ${resB1}`);
    console.log(`\u{1F4CA} DEX2 Reserves - TokenA: ${resA2}, TokenB: ${resB2}`);

    const initialBalanceA = BigInt(await tokenA.methods.balanceOf(arbitrageur).call());

    try{
        await arbitrage.methods.performArbitrage_A(
            (10n * SCALE).toString(),
            (3n * SCALE)
        ).send({ from: arbitrageur });
    }catch(err){
        console.error(`Error in performArbitrage_A ${err.message}`);
    }

    const finalBalanceA = BigInt(await tokenA.methods.balanceOf(arbitrageur).call());
    const profitA = finalBalanceA - initialBalanceA;

    console.log("\u{2705} Arbitrage A->B->A executed");
    console.log(`\u{1F4B8} Profit realized from arbitrage: ${Number(profitA)/1e18}`);

    // --------- Scenario 2: No Arbitrage using TokenB ------------
    console.log("\n\u{1F501} Trying Non-Profitable Arbitrage: TokenB as Input");

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
    await tokenB.methods.approve(dex2Address, (2100n * SCALE).toString()).send({ from: arbitrageur });
    await dex2.methods.addLiquidity((1000n * SCALE).toString(), (2100n * SCALE).toString()).send({ from: arbitrageur });
    console.log("\u{1F4A7} Reset DEX2 with new liquidity ratio");


    // Approving the arbitrage contract to draw 10 tokenB
    await tokenA.methods.approve(arbAddress, (10n * SCALE).toString()).send({ from: arbitrageur });
    console.log("\u{1F4E5} Approved 10 TokenB to Arbitrage Contract");

    // Print reserves before arbitrage
    resA1 = await dex1.methods.get_reserveA().call();
    resB1 = await dex1.methods.get_reserveB().call();
    resA2 = await dex2.methods.get_reserveA().call();
    resB2 = await dex2.methods.get_reserveB().call();

    console.log(`\u{1F4CA} DEX1 Reserves - TokenA: ${resA1}, TokenB: ${resB1}`);
    console.log(`\u{1F4CA} DEX2 Reserves - TokenA: ${resA2}, TokenB: ${resB2}`);

    const initialBalanceB = BigInt(await tokenA.methods.balanceOf(arbitrageur).call());
    await arbitrage.methods.performArbitrage_A(
        (10n * SCALE).toString(),
        (3n * SCALE)
    ).send({ from: arbitrageur });

    const finalBalanceB = BigInt(await tokenA.methods.balanceOf(arbitrageur).call());
    const profitB = finalBalanceB - initialBalanceB;

    console.log(`\u{1F4B8} Profit realized from arbitrage: ${Number(profitB)/1e18}`);
}

simulateArbitrage();

