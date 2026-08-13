#ifndef TBASIC_VEC_H
#define TBASIC_VEC_H

#include <stddef.h>
#include <stdlib.h>
#include "basics.h"

#define ANYVEC_DEFAULT_SIZE 8

#define STUB_VEC(type)\
typedef struct ts_any_vec_##type##_t {\
    type *data;\
    size_t length;\
    size_t capacity;\
} AnyVec_##type;\
\
void AnyVec##_##type##_dud(AnyVec##_##type *self);\
void AnyVec##_##type##_new(AnyVec##_##type *self, size_t n, const type *value);\
void AnyVec##_##type##_copy(AnyVec##_##type *self, const AnyVec##_##type *other);\
void AnyVec##_##type##_move(AnyVec##_##type *self, AnyVec##_##type *x);\
void AnyVec##_##type##_del(AnyVec##_##type *self);\
int8_t AnyVec##_##type##_empty(AnyVec##_##type *self);\
size_t AnyVec##_##type##_len(const AnyVec##_##type *self);\
const type *AnyVec##_##type##_get(const AnyVec##_##type *self, size_t pos);\
type *AnyVec##_##type##_getm(AnyVec##_##type *self, size_t pos);\
const type *AnyVec##_##type##_last(const AnyVec##_##type *self);\
type *AnyVec##_##type##_lastm(AnyVec##_##type *self);\
void AnyVec##_##type##_push(AnyVec##_##type *self, const type *value);\
void AnyVec##_##type##_pop(AnyVec##_##type *self);\
\

#define IMPL_VEC(type)\
void AnyVec##_##type##_dud(AnyVec##_##type *self) {\
    type *temp = NULL;\
    DUD_OBJS_N(type, temp, ANYVEC_DEFAULT_SIZE, __FILE__, __LINE__);\
    \
    self->data = temp;\
    self->length = 0;\
    self->capacity = ANYVEC_DEFAULT_SIZE;\
}\
\
void AnyVec##_##type##_new(AnyVec##_##type *self, size_t n, const type *value) {\
    type *temp = NULL;\
    DUD_OBJS_N(type, temp, n, __FILE__, __LINE__);\
    \
    for (size_t filler_i = 0; filler_i < n; filler_i++) {\
        type##_##copy(temp + filler_i, value);\
    }\
    \
    self->data = temp;\
    self->length = 0;\
    self->capacity = n;\
}\
\
void AnyVec##_##type##_copy(AnyVec##_##type *self, const AnyVec##_##type *other) {\
    const size_t other_count = other->length;\
    const size_t other_capacity = other->capacity;\
    \
    type *temp = NULL;\
    DUD_OBJS_N(type, temp, other_capacity, __FILE__, __LINE__);\
    \
    for (size_t copy_i = 0; copy_i < other_count; copy_i++) {\
        type##_copy(temp + copy_i, other->data + copy_i);\
    }\
    \
    self->data = temp;\
    self->length = other->length;\
    self->capacity = other->capacity;\
}\
\
void AnyVec##_##type##_move(AnyVec##_##type *self, AnyVec##_##type *x) {\
    if (self == x) {\
        return;\
    }\
    \
    if (AnyVec##_##type##_empty(self) == 0) {\
        AnyVec##_##type##_del(self);\
    }\
    \
    self->data = x->data;\
    x->data = NULL;\
    self->length = x->length;\
    x->length = 0;\
    self->capacity = x->capacity;\
    x->capacity = 0;\
}\
\
void AnyVec##_##type##_del(AnyVec##_##type *self) {\
    if (self->data != NULL) {\
        type *temp = self->data;\
        for (size_t destroy_i = 0; destroy_i < self->length; destroy_i++) {\
            type##_del(temp + destroy_i);\
        }\
        free(self->data);\
        self->data = NULL;\
        self->length = 0;\
        self->capacity = 0;\
    }\
}\
\
int8_t AnyVec##_##type##_empty(AnyVec##_##type *self) {\
    return self->data == NULL || self->length == 0;\
}\
\
size_t AnyVec##_##type##_len(const AnyVec##_##type *self) {\
    return self->length;\
}\
\
const type *AnyVec##_##type##_get(const AnyVec##_##type *self, size_t pos) {\
    if (pos >= self->length) {\
        FATAL_ABORT("OOB Error", __FILE__, __LINE__, "OOB access of AnyVec here!")\
    }\
    \
    return self->data + pos;\
}\
\
type *AnyVec##_##type##_getm(AnyVec##_##type *self, size_t pos) {\
    if (pos >= self->length) {\
        FATAL_ABORT("OOB Error", __FILE__, __LINE__, "OOB access of AnyVec here!")\
    }\
    \
    return self->data + pos;\
}\
\
const type *AnyVec##_##type##_last(const AnyVec##_##type *self) {\
    if (self->data == NULL || self->length < 1) {\
        return NULL;\
    }\
    return self->data + self->length - 1;\
}\
\
type *AnyVec##_##type##_lastm(AnyVec##_##type *self) {\
    if (self->data == NULL || self->length < 1) {\
        return NULL;\
    }\
    return self->data + self->length - 1;\
}\
\
void AnyVec##_##type##_push(AnyVec##_##type *self, const type *value) {\
    const size_t next_pos = self->length;\
    const size_t old_capacity = self->capacity;\
    \
    if (next_pos >= old_capacity) {\
        const size_t new_capacity = 1 + (old_capacity * 3) / 2;\
        type *temp_p = realloc(self->data, sizeof(type) * new_capacity);\
        \
        if (temp_p == NULL) {\
            FATAL_ABORT("Realloc Error", __FILE__, __LINE__, "Failed to reallocate AnyVec buffer here!");\
        }\
        self->data = temp_p;\
        self->capacity = new_capacity;\
    }\
    \
    self->data[next_pos] = *value;\
    self->length++;\
}\
\
void AnyVec##_##type##_pop(AnyVec##_##type *self) {\
    const size_t next_pos = self->length - 1;\
    \
    type##_del(self->data + next_pos);\
    type##_dud(self->data + next_pos);\
    self->length--;\
}\
\

// ? Provides a simpler "vector" of trivially copyable / movable types (e.g scalars)
#define STUB_SCALAR_VEC(type)\
typedef struct ts_scalar_vec_##type##_t {\
    type *data;\
    size_t length;\
    size_t capacity;\
} ScalarVec_##type;\
\
void ScalarVec_##type##_dud(ScalarVec_##type *self);\
void ScalarVec_##type##_new(ScalarVec_##type *self, size_t n, type value);\
void ScalarVec_##type##_copy(ScalarVec_##type *self, const ScalarVec_##type *other);\
void ScalarVec_##type##_move(ScalarVec_##type *self, ScalarVec_##type *x);\
void ScalarVec_##type##_del(ScalarVec_##type *self);\
int8_t ScalarVec_##type##_empty(ScalarVec_##type *self);\
size_t ScalarVec_##type##_len(const ScalarVec_##type *self);\
type ScalarVec_##type##_get(const ScalarVec_##type *self, size_t pos);\
type *ScalarVec_##type##_getm(ScalarVec_##type *self, size_t pos);\
void ScalarVec_##type##_push(ScalarVec_##type *self, type value);\
void ScalarVec_##type##_pop(ScalarVec_##type *self);\
\

#define IMPL_SCALAR_VEC(type)\
void ScalarVec_##type##_dud(ScalarVec_##type *self) {\
    type *temp = NULL;\
    DUD_SCALARS_N(type, temp, ANYVEC_DEFAULT_SIZE, __FILE__, __LINE__);\
    \
    self->data = temp;\
    self->length = 0;\
    self->capacity = ANYVEC_DEFAULT_SIZE;\
}\
\
void ScalarVec_##type##_new(ScalarVec_##type *self, size_t n, type value) {\
    type *temp = NULL;\
    DUD_SCALARS_N(type, temp, n, __FILE__, __LINE__);\
    \
    for (size_t filler_i = 0; filler_i < n; filler_i++) {\
        temp[filler_i] = value;\
    }\
    \
    self->data = temp;\
    self->length = 0;\
    self->capacity = n;\
}\
\
void ScalarVec_##type##_copy(ScalarVec_##type *self, const ScalarVec_##type *other) {\
    const size_t other_count = other->length;\
    const size_t other_capacity = other->capacity;\
    \
    type *temp = NULL;\
    DUD_SCALARS_N(type, temp, other_capacity, __FILE__, __LINE__);\
    \
    for (size_t copy_i = 0; copy_i < other_count; copy_i++) {\
        temp[copy_i] = other->data[copy_i];\
    }\
    \
    self->data = temp;\
    self->length = other->length;\
    self->capacity = other->capacity;\
}\
\
void ScalarVec_##type##_move(ScalarVec_##type *self, ScalarVec_##type *x) {\
    if (self == x) {\
        return;\
    }\
    \
    if (ScalarVec_##type##_empty(self) == 0) {\
        ScalarVec_##type##_del(self);\
    }\
    \
    self->data = x->data;\
    x->data = NULL;\
    self->length = x->length;\
    x->length = 0;\
    self->capacity = x->capacity;\
    x->capacity = 0;\
}\
\
void ScalarVec_##type##_del(ScalarVec_##type *self) {\
    if (self->data != NULL) {\
        free(self->data);\
        self->data = NULL;\
    }\
}\
\
int8_t ScalarVec_##type##_empty(ScalarVec_##type *self) {\
    return self->data == NULL || self->length == 0;\
}\
\
size_t ScalarVec_##type##_len(const ScalarVec_##type *self) {\
    return self->length;\
}\
\
type ScalarVec_##type##_get(const ScalarVec_##type *self, size_t pos) {\
    if (pos >= self->length) {\
        FATAL_ABORT("OOB Error", __FILE__, __LINE__, "OOB access of ScalarVec here!")\
    }\
    \
    return self->data[pos];\
}\
\
type *ScalarVec_##type##_getm(ScalarVec_##type *self, size_t pos) {\
    if (pos >= self->length) {\
        FATAL_ABORT("OOB Error", __FILE__, __LINE__, "OOB access of ScalarVec here!")\
    }\
    \
    return self->data + pos;\
}\
\
void ScalarVec_##type##_push(ScalarVec_##type *self, type value) {\
    const size_t next_pos = self->length;\
    const size_t old_capacity = self->capacity;\
    \
    if (next_pos >= old_capacity) {\
        const size_t new_capacity = (old_capacity * 3) / 2;\
        type *temp_p = realloc(self->data, sizeof(type) * new_capacity);\
        \
        if (temp_p == NULL) {\
            FATAL_ABORT("Realloc Error", __FILE__, __LINE__, "Failed to reallocate ScalarVec buffer here!");\
        }\
        self->data = temp_p;\
        self->capacity = new_capacity;\
    }\
    \
    self->data[next_pos] = value;\
    self->length++;\
}\
\
void ScalarVec_##type##_pop(ScalarVec_##type *self) {\
    self->length--;\
}\
\

#endif