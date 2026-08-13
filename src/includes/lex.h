#ifndef TBASIC_LEX_H
#define TBASIC_LEX_H

#include "mystr.h"
#include "token.h"



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
    uint16_t line;
    uint16_t col;
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
