#### TBasic Native Procedure Conventions


##### Invariants: 
1. Returns NONE in STACK[CALLEE_BP].
2. The convention is followed for the VM:
    - CALLEE_BP = `SP - ARGC`
    - PROCEDURE_REF = `STACK[CALLEE_BP]`
    - LOCAL-N = `STACK[CALLEE_BP + 1 + N]`

##### Example:
The stack layout: of `print(1, 2, 3)` before invocation is:
```
| Value(Int(3)) | <--- SP <--- CALLEE_BP + 3
| Value(Int(2)) |
| Value(Int(1)) | <--- LOCAL_1 <--- CALLEE_BP + 1
| Value(Fun-ID) | <--- CALLEE_BP = SP - ARGC = SP - 3 <--- PUT "none"
```