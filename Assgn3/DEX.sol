// SPDX-License-Identifier: GPL-3.0

pragma solidity >=0.7.0 <0.9.0;

import "@openzeppelin/contracts/token/ERC20/ERC20.sol";
import "contracts/LPToken.sol";
import "contracts/Token.sol";

contract DEX {
    IERC20 public tokenA;
    IERC20 public tokenB;
    LPToken public lpToken;

    constructor(address _tokA, address _tokB) {
        tokenA = IERC20(_tokA);
        tokenB = IERC20(_tokB);
        lpToken = new LPToken(address(this));
    }

    function addLiquidity(uint256 amountA, uint256 amountB) public {
        require(amountA > 0 && amountB > 0, "Zero amounts");
        require(tokenA.balanceOf(msg.sender) >= amountA, "Insufficient token A balance");
        require(tokenB.balanceOf(msg.sender) >= amountB, "Insufficient token B balance");

        uint reserveA = tokenA.balanceOf(address(this));
        uint reserveB = tokenB.balanceOf(address(this));

        if (reserveA == 0 && reserveB == 0) {
            // First LP sets the ratio
            tokenA.transferFrom(msg.sender, address(this), amountA);
            tokenB.transferFrom(msg.sender, address(this), amountB);

            uint256 lpAmount = 1e18;
            lpToken.mint(msg.sender, lpAmount);
        } else {
            // Maintain ratio: amountA / amountB = reserveA / reserveB
            require(
                reserveA * amountB == reserveB * amountA,
                "Invalid deposit ratio"
            );

            uint256 totalSupply = lpToken.totalSupply();
            uint256 lpAmount = (totalSupply * amountA) / reserveA;
            lpToken.mint(msg.sender, lpAmount);

            tokenA.transferFrom(msg.sender, address(this), amountA);
            tokenB.transferFrom(msg.sender, address(this), amountB);
        }
    }

    function removeLiquidity(uint256 lpAmount) public  {
        require(lpAmount > 0, "Zero amount");
        uint256 totalSupply = lpToken.totalSupply();
        require(totalSupply > 0, "No LP tokens");
        require(lpToken.balanceOf(msg.sender) >= lpAmount, "Insufficient LP balance");

        uint reserveA = tokenA.balanceOf(address(this));
        uint reserveB = tokenB.balanceOf(address(this));

        uint256 amountA = (lpAmount * reserveA) / totalSupply;
        uint256 amountB = (lpAmount * reserveB) / totalSupply;

        lpToken.burn(msg.sender, lpAmount);

        tokenA.transfer(msg.sender, amountA);
        tokenB.transfer(msg.sender, amountB);
    }

    function swap_A_to_B(uint256 amt) public {
        require(tokenA.balanceOf(msg.sender) >= amt, "Insufficient token A balance");
        uint reserveA = tokenA.balanceOf(address(this));
        uint reserveB = tokenB.balanceOf(address(this));

        uint256 eff_A = (997*amt)/1000;        
        uint256 amt_B = (eff_A * reserveB) / (reserveA + eff_A);
        require(tokenB.balanceOf(address(this)) >= amt_B, "Insufficient token B reserves");

        tokenB.transfer(msg.sender, amt_B);
        tokenA.transferFrom(msg.sender, address(this), eff_A);
    }

    function swap_B_to_A(uint amt) public {
        require(tokenB.balanceOf(msg.sender) >= amt, "Insufficient token B balance");
        uint reserveA = tokenA.balanceOf(address(this));
        uint reserveB = tokenB.balanceOf(address(this));

        uint256 eff_B = (997*amt)/1000;        
        uint256 amt_A = (eff_B * reserveA) / (reserveB + eff_B);
        require(tokenA.balanceOf(address(this)) >= amt_A, "Insufficient token A reserves");

        tokenA.transfer(msg.sender, amt_A);
        tokenB.transferFrom(msg.sender, address(this), eff_B);
    }

    function get_B_to_A(uint256 amt_B) public view returns (uint256){
        uint reserveA = tokenA.balanceOf(address(this));
        uint reserveB = tokenB.balanceOf(address(this));
        return (reserveA * amt_B)/reserveB;
    }

    function get_A_to_B(uint256 amt_A) public view returns (uint256){
        uint reserveA = tokenA.balanceOf(address(this));
        uint reserveB = tokenB.balanceOf(address(this));
        return (reserveB * amt_A)/reserveA;
    }

    function get_spot_price() public view returns (uint256){
        uint reserveA = tokenA.balanceOf(address(this));
        uint reserveB = tokenB.balanceOf(address(this));
        return reserveA/reserveB;
    }

    function get_reserveA() public view returns (uint256){
        uint reserveA = tokenA.balanceOf(address(this));
        return reserveA;
    } 

    function get_reserveB() public view returns (uint256){
        uint reserveB = tokenB.balanceOf(address(this));
        return reserveB;
    } 

    function get_lp_tokens() public view returns (uint256){
        return lpToken.balanceOf(msg.sender);
    }
}
