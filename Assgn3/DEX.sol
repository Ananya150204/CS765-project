// SPDX-License-Identifier: GPL-3.0

pragma solidity >=0.7.0 <0.9.0;

import "@openzeppelin/contracts/token/ERC20/ERC20.sol";
import "contracts/LPToken.sol";
import "contracts/Token.sol";

// every transaction occurs in terms of mini-tokens, so all the balances outputted are original and in units of mini tokens
// only the reserve ratio is scaled
contract DEX {
    IERC20 private tokenA;
    IERC20 private tokenB;
    LPToken private lpToken;

    uint256 private reserveA;               // reserves of A at any point
    uint256 private reserveB;               // reserves of B at any point
    address private owner;

    uint256 private cumulativeFeePerLPTokenA;
    uint256 private cumulativeFeePerLPTokenB;
    mapping(address => uint256) private userLastCumulativeFeeA;                 // how much fees has the LP last seen
    mapping(address => uint256) private userLastCumulativeFeeB;
    mapping(address => uint256) private earningsA;
    mapping(address => uint256) private earningsB;

    uint256 private constant SCALE = 1e18;

    constructor(address _tokA, address _tokB) {
        tokenA = IERC20(_tokA);
        tokenB = IERC20(_tokB);

        lpToken = new LPToken(address(this));
        reserveA = 0;           // reserve stores in terms of mini-tokens
        reserveB = 0;
        owner = address(0);
    }

    // function to update LP's fee earnings
    function updateEarnings(address user) internal {
        uint256 userBalance = lpToken.balanceOf(user);
        uint256 deltaA = cumulativeFeePerLPTokenA - userLastCumulativeFeeA[user];
        uint256 deltaB = cumulativeFeePerLPTokenB - userLastCumulativeFeeB[user];

        earningsA[user] += (userBalance * deltaA) / SCALE;
        earningsB[user] += (userBalance * deltaB) / SCALE;

        userLastCumulativeFeeA[user] = cumulativeFeePerLPTokenA;
        userLastCumulativeFeeB[user] = cumulativeFeePerLPTokenB;
    }

    // liquidity gets added in terms of mini-tokens, also lp tokens are minted and transferred in mini-tokens
    function addLiquidity(uint256 amountA, uint256 amountB) external  {
        require(amountA > 0 && amountB > 0, "Zero amounts in add liquidity");
        require(tokenA.balanceOf(msg.sender) >= amountA, "Insufficient token A balance");
        require(tokenB.balanceOf(msg.sender) >= amountB, "Insufficient token B balance");
        if(owner == address(0)) {
            owner = msg.sender;
            lpToken.mint(owner, 5e12);     // giving 5e12 mini-LP tokens to owner initially
        }

        if (reserveA == 0 && reserveB == 0) {
            // First LP sets the ratio
            require(tokenA.transferFrom(msg.sender, address(this), amountA),"Approve the liquidity of A");
            require(tokenB.transferFrom(msg.sender, address(this), amountB),"Approve the liquidity of B");

            updateEarnings(msg.sender);
            uint256 lpAmount = SCALE;
            lpToken.mint(msg.sender, lpAmount);
        } else {
            // Maintain ratio: amountA / amountB = reserveA / reserveB
            uint256 a = (reserveA * SCALE) / reserveB;
            uint256 b = (amountA * SCALE) / amountB;
            require(
                a > b ? (a-b <= 100) : (b-a <= 100),          // Allowing 16th decimal place to be different
                "Invalid deposit ratio"
            );

            require(tokenA.transferFrom(msg.sender, address(this), amountA),"Approve the liquidity of A");
            require(tokenB.transferFrom(msg.sender, address(this), amountB),"Approve the liquidity of B");

            updateEarnings(msg.sender);
            uint256 totalSupply = lpToken.totalSupply();
            uint256 lpAmount = (totalSupply * amountA) / reserveA;
            lpToken.mint(msg.sender, lpAmount);
        }

        reserveA += amountA;
        reserveB += amountB;
    }

    function removeLiquidity(uint256 lpAmount) external  {
        require(lpAmount > 0, "Zero amount in remove liquidity");
        uint256 totalSupply = lpToken.totalSupply();
        require(totalSupply > 0, "No LP tokens");
        require(lpToken.balanceOf(msg.sender) >= lpAmount, "Insufficient LP balance");

        uint256 amountA = (lpAmount * reserveA) / totalSupply;
        uint256 amountB = (lpAmount * reserveB) / totalSupply;

        reserveA -= amountA;
        reserveB -= amountB;

        updateEarnings(msg.sender); 
        uint256 userBalance = lpToken.balanceOf(msg.sender);                        // includes lpAmount
        uint256 shareA = (lpAmount * earningsA[msg.sender]) / userBalance;
        uint256 shareB = (lpAmount * earningsB[msg.sender]) / userBalance;
        earningsA[msg.sender] -= shareA;
        earningsB[msg.sender] -= shareB;

        lpToken.burn(msg.sender, lpAmount);

        tokenA.transfer(msg.sender, amountA + shareA);
        tokenB.transfer(msg.sender, amountB + shareB);
    }

    function swap_A_to_B(uint256 amt) external{
        require(amt > 0, "Enter non-zero amount in swap_A_to_B");
        require(tokenA.balanceOf(msg.sender) >= amt, "Insufficient token A balance");
        uint256 eff_A = (997*amt)/1000;
        uint256 amt_B = (eff_A * reserveB) / (reserveA + eff_A);
        require(tokenB.balanceOf(address(this)) >= amt_B, "Insufficient token B reserves");

        require(tokenA.transferFrom(msg.sender, address(this), amt),"Approve the swap from A");
        tokenB.transfer(msg.sender, amt_B);

        reserveA += eff_A;
        reserveB -= amt_B;

        uint256 totalSupply = lpToken.totalSupply();
        if (totalSupply > 0) {
            cumulativeFeePerLPTokenA += ((amt - eff_A) * SCALE) / totalSupply;
        }
    }

    function swap_B_to_A(uint256 amt) external{
        require(amt > 0, "Enter non-zero amount in swap_B_to_A");
        require(tokenB.balanceOf(msg.sender) >= amt, "Insufficient token B balance");
        uint256 eff_B = (997*amt)/1000;  
        uint256 amt_A = (eff_B * reserveA) / (reserveB + eff_B);
        require(tokenA.balanceOf(address(this)) >= amt_A, "Insufficient token A reserves");

        require(tokenB.transferFrom(msg.sender, address(this), amt),"Approve the swap from B");
        tokenA.transfer(msg.sender, amt_A);

        reserveA -= amt_A;
        reserveB += eff_B;

        uint256 totalSupply = lpToken.totalSupply();
        if (totalSupply > 0) {
            cumulativeFeePerLPTokenB += ((amt - eff_B) * SCALE) / totalSupply;
        }
    }

    function get_B_to_A(uint256 amt_B) external view returns (uint256){
        return (reserveA * amt_B)/reserveB;
    }

    function get_A_to_B(uint256 amt_A) external view returns (uint256){
        return (reserveB * amt_A)/reserveA;
    }

    function spotPrice() external view returns (uint256){
        return (reserveA * SCALE)/reserveB;
    }

    function get_reserveA() external view returns (uint256){
        return reserveA;
    } 

    function get_reserveB() external view returns (uint256){
        return reserveB;
    } 

    function get_lp_tokens(address t) external view returns (uint256){
        return lpToken.balanceOf(t);
    }

    function get_tokenA() external view returns (IERC20){
        return tokenA;
    }

    function get_tokenB() external view returns (IERC20){
        return tokenB;
    }
}

