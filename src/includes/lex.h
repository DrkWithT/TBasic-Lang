#ifndef TBASIC_LEX_H
#define TBASIC_LEX_H

#include "mystr.h"

typedef enum token_tag_t {
    tk_unknown,
    tk_spaces,
    tk_comment,
    tk_keyword_let,
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
    tk_keyword_end,
    tk_identifier,
    tk_none,
    tk_true,
    tk_false,
    tk_integer,
    tk_real,
    tk_string,
    tk_os_nullish,
    tk_os_nullcol,
    tk_os_times,
    tk_os_slash,
    tk_os_plus,
    tk_os_minus,
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
    int line;
    TkTag tag;
} Token;

static inline int8_t is_word_symbol(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_';
}

static inline int8_t is_numeric_symbol(char c) {
    return (c >= '0' && c <= '9') || c == '.';
}

static inline int8_t is_op_symbol(char c) {
    switch (c) {
        case '*': case '/': case '+': case '-': // arithmetic
        case '=': case '!': case '<': case '>': case '|': case '&': // comparisons / logicals
        case ':': case '?': return 1; // extra
        default: return 0;
    }
}

static inline int8_t is_space_symbol(char c) {
    switch (c) {
        case ' ': case '\t': case '\n': return 1;
        default: return 0;
    }
}

typedef struct lexical_item_t {
    const char *literal;
    TkTag tag;
} LexItem;

typedef struct lexer_t {
    const LexItem *specials;
    int pos;
    int end;
    int line;
} Lexer;

Lexer make_lexer(const charspan *s, const LexItem *special_array);

int8_t lexer_done(const Lexer *self);

void lexer_consume(Lexer *self, char c);

Token lexer_lex_space(Lexer *self, const charspan *s);

Token lexer_lex_single(Lexer *self, TkTag tag, const charspan *s);

Token lexer_lex_between(Lexer *self, TkTag tag, const charspan *s);

Token lexer_lex_numeric(Lexer *self, const charspan *s);

Token lexer_lex_word(Lexer *self, const charspan *s);

Token lexer_lex_operator(Lexer *self, const charspan *s);

Token lexer_next(Lexer *self, const charspan *s);

#endif
