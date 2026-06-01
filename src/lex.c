#include <string.h>
#include "lex.h"

Lexer make_lexer(const charspan *s, const LexItem *special_array) {
    return (Lexer) {
        .specials = special_array,
        .pos = 0,
        .end = charspan_len(s),
        .line = 1
    };
}

int8_t lexer_done(const Lexer *self) {
    return self->pos >= self->end;
}

void lexer_consume(Lexer *self, char c) {
    if (c == '\n') {
        self->line++;
    }

    self->pos++;
}

Token lexer_lex_space(Lexer *self, const charspan *s) {
    const int tk_start = self->pos;
    const int tk_line = self->line;

    while (!lexer_done(self)) {
        const char c = s->data[self->pos];

        if (is_space_symbol(c)) {
            lexer_consume(self, c);
        } else {
            break;
        }
    }

    return (Token) {
        .begin = tk_start,
        .length = self->pos - tk_start,
        .line = tk_line,
        .tag = tk_spaces
    };
}

Token lexer_lex_single(Lexer *self, TkTag tag, const charspan *s) {
    const int tk_start = self->pos;
    const int tk_line = self->line;

    lexer_consume(self, s->data[tk_start]);

    return (Token) {
        .begin = tk_start,
        .length = 1,
        .line = tk_line,
        .tag = tag
    };
}

Token lexer_lex_between(Lexer *self, TkTag tag, const charspan *s) {
    const char delim = s->data[self->pos];
    lexer_consume(self, delim);

    const int tk_start = self->pos;
    const int tk_line = self->line;
    int8_t closed = 0;

    while (!lexer_done(self)) {
        const char c = s->data[self->pos];

        if (c == delim) {
            lexer_consume(self, c);
            closed = 1;

            break;
        } else {
            lexer_consume(self, c);
        }
    }

    return (Token) {
        .begin = tk_start,
        .length = self->pos - tk_start - 1,
        .line = tk_line,
        .tag = (closed) ? tag : tk_unknown
    };
}

Token lexer_lex_numeric(Lexer *self, const charspan *s) {
    const int tk_start = self->pos;
    const int tk_line = self->line;
    int8_t points = 0;

    if (s->data[self->pos] == '-') {
        lexer_consume(self, '-');
    }

    while (!lexer_done(self)) {
        const char c = s->data[self->pos];

        if (is_numeric_symbol(c)) {
            if (points >= 1 && c == '.') {
                break;
            } else if (c == '.') {
                points++;
            }

            lexer_consume(self, c);
        } else {
            break;
        }
    }

    TkTag temp_tag;

    if (points == 0) {
        temp_tag = tk_integer;
    } else if (points == 1) {
        temp_tag = tk_real;
    } else {
        temp_tag = tk_real;
    }

    return (Token) {
        .begin = tk_start,
        .length = self->pos - tk_start,
        .line = tk_line,
        .tag = temp_tag
    };
}

Token lexer_lex_word(Lexer *self, const charspan *s) {
    const int tk_start = self->pos;
    const int tk_line = self->line;

    while (!lexer_done(self)) {
        const char c = s->data[self->pos];

        if (is_word_symbol(c)) {
            lexer_consume(self, c);
        } else {
            break;
        }
    }

    const int tk_length = self->pos - tk_start;
    TkTag temp_tag = tk_identifier;

    for (const LexItem *special_it = self->specials; special_it->literal != NULL; special_it++) {
        charspan lexeme;
        charspan_new(&lexeme, s->data + tk_start, tk_length);

        if (charspan_equals_raw(&lexeme, special_it->literal, tk_length)) {
            temp_tag = special_it->tag;
        }
    }

    return (Token) {
        .begin = tk_start,
        .length = tk_length,
        .line = tk_line,
        .tag = temp_tag
    };
}

Token lexer_lex_operator(Lexer *self, const charspan *s) {
    const int tk_start = self->pos;
    const int tk_line = self->line;

    while (!lexer_done(self)) {
        const char c = s->data[self->pos];

        if (is_op_symbol(c)) {
            lexer_consume(self, c);
        } else {
            break;
        }
    }

    const int tk_length = self->pos - tk_start;
    TkTag temp_tag = tk_unknown;

    for (const LexItem *special_it = self->specials; special_it->literal != NULL; special_it++) {
        charspan lexeme;
        charspan_new(&lexeme, s->data + tk_start, tk_length);

        if (charspan_equals_raw(&lexeme, special_it->literal, tk_length)) {
            temp_tag = special_it->tag;
        }
    }

    return (Token) {
        .begin = tk_start,
        .length = tk_length,
        .line = tk_line,
        .tag = temp_tag
    };
}

Token lexer_next(Lexer *self, const charspan *s) {
    if (lexer_done(self)) {
        return (Token) {
            .begin = self->end,
            .length = 1,
            .line = self->line,
            .tag = tk_eof
        };
    }

    const char c = s->data[self->pos];

    switch (c) {
        case ',': return lexer_lex_single(self, tk_comma, s);
        case ';': return lexer_lex_single(self, tk_semicolon, s);
        case '(': return lexer_lex_single(self, tk_lparen, s);
        case ')': return lexer_lex_single(self, tk_rparen, s);
        case '[': return lexer_lex_single(self, tk_lbrack, s);
        case ']': return lexer_lex_single(self, tk_rbrack, s);
        case '{': return lexer_lex_single(self, tk_lbrace, s);
        case '}': return lexer_lex_single(self, tk_rbrace, s);
        case '`': return lexer_lex_between(self, tk_comment, s);
        case '\"': return lexer_lex_between(self, tk_string, s);
        default: break;
    }

    const char c2 = s->data[self->pos + 1];

    if (is_space_symbol(c)) {
        return lexer_lex_space(self, s);
    } else if (is_numeric_symbol(c) || (c == '-' && is_numeric_symbol(c2))) {
        return lexer_lex_numeric(self, s);
    } else if (is_op_symbol(c)) {
        return lexer_lex_operator(self, s);
    } else if (is_word_symbol(c)) {
        return lexer_lex_word(self, s);
    }

    return lexer_lex_single(self, tk_unknown, s);
}
