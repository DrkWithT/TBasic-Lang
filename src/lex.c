#include <string.h>
#include "lex.h"



Lexer make_lexer(const charspan *s, const LexItem *special_array) {
    return (Lexer) {
        .specials = special_array,
        .pos = 0,
        .end = charspan_len(s),
        .line = 1,
        .col = 1,
    };
}

int8_t lexer_done(const Lexer *self) {
    return self->pos >= self->end;
}

void lexer_consume(Lexer *self, char c) {
    if (c == '\n') {
        self->line++;
        self->col = 1;
    } else {
        self->col++;
    }

    self->pos++;
}

Token lexer_lex_space(Lexer *self, const charspan *s) {
    const int tk_start = self->pos;
    const uint16_t tk_line = self->line;
    const uint16_t tk_col = self->col;

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
        .col = tk_col,
        .tag = tk_spaces
    };
}

Token lexer_lex_single(Lexer *self, TkTag tag, const charspan *s) {
    const int tk_start = self->pos;
    const uint16_t tk_line = self->line;
    const uint16_t tk_col = self->col;

    lexer_consume(self, s->data[tk_start]);

    return (Token) {
        .begin = tk_start,
        .length = 1,
        .line = tk_line,
        .col = tk_col,
        .tag = tag
    };
}

Token lexer_lex_between(Lexer *self, TkTag tag, const charspan *s) {
    const char delim = s->data[self->pos];
    lexer_consume(self, delim);

    const int tk_start = self->pos;
    const uint16_t tk_line = self->line;
    const uint16_t tk_col = self->col;
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
        .col = tk_col,
        .tag = (closed) ? tag : tk_unknown
    };
}

static int8_t lexer_lex_escape_seq(Lexer *self, const charspan *s) {
    lexer_consume(self, '\\'); // skip checked '\';

    const char peek0 = s->data[self->pos];
    if (peek0 == 'v' || peek0 == 'r' || peek0 == 't' || peek0 == 'n') {
        lexer_consume(self, peek0);
        return 1;
    } else if (peek0 == 'x') {
        lexer_consume(self, peek0);
    } else {
        return 0;
    }

    const char peek1 = s->data[self->pos];
    if (is_dec_digit(peek1)) {
        lexer_consume(self, peek1);
    } else {
        return 0;
    }

    const char peek2 = s->data[self->pos];
    if (is_dec_digit(peek2)) {
        lexer_consume(self, peek2);
    } else {
        return 0;
    }

    return 1;
}

Token lexer_lex_escaped_str(Lexer *self, const charspan *s) {
    const char delim = s->data[self->pos];
    lexer_consume(self, delim);

    const int tk_start = self->pos;
    const uint16_t tk_line = self->line;
    const uint16_t tk_col = self->col;
    int8_t validated = 1;
    int8_t escaped = 0;

    while (!lexer_done(self)) {
        const char peeked = s->data[self->pos];
        if (peeked == delim) {
            lexer_consume(self, peeked);
            break;
        } else if (peeked != '\\') {
            lexer_consume(self, peeked);
        } else if (!lexer_lex_escape_seq(self, s)) {
            validated = 0;
        } else {
            escaped = 1;
        }
    }

    return (Token) {
        .begin = tk_start,
        .length = self->pos - tk_start - 1,
        .line = tk_line,
        .col = tk_col,
        .tag = validated
            ? (escaped)
                ? tk_escaped_str
                : tk_string
            : tk_unknown
    };
}

Token lexer_lex_numeric(Lexer *self, const charspan *s) {
    const int tk_start = self->pos;
    const uint16_t tk_line = self->line;
    const uint16_t tk_col = self->col;
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
        .col = tk_col,
        .tag = temp_tag
    };
}

Token lexer_lex_based_int(Lexer *self, const charspan *s) {
    lexer_consume(self, s->data[self->pos]); // ? eat '#' prefix of BIN / HEX int literal.

    LexFn predicate = NULL;
    TkTag tk_tag = tk_unknown;

    switch (s->data[self->pos]) {
        case 'x':
        case 'X':
            predicate = is_hex_digit;
            tk_tag = tk_integer_hex;
            lexer_consume(self, s->data[self->pos]);
            break;
        case 'b':
        case 'B':
            predicate = is_bin_digit;
            tk_tag = tk_integer_bin;
            lexer_consume(self, s->data[self->pos]);
            break;
        default:
            break;
    }

    if (predicate == NULL) {
        return (Token) {
            .begin = self->pos - 1,
            .length = 2,
            .line = self->line,
            .col = self->col - 1,
            .tag = tk_tag
        };
    }

    const int tk_begin = self->pos;
    const uint16_t tk_line = self->line;
    const uint16_t tk_col = self->col;

    while (!lexer_done(self)) {
        const char c = s->data[self->pos];

        if (predicate(c)) {
            lexer_consume(self, c);
        } else {
            break;
        }
    }

    return (Token) {
        .begin = tk_begin,
        .length = self->pos - tk_begin,
        .line = tk_line,
        .col = tk_col,
        .tag = tk_tag
    };
}

Token lexer_lex_word(Lexer *self, const charspan *s) {
    const int tk_start = self->pos;
    const uint16_t tk_line = self->line;
    const uint16_t tk_col = self->col;

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
            break;
        }
    }

    return (Token) {
        .begin = tk_start,
        .length = tk_length,
        .line = tk_line,
        .col = tk_col,
        .tag = temp_tag
    };
}

Token lexer_lex_operator(Lexer *self, const charspan *s) {
    const int tk_start = self->pos;
    const uint16_t tk_line = self->line;
    const uint16_t tk_col = self->col;

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
        .col = tk_col,
        .tag = temp_tag
    };
}

Token lexer_next(Lexer *self, const charspan *s) {
    if (lexer_done(self)) {
        return (Token) {
            .begin = self->end,
            .length = 1,
            .line = self->line,
            .col = self->col,
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
        case '\"': return lexer_lex_escaped_str(self, s);
        case '#': return lexer_lex_based_int(self, s);
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
