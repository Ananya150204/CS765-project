// SPDX-License-Identifier: GPL-3.0

pragma solidity >=0.7.0 <0.9.0;

import "@openzeppelin/contracts/token/ERC20/ERC20.sol";
import "contracts/LPToken.sol";
import "contracts/Token.sol";
import "contracts/DEX.sol";


interface IDex {
    function spotPrice() external view returns (uint256);
    function get_reserveA() external view returns(uint256);
    function get_reserveB() external view returns(uint256);
    function swap_A_to_B(uint256 amt) external;
    function swap_B_to_A(uint256 amt) external;
    function get_tokenA() external view returns (IERC20);
    function get_tokenB() external view returns (IERC20);
}

contract Arbitrage {
    IDex internal dex1;
    IDex internal dex2;

    IERC20 internal tokenA;
    IERC20 internal tokenB;

    address internal owner;

    constructor(address _dex1, address _dex2) {
        dex1 = IDex(_dex1);
        dex2 = IDex(_dex2);
        tokenA = dex1.get_tokenA();
        tokenB = dex1.get_tokenB();
        owner = msg.sender;
    }

    function get_swap_A_to_B(uint256 amt, uint256 reserveA, uint256 reserveB) internal pure returns (uint256){
        uint256 eff_A = (997*amt)/1000;        
        uint256 amt_B = (eff_A * reserveB) / (reserveA + eff_A);
        return amt_B;   
    }

    function get_swap_B_to_A(uint256 amt, uint256 reserveA, uint256 reserveB) internal pure returns (uint256){
        uint256 eff_B = (997*amt)/1000;        
        uint256 amt_A = (eff_B * reserveA) / (reserveB + eff_B);
        return amt_A;   
    }

    // owner gives token A to arbitrage
    function performArbitrage_A(uint256 amountIn, uint256 minProfit) external {
        require(msg.sender == owner, "Only owner can call");
        require(dex1.spotPrice() != dex2.spotPrice(), "Reserve ratios are the same");
        require(tokenA.balanceOf(msg.sender) >= amountIn, "Insufficient balance");
        
        // Transfer amountIn TokenA from user to this contract
        require(tokenA.transferFrom(msg.sender, address(this), amountIn), "Transfer failed");

        uint256 dex1_res_A = dex1.get_reserveA();
        uint256 dex1_res_B = dex1.get_reserveB();
        uint256 dex2_res_A = dex2.get_reserveA();
        uint256 dex2_res_B = dex2.get_reserveB();

        uint256 price_B_1 = get_swap_A_to_B(amountIn, dex1_res_A, dex1_res_B);
        uint256 price_A_2 = get_swap_B_to_A(price_B_1, dex2_res_A, dex2_res_B);

        uint256 price_B_2 = get_swap_A_to_B(amountIn, dex2_res_A, dex2_res_B);
        uint256 price_A_1 = get_swap_B_to_A(price_B_2, dex1_res_A, dex1_res_B);

        if(price_A_2 > amountIn + minProfit){
            tokenA.approve(address(dex1),amountIn);
            dex1.swap_A_to_B(amountIn);
            uint256 temp = tokenB.balanceOf(address(this));
            tokenB.approve(address(dex2),temp);
            dex2.swap_B_to_A(temp);   
            temp = tokenA.balanceOf(address(this));
            tokenA.transfer(owner, temp); // Send back capital + profit
        }
        else if(price_A_1 > amountIn + minProfit){
            tokenA.approve(address(dex2),amountIn);
            dex2.swap_A_to_B(amountIn);
            uint256 temp = tokenB.balanceOf(address(this));
            tokenB.approve(address(dex1),temp);
            dex1.swap_B_to_A(temp);
            temp = tokenA.balanceOf(address(this));   
            tokenA.transfer(owner, temp); // Send back capital + profit
        }
        else{
            tokenA.transfer(owner, amountIn);       // send back capital
        }
    }

    // owner gives token B to arbitrage
    function performArbitrage_B(uint256 amountIn, uint256 minProfit) external {
        require(msg.sender == owner, "Only owner can call");
        require(dex1.spotPrice() != dex2.spotPrice(), "Reserve ratios are the same");
        require(tokenB.balanceOf(msg.sender) >= amountIn, "Insufficient balance");
        
        // Transfer amountIn TokenA from user to this contract
        require(tokenB.transferFrom(msg.sender, address(this), amountIn), "Transfer failed");

        uint256 dex1_res_A = dex1.get_reserveA();
        uint256 dex1_res_B = dex1.get_reserveB();
        uint256 dex2_res_A = dex2.get_reserveA();
        uint256 dex2_res_B = dex2.get_reserveB();

        uint256 price_A_1 = get_swap_B_to_A(amountIn, dex1_res_A, dex1_res_B);
        uint256 price_B_2 = get_swap_A_to_B(price_A_1, dex2_res_A, dex2_res_B);

        uint256 price_A_2 = get_swap_B_to_A(amountIn, dex2_res_A, dex2_res_B);
        uint256 price_B_1 = get_swap_A_to_B(price_A_2, dex1_res_A, dex1_res_B);

        if(price_B_2 > amountIn + minProfit){
            tokenB.approve(address(dex1),amountIn);
            dex1.swap_B_to_A(amountIn);
            uint256 temp = tokenA.balanceOf(address(this));
            tokenA.approve(address(dex2),temp);
            dex2.swap_A_to_B(temp);   
            temp = tokenB.balanceOf(address(this));
            tokenB.transfer(owner, temp); // Send back capital + profit
        }
        else if(price_B_1 > amountIn + minProfit){
            tokenB.approve(address(dex2),amountIn);
            dex2.swap_B_to_A(amountIn);
            uint256 temp = tokenA.balanceOf(address(this));
            tokenA.approve(address(dex1),temp);
            dex1.swap_A_to_B(temp);   
            temp = tokenB.balanceOf(address(this));
            tokenB.transfer(owner, temp); // Send back capital + profit
        }
        else{
            tokenB.transfer(owner, amountIn);       // send back capital
        }
    }
}

