// SPDX-License-Identifier: GPL-3.0

pragma solidity >=0.7.0 <0.9.0;

import "@openzeppelin/contracts/token/ERC20/ERC20.sol";

contract LPToken is ERC20 {
    address dexadd;
    constructor(address dex) ERC20("LP Token", "LPT") {
        dexadd = dex;
    }

    function mint(address to, uint256 amount) external {
        require(msg.sender == dexadd, "ONLY DEX CAN MINT LP TOKENS");
        _mint(to, amount);
    }

    function burn(address from, uint256 amount) external {
        require(msg.sender == dexadd, "ONLY DEX CAN BURN LP TOKENS");
        _burn(from, amount);
    }
}

