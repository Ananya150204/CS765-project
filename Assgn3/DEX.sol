// SPDX-License-Identifier: GPL-3.0

pragma solidity >=0.7.0 <0.9.0;

import "@openzeppelin/contracts/token/ERC20/ERC20.sol";
import "contracts/LPToken.sol";
import "contracts/Token.sol";


// every transaction occurs in terms of mini-tokens, so all the balances outputted are original and in units of mini tokens
// only the reserve ratio is scaled
contract DEX {
    IERC20 internal tokenA;
    IERC20 internal tokenB;
    LPToken internal lpToken;

    uint256 internal reserveA;
    uint256 internal reserveB;
    uint256 internal feeA;
    uint256 internal feeB;
    uint256 internal tot_feeA;
    uint256 internal tot_feeB;
    uint256 internal swap_A;
    uint256 internal swap_B;
    address internal owner;

    uint256 internal constant SCALE = 1e18;

    constructor(address _tokA, address _tokB) {
        tokenA = IERC20(_tokA);
        tokenB = IERC20(_tokB);

        lpToken = new LPToken(address(this));
        reserveA = 0;           // reserve stores in terms of mini-tokens
        reserveB = 0;
        feeA = 0;
        feeB = 0;
        tot_feeA = 0;
        tot_feeB = 0;
        swap_A = 0;
        swap_B = 0;
        owner = address(0);
    }

    // liquidity gets added in terms of mini-tokens, also lp tokens are minted and transferred in mini-tokens
    function addLiquidity(uint256 amountA, uint256 amountB) public {
        require(amountA > 0 && amountB > 0, "Zero amounts");
        require(tokenA.balanceOf(msg.sender) >= amountA, "Insufficient token A balance");
        require(tokenB.balanceOf(msg.sender) >= amountB, "Insufficient token B balance");
        if(owner == address(0)) {
            owner = msg.sender;
            lpToken.mint(owner, SCALE);     // giving 1 LP token to owner initially
        }

        if (reserveA == 0 && reserveB == 0) {
            // First LP sets the ratio
            tokenA.transferFrom(msg.sender, address(this), amountA);
            tokenB.transferFrom(msg.sender, address(this), amountB);

            uint256 lpAmount = SCALE;
            lpToken.mint(msg.sender, lpAmount);
        } else {
            // Maintain ratio: amountA / amountB = reserveA / reserveB
            uint256 a = (reserveA * SCALE) / reserveB;
            uint256 b = (amountA * SCALE) / amountB;
            require(
                a > b ? (a-b < 1) : (b-a < 1),
                "Invalid deposit ratio"
            );

            uint256 totalSupply = lpToken.totalSupply();
            uint256 lpAmount = (totalSupply * amountA) / reserveA;
            lpToken.mint(msg.sender, lpAmount);

            require(tokenA.transferFrom(msg.sender, address(this), amountA),"Approve the liquidity of A");
            require(tokenB.transferFrom(msg.sender, address(this), amountB),"Approve the liquidity of B");
        }

        reserveA += amountA;
        reserveB += amountB;
    }

    function removeLiquidity(uint256 lpAmount) public  {
        require(lpAmount > 0, "Zero amount");
        uint256 totalSupply = lpToken.totalSupply();
        require(totalSupply > 0, "No LP tokens");
        require(lpToken.balanceOf(msg.sender) >= lpAmount, "Insufficient LP balance");

        uint256 amountA = (lpAmount * reserveA) / totalSupply;
        uint256 amountB = (lpAmount * reserveB) / totalSupply;

        reserveA -= amountA;
        reserveB -= amountB;

        uint256 temp = (lpAmount * feeA) / totalSupply;
        amountA += temp;
        feeA -= temp;
        temp = (lpAmount * feeB) / totalSupply;
        amountB += temp;
        feeB -= temp;

        lpToken.burn(msg.sender, lpAmount);

        tokenA.transfer(msg.sender, amountA);
        tokenB.transfer(msg.sender, amountB);
    }

    function swap_A_to_B(uint256 amt) public{
        require(tokenA.balanceOf(msg.sender) >= amt, "Insufficient token A balance");
        swap_A += amt;

        uint256 eff_A = (997*amt)/1000;
        feeA = feeA + amt - eff_A;
        tot_feeA = tot_feeA + amt - eff_A;        
        uint256 amt_B = (eff_A * reserveB) / (reserveA + eff_A);
        require(tokenB.balanceOf(address(this)) >= amt_B, "Insufficient token B reserves");

        require(tokenA.transferFrom(msg.sender, address(this), amt),"Approve the swap from A");
        tokenB.transfer(msg.sender, amt_B);

        reserveA += eff_A;
        reserveB -= amt_B;
    }

    // return so that we know how much money actually got transferred so that arbitrageur can use it
    function swap_B_to_A(uint256 amt) public{
        require(tokenB.balanceOf(msg.sender) >= amt, "Insufficient token B balance");
        swap_B += amt;

        uint256 eff_B = (997*amt)/1000;  
        feeB = feeB + amt - eff_B;   
        tot_feeB = tot_feeB + amt - eff_B;   
        uint256 amt_A = (eff_B * reserveA) / (reserveB + eff_B);
        require(tokenA.balanceOf(address(this)) >= amt_A, "Insufficient token A reserves");

        require(tokenB.transferFrom(msg.sender, address(this), amt),"Approve the swap from B");
        tokenA.transfer(msg.sender, amt_A);

        reserveA -= amt_A;
        reserveB += eff_B;
    }

    function get_B_to_A(uint256 amt_B) public view returns (uint256){
        return (reserveA * amt_B)/reserveB;
    }

    function get_A_to_B(uint256 amt_A) public view returns (uint256){
        return (reserveB * amt_A)/reserveA;
    }

    function spotPrice() public view returns (uint256){
        return (reserveA * SCALE)/reserveB;
    }

    function get_reserveA() public view returns (uint256){
        return reserveA;
    } 

    function get_reserveB() public view returns (uint256){
        return reserveB;
    } 

    function get_lp_tokens(address t) public view returns (uint256){
        return lpToken.balanceOf(t);
    }

    function get_tokenA() public view returns (IERC20){
        return tokenA;
    }

    function get_tokenB() public view returns (IERC20){
        return tokenB;
    }

    function get_tot_feeA() external view returns (uint256){
        require(msg.sender == owner, "Only owner can view the fees of A");
        return tot_feeA;
    }

    function get_tot_feeB() external view returns (uint256){
        require(msg.sender == owner, "Only owner can view the fees of B");
        return tot_feeB;
    }

    function get_swapA() external view returns (uint256){
        require(msg.sender == owner, "Only owner can view the total swapped tokens of A");   
        return swap_A;
    }

    function get_swapB() external view returns (uint256){
        require(msg.sender == owner, "Only owner can view the total swapped tokens of B");   
        return swap_B;
    }
}

