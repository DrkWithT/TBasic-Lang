#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "mystr.h"


void charspan_dud(charspan *self) {
    self->data = NULL;
    self->length = 0;
}

void charspan_new(charspan *self, const char *s, size_t n) {
    self->data = s;
    self->length = n;
}

void charspan_from_str(charspan *self, const mystr *s) {
    const char *raw_s = s->data;

    self->data = raw_s;
    self->length = strnlen(raw_s, s->length);
}

void charspan_copy(charspan *self, const charspan *other) {
    self->data = other->data;
    self->length = other->length;
}

void charspan_del(charspan *self) {
    self->data = NULL;
    self->length = 0;
}



int8_t charspan_empty(const charspan *self) {
    return self->data == NULL || self->length == 0;
}

size_t charspan_len(const charspan *self) {
    return self->length;
}

int8_t charspan_equals_raw(const charspan *self, const char *s, size_t n) {
    size_t s_len = strlen(s);

    if (s_len != n) {
        return 0;
    }

    return !strncmp(self->data, s, n);
}

int8_t charspan_equals_charspan(const charspan *self, const charspan *other) {
    if (self->length != other->length) {
        return 0;
    }

    if (self->data != NULL && other->data != NULL) {
        return !strncmp(self->data, other->data, self->length);
    }

    return 0;
}



void mystr_dud(mystr *self) {
    self->data = NULL;
    self->length = 0;
    self->capacity = 0;
}

void mystr_res(mystr *self, size_t n) {
    if (n == 0) {
        return;
    }

    char *temp = calloc(n + 1, sizeof(char));

    if (temp != NULL) {
        memset(temp, '\0', n);
        self->data = temp;
        self->length = 0;
        self->capacity = n;
    }
}

void mystr_new(mystr *self, const char *s) {
    const size_t count = strlen(s) + 1;

    char *temp = calloc((count * 3) / 2, sizeof(char));

    if (temp != NULL) {
        strncpy(temp, s, count);
        self->data = temp;
        self->length = strlen(s);
        self->capacity = (count * 3) / 2;
    }
}

mystr* mystr_copy(mystr *self, const mystr *other) {
    if (self == other) {
        return self;
    }

    char *temp = calloc(other->capacity, sizeof(char));

    if (temp != NULL) {
        strncpy(temp, other->data, other->capacity);
        self->data = temp;
        self->length = other->length;
        self->capacity = other->capacity;
    }

    return self;
}

mystr* mystr_move(mystr *self, mystr *other) {
    if (self == other) {
        return self;
    }

    self->data = other->data;
    other->data = NULL;

    self->length = other->length;
    other->length = 0;

    self->capacity = other->capacity;
    other->capacity = 0;

    return self;
}

void mystr_del(mystr *self) {
    if (self->data != NULL) {
        free(self->data);
        self->data = NULL;
        self->capacity = 0;
    }
}



const char *mystr_raw(const mystr *self) {
    return self->data;
}

int8_t mystr_empty(const mystr *self) {
    return self->data == NULL || self->length == 0;
}

size_t mystr_len(const mystr *self) {
    return self->length;
}

int8_t mystr_append_raw(mystr *self, const char *s, size_t n) {
    const size_t next_pos = self->length;
    size_t old_capacity = self->capacity;
    
    // ? character push count = old_capacity - 1 - next_pos for checking
    if (old_capacity - 1 - next_pos < n) {
        const size_t new_capacity = old_capacity + (
                (next_pos + n < old_capacity)
                ? old_capacity
                : ((next_pos + n) * 3) / 2
            );
        char *reallocated_temp = realloc(self->data, sizeof(char) * new_capacity);

        if (reallocated_temp == NULL) {
            return 0;
        }

        self->data = reallocated_temp;
        self->capacity = new_capacity;
    }

    for (size_t i = 0; i < n; i++) {
        self->data[next_pos + i] = s[i];
    }

    self->data[next_pos + n] = '\0';
    self->length += n;

    return 1;
}

int8_t mystr_append_charspan(mystr *self, const charspan *span, size_t n) {
    return mystr_append_raw(self, span->data, span->length);
}

int8_t mystr_append_mystr(mystr *self, const mystr *other) {
    return mystr_append_raw(self, other->data, other->length);
}


int8_t mystr_equals_raw(const mystr *self, const char *s, size_t n) {
    if (mystr_empty(self)) {
        return 0;
    } else if (self->length != n) {
        return 0;
    }

    return !strncmp(self->data, s, n);
}

int8_t mystr_equals_charspan(const mystr *self, const charspan *span) {
    if (mystr_empty(self) && charspan_empty(span)) {
        return 1;
    } else if (mystr_empty(self) || charspan_empty(span)) {
        return 0;
    } else if (mystr_len(self) != charspan_len(span)) {
        return 0;
    }

    return !strncmp(self->data, span->data, self->length);
}

int8_t mystr_equals_mystr(const mystr *self, const mystr *other) {
    if (self == other) {
        return 1;
    } else if (self->length != other->length) {
        return 0;
    }

    return !strncmp(self->data, other->data, self->length);
}



// ! This is UNCHECKED, assuming that the charspan is for a valid integer lexeme after tokenization.
int charspan_atoi(const charspan *s) {
    int result = 0;
    int base = 1;
    
    if (s->length > 8) {
        return 0;
    }

    const int8_t is_signed = s->data[0] == '-';

    for (int i = s->length - 1; i >= is_signed; i--, base *= 10) {
        const int digit = s->data[i] - '0';

        result += digit * base;
    }

    if (is_signed) {
        result = -result;
    }

    return result;
}

float charspan_atof(const charspan *s) {
    static const float digit_vals[] = {0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0, 7.0f, 8.0f, 9.0f}; // ! NOTE: This uses a lookup table for digits as floats... Casting int to float is slow because of RAX <--> XMM.

    float result = 0.0f;
    float base = 1.0f;
    const int8_t is_signed = s->data[0] == '-';

    for (int point_search_pos = is_signed; point_search_pos < s->length; point_search_pos++) {
        if (s->data[point_search_pos + 1] == '.') {
            break;
        }

        base *= 10.0f;
    }

    for (int i = is_signed; i < s->length; i++) {
        const char c = s->data[i];

        if (c == '.') {
            continue;
        }

        result += digit_vals[c - '0'] * base;
        base /= 10.0f;
    }

    if (is_signed) {
        result = -result;
    }

    return result;
}

float charspan_checked_atof(const charspan *s) {
    static const float digit_vals[] = {0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0, 7.0f, 8.0f, 9.0f};

    float result = 0.0f;
    float base = 1.0f;
    const int8_t is_signed = s->data[0] == '-';
    int8_t reached_point = 0;

    for (int point_search_pos = is_signed; point_search_pos < s->length; point_search_pos++) {
        if (s->data[point_search_pos + 1] == '.') {
            break;
        }

        base *= 10.0f;
    }

    for (int i = is_signed; i < s->length; i++) {
        const char c = s->data[i];

        if (c == '.') {
            if (reached_point) {
                result = NAN;
                break;
            } else {
                reached_point = 1;
                continue;
            }
        }

        if (c >= '0' && c <= '9') {   
            result += digit_vals[c - '0'] * base;
            base /= 10.0f;
        } else {
            result = NAN;
            break;
        }
    }

    if (is_signed) {
        result = -result;
    }

    return result;
}

