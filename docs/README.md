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
 - Refactor compiler to use cleaner tracking of flag changes, passing bitflags per callee. **OK**
 - Refactor VM to trampoline into native calls vs. doing a branch each time. **OK**
 - Fix exceptions to not naively unwind out of the nearest `RET`. What if a `RET` is in a `TRY` and makes that exception falsely uncaught? **OK**

#### v0.11.x: QoL 4
 - Add `ASSERT <expr>, <simple-expr, call or literal>;` **OK**
 - Add lambdas: **OK**
  ```
    LET foo : FUN (a, b):
      RET a + b;
    END;
  ```
 - Add destructuring of lists into a finite set of names. **OK**
  ```
    BIND nums : [a, b, c];
  ```

#### v0.12.x: Diagnostic Improvements
 - Fix the atrocious compiler errors to be more specific with column numbers per line. **OK**
 - Add debug info per bytecode chunk e.g line & col per statement. **OK**

#### v0.13.x: QoL 5
 - Add closures. **WIP**
  ```
    FUN makeCounter(x, y):
      RET FUN() USES (x, y):
        x = x + y;

        RET x;
      END
    END
  ```
 - Add iterator support.
    - Iterators only work on `ObjBase` types, holding a reference to them.
    - Iterators have certain codes for actions:
      - 0 -> check
      - 1 -> peek
      - 2 -> get & consume
    - Library: `mkit` to create forward iterators.
 - Add list slicing.
  ```
    LET foo : [1, 2, 3, 4];
    LET partFoo : CUT [foo : 0 : 2];
  ```
 - Add bitwise NOT, AND, OR
 - Add bit shifting.

#### v0.14.x: QoL 6
 - Expand builtin library:
    - Add `typeof` function:
      - `typeof(val)`
    - Add list functions:
      - `lsrev(list)`
      - `lscat(dest, src)` using native `__lscat(dest, src)`.
      - `lsclr(list)` using native `__lsclr(list)`.
      - `lsmap(list, fn)`
      - `lsflt(list, fn)`
      - `apply(fn, argv)` using native `__apply(fn, argv)`.
    - Add dict functions:
      - `dckeys(dict)` using native `__iterof(dict)`.
    - Add file stream functions:
      - `fopen(path, bitflags)`
      - `fclose(fd)`
      - `fpeek(fd)`
      - `fread(fd, buf, n)`
      - `fwrite(fd, buf, n)`

#### v0.15.x: QoL 7
 - Add the ability to generate standalone C files which bundle TBasic bytecode & the interpreter as an executable.
