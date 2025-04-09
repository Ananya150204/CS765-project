// SPDX-License-Identifier: GPL-3.0

pragma solidity >=0.7.0 <0.9.0;

import "@openzeppelin/contracts/token/ERC20/ERC20.sol";

contract Token is ERC20 {
    constructor(uint256 initialSupply) ERC20("TokenA", "TKA") {
        _mint(msg.sender, initialSupply);
    }
    // initial supply, balances and transfers all will be done in terms of mini-tokens (i.e. 1e-18 tokens)
}

