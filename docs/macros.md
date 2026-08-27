#### Macro system

#### Prior Considerations
 - Rust macros:
    - Match syntactical patterns of tokens to custom syntax... Transforms parts of the AST.
 - TBasic:
    - No AST because the compiler is single-pass. Tokens are lexed on demand.
    - If macros are doable, they would be functions which affect the token stream.
    - **MUST DO:** Refactor the compiler to lex everything AOT, intern source strings, make tokens refer to bounds in source strings, etc.

##### Purpose
 - Reduce repetition / boilerplate in TBasic code!
 - Extend the language syntax!
 - Compile-time evaluation!

##### What's a macro?
 - A macro is like a function, but it can take actions upon the token stream reference.
 - All macros must have only 1 definition.
 - Parsed and then compiled into macro-bytecode before anything else.
 - Inputs: arguments are a finite list of token-tagged parameters.
 - Processing:
    - Match cases of lexical constraints to a sequence of tokens to insert (expand the macro).
 - Output: the token stream has new tokens inserted.
 - Evaluated by a sub-VM.

##### Macro Constraints:
 - `atom`: a single-token element i.e `NIL`, boolean, number, or string literal.
 - `ident`: a name.
 - `pack`: a variadic window of tokens.

##### Macro Literals / Routines:
 - `$@`: the token stream reference (a dynamic array of tokens).
 - `$def(name, val)`: defines a meta variable in the macro.
 - `$len(a)`: gets the length of a token pack.
 - `$eq(dest, src), $ne, $lt, $gt`: compares meta variables.
 - `$error("some message")`: fails macro expansion with a custom error.
 - `$push(args...)`: appends to the token stream.
 - `$pop(n)`: pops n tokens from the token stream.
 - `$str(arg)`: converts a token into a string literal.
 - `$match(dest, src)`: matches lexemes of tokens.
 - `$exit()`: stops macro invocation.

##### Meta Statements:
 - `IF`: just like an IF statement but no ELSE.
 - `WHILE`: just like a while loop but in the macro context.
 - `RET`: stops macro execution.
 - `DO <macro name> (args...)`: invokes a macro within the macro context.

##### Example - Variadic list appending.
```
MACRO CAT_ATOMS($dest: ident, $args: pack):
    $def(i, 0);
    $def(n, $len($args));

    IF $lt(n, 1):
        $error("Appends of 0 elements forbidden!");
    END

    WHILE $lt(i, n):
        $push($@, dest);
        $push($@,
            '[', ']',
            $space(), '=',
            $space(), $get(args, i),
            ';',
            $lf()
        );
        $add(i, 1);
    END

    $exit();
END
```

##### Example - Invoke a macro in normal code
```
LET nums : [];
$CAT_ATOMS(nums, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9)
```