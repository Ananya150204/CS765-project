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
    address internal dex1Address;
    address internal dex2Address;

    constructor(address _dex1, address _dex2) {
        dex1 = IDex(_dex1);
        dex2 = IDex(_dex2);
        tokenA = dex1.get_tokenA();
        tokenB = dex1.get_tokenB();
        owner = msg.sender;
        dex1Address = _dex1;
        dex2Address = _dex2;
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

    function perform_arbitrage(uint256 amt_A, uint256 amt_B, uint256 threshold_percent_scaled) external{
        require(msg.sender == owner, "Only owner can call");
        require(dex1.spotPrice() != dex2.spotPrice(), "Reserve ratios are the same");
        require(amt_A > 0, "Zero amount of token A");
        require(amt_B > 0, "Zero amount of token B");
        require(tokenA.balanceOf(msg.sender) >= amt_A, "Insufficient token A balance");
        require(tokenB.balanceOf(msg.sender) >= amt_B, "Insufficient token B balance");

        // wherever A->B->A arbitrage is possible there B->A->B is also possible in the reverse direction
        // so assume A->B->A happens left to right
        // B->A->B happens right to left

        IDex left = IDex(dex2Address);
        IDex right = IDex(dex1Address);
        if(dex1.spotPrice() < dex2.spotPrice()){
            right = IDex(dex2Address);
            left = IDex(dex1Address);
        }

        // A->B->A
        uint256 intermediate_price = get_swap_A_to_B(amt_A, left.get_reserveA(), left.get_reserveB());
        uint256 final_price = get_swap_B_to_A(intermediate_price, right.get_reserveA(), right.get_reserveB());
        uint256 profit_A_B_A = (final_price >= amt_A) ? (final_price - amt_A) : 0;

        // B->A->B
        intermediate_price = get_swap_B_to_A(amt_B, right.get_reserveA(), right.get_reserveB());
        final_price = get_swap_A_to_B(intermediate_price, left.get_reserveA(), left.get_reserveB());
        uint256 profit_B_A_B = (final_price >= amt_B) ? (final_price - amt_B) : 0;

        if(profit_A_B_A >= (threshold_percent_scaled * amt_A)/ 1e20){
            require(tokenA.transferFrom(msg.sender, address(this), amt_A), "Transfer failed");

            tokenA.approve(address(left),amt_A);
            left.swap_A_to_B(amt_A);
            intermediate_price = tokenB.balanceOf(address(this));
            tokenB.approve(address(right),intermediate_price);
            right.swap_B_to_A(intermediate_price);   
            intermediate_price = tokenA.balanceOf(address(this));
            tokenA.transfer(owner, intermediate_price); // Send back capital + profit
        }
        else if(profit_B_A_B >= (threshold_percent_scaled * amt_B)/ 1e20){
            require(tokenB.transferFrom(msg.sender, address(this), amt_B), "Transfer failed");

            tokenB.approve(address(right),amt_B);
            right.swap_B_to_A(amt_B);
            intermediate_price = tokenA.balanceOf(address(this));
            tokenA.approve(address(left),intermediate_price);
            left.swap_A_to_B(intermediate_price);   
            intermediate_price = tokenB.balanceOf(address(this));
            tokenB.transfer(owner, intermediate_price); // Send back capital + profit
        }
    }
}

