#ifndef TBASIC_TOKEN_H
#define TBASIC_TOKEN_H

#include <stddef.h>
#include <stdint.h>



typedef enum token_tag_t {
    tk_unknown,
    tk_spaces,
    tk_comment,
    tk_keyword_let,
    tk_keyword_bind,
    tk_keyword_if,
    tk_keyword_else,
    tk_keyword_while,
    tk_keyword_for,
    tk_keyword_break,
    tk_keyword_continue,
    tk_keyword_ret,
    tk_keyword_throw,
    tk_keyword_try,
    tk_keyword_catch,
    tk_keyword_err,
    tk_keyword_fun,
    tk_keyword_uses,
    tk_keyword_end,
    tk_keyword_assert,
    tk_identifier,
    tk_none,
    tk_true,
    tk_false,
    tk_integer,
    tk_real,
    tk_string,
    tk_os_nullish,
    tk_os_nullcol,
    tk_os_bit_not,
    tk_os_times,
    tk_os_slash,
    tk_os_plus,
    tk_os_minus,
    tk_os_bit_and,
    tk_os_bit_or,
    tk_os_bit_xor,
    tk_os_bit_shl,
    tk_os_bit_shr,
    tk_os_bang,
    tk_os_equals,
    tk_os_bang_equals,
    tk_os_lesser,
    tk_os_greater,
    tk_os_and,
    tk_os_or,
    tk_os_bind_equals,  // ? `:=` is for mutating a variable
    tk_comma,
    tk_colon,
    tk_semicolon,
    tk_lparen,
    tk_rparen,
    tk_lbrack,
    tk_rbrack,
    tk_lbrace,
    tk_rbrace,
    tk_eof
} TkTag;

typedef struct token_v_t {
    int begin;
    int length;
    uint16_t line;
    uint16_t col;
    TkTag tag;
} Token;

#endif