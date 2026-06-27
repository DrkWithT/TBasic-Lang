#### Bytecode Serialization Format

##### 1: Heading
 - "TBCF" Magic prefix (TBasic Code Format)
 - Metadata Fields:
    - `data: u8[4]`
    - entry chunk ID ~ `u32` (despite being casted to `int` in process)

##### 2: Global string Data
 - Repeat N times according to metadata field `0x1`:
    - `length: u32, data: char[length]`
    - If `length` is `0`, there are no more string blobs to load.

##### 3: Chunk Data
 - Repeat N times according to metadata field `0x2`:
    - `inst_length: u64, data: u8[4 * inst_length] OF INST_FMT, konst_length: u64, konst_data: u8[5 * konst_length] OF VALUE_FMT`
    - `INST_FMT`: 4 bytes blob of 1 opcode `u8`, 1 bitflag `u8`, and 2-byte `u16`
    - `VALUE_FMT`: 5 bytes blob of 1 byte tag before 4 byte payload (`memcpy` into native value OR `u8` of `0` or `1` _for booleans_)
    - If `inst_length` is `0`, there are no more things to load
