contract B
{
    uint x;
    function getBalance() public view returns (uint) {
        return address(this).balance * 1000 + x;
    }
    constructor(uint _x) payable {
        x = _x;
    }
}

contract A {
    function f() public payable returns (uint, uint, uint) {
        B x = new B{salt: "abc", value: 3}(7);
        B y = new B{value: 3, salt: "abc"}(8);
        B z = new B{salt: "abc", value: 3}(9);
        return (x.getBalance(), y.getBalance(), z.getBalance());
    }
}
// ====
// EVMVersion: >=constantinople
// compileViaYul: also
// ----
// f(), 10 ether -> 3007, 3008, 3009
// gas ir: 338630
// gas irOptimized: 338630
// gas legacy: 476123
// gas legacyOptimized: 340251
