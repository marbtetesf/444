contract c {
    function set(uint key) public returns (bool) { data[key] = msg.data; return true; }
    function copy(uint from, uint to) public returns (bool) { data[to] = data[from]; return true; }
    mapping(uint => bytes) data;
}
// ====
// compileViaYul: also
// ----
// set(uint256): 1, 2 -> true
// gas ir: 104197
// gas irOptimized: 103365
// gas legacy: 103491
// gas legacyOptimized: 103110
// set(uint256): 2, 2, 3, 4, 5 -> true
// gas ir: 164902
// gas irOptimized: 164052
// gas legacy: 164121
// gas legacyOptimized: 163740
// storage: nonempty
// copy(uint256,uint256): 1, 2 -> true
// storage: nonempty
// copy(uint256,uint256): 99, 1 -> true
// storage: nonempty
// copy(uint256,uint256): 99, 2 -> true
// storage: empty
