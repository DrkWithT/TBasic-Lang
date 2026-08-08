## README

### Brief
A very trivial scripting language implemented in C11. Only for educational purposes.

### Usage - Building
 - Requires CMake 4.2 (Actually, 3.16+ is fine!)
 - Make, Ninja, or another build tool
 - Clang is preferred
 - **Usage:** `./project.sh help`
   - Statically-linked binary: `./project.sh br`
   - Dynamically-linked demo binary (uses shared lib): `./project.sh sr`

### Basic Features
 - BASIC inspired but:
    - No line numbers or GOTO.
    - And even more QoL extras!

### Roadmap
#### v0.1.x
 - Add logical operator (&&, ||) support. **OK**
 - Add assignment operator.
    - Add variable assignment. **OK**
 - Add loops support. **OK**

#### v0.2.x
 - Add object base. **OK**
 - Add object heap. **OK**
 - Add simple, fixed size list objects. **OK**
 - Add mark & sweep GC. **OK**

#### v0.3.x: QoL 0
 - Add native function library for: **OK**
   - I/O: print(...args)
   - Math: powf(), sqrtf(), clamp(), floorf(), and ceilf()

#### v0.4.x
 - Add for-loop variation with `BREAK;` and `CONTINUE;` **OK**
 - Add negative number literals. **OK**
 - Add object display methods. **OK**

#### v0.5.x
 - Add immutable strings as separate, interned values. **OK**
   - Create string type. **OK**
   - Add more library functions:
      - `stoi`, `stof` **OK**

#### v0.6.x:
 - Add "dict objects": **OK**
 - Add `foo["bar"]` syntax for accessing any keys of objects vs. `::`. **OK**
 - Add compiler support for not duplicating string constants. **OK**
 - Add compiler support for dict literals. **OK**
 - Add `make_dict_dud` opcode to VM & generation. **OK**

#### v0.7.x: QoL 1
 - Make unified API to register native functions & manipulate VM state. **OK**
 - Support shared object library builds, exposing a header API to the shared lib. **OK**

#### v0.8.x: QoL 2
 - Fix IF-ELSE syntax to allow ELSE IF or premature END. **OK**
 - Add null handling operators: **OK**
   - Prefix `?` for compact null checks.
   - Binary Infix `??` for null coalescing.

#### v0.9.x: QoL 3
 - Fix var hoisting. **OK**
   - Add `RESERVE` opcode.
   - Change variable generations to set a local offset in the RESERVE-d stack slots upon assignment or initialization.
   - Implement `RESERVE` opcode.
 - Add try-catch & exceptions. **OK**
 - Add bytecode optimization pass with super-instructions. **OK**

#### v0.10.x: BIG Refactor
 - Refactor compiler to use cleaner tracking of flag changes, passing bitflags per callee.
 - Refactor VM to trampoline into native calls vs. doing a branch each time.

#### v0.11.x: QoL 4
 - Add destructuring of lists into a finite set of names & a ranged view.
 - Add viewing ranges and generative ranges (giving `NIL` if done):
    - `*[list : 1 : 4]`, viewing 4 elements starting at position 1
    - `$[[1, 2, 3, 4, 5] | twice]`, generating 5 multiples of 2 
 - Add thunks:
  ```
    LET fooSum := THUNK (CTX, ...ARGV):
      LET [a, b, ...rest] = ARGV;

      RET CTX["x"] * a + CTX["y"] * b;
    END;

    print(UNTHUNK(fooSum, {"x": 3, "y": 7}, 1, 4));
  ```
 - Add TASKS for timed-out operations:
  ```
    `timed out hello after 3s`
    LET test := TASK (CTX, ...ARGV) AFTER 3000:
      print("Hello");
    END
  ```
