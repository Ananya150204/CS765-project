// SPDX-License-Identifier: GPL-3.0

pragma solidity >=0.7.0 <0.9.0;

import "@openzeppelin/contracts/token/ERC20/IERC20.sol";
import "@openzeppelin/contracts/token/ERC20/ERC20.sol";

contract LPToken is ERC20 {
    constructor() ERC20("LP Token", "LPT") {}

    function mint(address to, uint256 amount) external {
        _mint(to, amount);
    }

    function burn(address from, uint256 amount) external {
        _burn(from, amount);
    }
}

contract SimpleAMM {
    IERC20 public tokenA;
    IERC20 public tokenB;
    LPToken public lpToken;

    uint256 public reserveA;
    uint256 public reserveB;

    constructor(address _tokenA, address _tokenB) {
        tokenA = IERC20(_tokenA);
        tokenB = IERC20(_tokenB);
        lpToken = new LPToken();
    }

    function addLiquidity(uint256 amountA, uint256 amountB) public {
        require(amountA > 0 && amountB > 0, "Zero amounts");

        if (reserveA == 0 && reserveB == 0) {
            // First LP sets the ratio
            tokenA.transferFrom(msg.sender, address(this), amountA);
            tokenB.transferFrom(msg.sender, address(this), amountB);

            uint256 lpAmount = 1e18;
            lpToken.mint(msg.sender, lpAmount);

            reserveA += amountA;
            reserveB += amountB;
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
            reserveA += amountA;
            reserveB += amountB;
        }
    }

    function removeLiquidity(uint256 lpAmount) public  {
        require(lpAmount > 0, "Zero amount");

        uint256 totalSupply = lpToken.totalSupply();
        require(totalSupply > 0, "No LP tokens");

        uint256 amountA = (lpAmount * reserveA) / totalSupply;
        uint256 amountB = (lpAmount * reserveB) / totalSupply;

        lpToken.burn(msg.sender, lpAmount);

        reserveA -= amountA;
        reserveB -= amountB;

        tokenA.transfer(msg.sender, amountA);
        tokenB.transfer(msg.sender, amountB);
    }

    function swap_A_to_B(uint256 amt) public {
        uint256 eff_A = (997*amt)/1000;        
        uint256 amt_B = (eff_A * reserveB) / (reserveA + eff_A);

        tokenB.transfer(msg.sender, amt_B);
        tokenA.transferFrom(msg.sender, address(this), eff_A);

        reserveA += eff_A;
        reserveB -= amt_B;
    }

    function swap_B_to_A(uint amt) public {
        uint256 eff_B = (997*amt)/1000;        
        uint256 amt_A = (eff_B * reserveA) / (reserveB + eff_B);

        tokenA.transfer(msg.sender, amt_A);
        tokenB.transferFrom(msg.sender, address(this), eff_B);

        reserveB += eff_B;
        reserveA -= amt_A;
    }

    function get_B_to_A(uint256 amt_B) public view returns (uint256){
        return (reserveA * amt_B)/reserveB;
    }

    function get_A_to_B(uint256 amt_A) public view returns (uint256){
        return (reserveB * amt_A)/reserveA;
    }

    function get_spot_price() public view returns (uint256){
        return reserveA/reserveB;
    }

    function get_reserveA() public view returns (uint256){
        return reserveA;
    } 

    function get_reserveB() public view returns (uint256){
        return reserveB;
    } 
}
