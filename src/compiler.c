#include <stdint.h>
#include <stdio.h>

#include "compiler.h"



IMPL_SCALAR_VEC(int)

void ActiveLoop_dud(ActiveLoop *self) {
    ScalarVec_int_dud(&self->loop_breaks);
    ScalarVec_int_dud(&self->loop_continues);
}

void ActiveLoop_copy(ActiveLoop *self, const ActiveLoop *other) {
    ScalarVec_int_copy(&self->loop_breaks, &other->loop_breaks);
    ScalarVec_int_copy(&self->loop_continues, &other->loop_continues);
}

void ActiveLoop_del(ActiveLoop *self) {
    ScalarVec_int_del(&self->loop_breaks);
    ScalarVec_int_del(&self->loop_continues);
}

IMPL_VEC(ActiveLoop)



SymbolInfo make_symbol_info(charspan name_v, int16_t id, Domain d) {
    return (SymbolInfo) {
        .name = name_v,
        .id = id,
        .domain = d
    };
}

SymbolTable make_symbol_table() {
    SymbolInfo *temp_infos = calloc(DEFAULT_SYMBOL_CAPACITY, sizeof(SymbolInfo));

    if (temp_infos != NULL) {   
        return (SymbolTable) {
            .infos = temp_infos,
            .length = 0,
            .capacity = DEFAULT_SYMBOL_CAPACITY,
            .var_alloc_ip = 0,
            .local_argc = 0,
            .next_local_id = 0,     // ? Start from BP since BP holds the callee... OLD + 1 --> new ID.
            .next_global_id = 1     // ? Start from 1 since chunks 1+ are for other procedures. 0 is the implicit main one.
        };
    }

    return (SymbolTable) {
        .infos = NULL,
        .length = 0,
        .capacity = 0,
        .var_alloc_ip = 0,
        .local_argc = 0,
        .next_local_id = 0,
        .next_global_id = 1
    };
}

void symbol_table_del(SymbolTable *self) {
    if (self->infos != NULL) {
        free(self->infos);
        self->infos = NULL;
    }
}

void symbol_table_clear(SymbolTable *self) {
    const int entry_n = self->length;

    for (int i = 0; i < entry_n; i++) {
        self->infos[i] = (SymbolInfo) {
            .name = {
                .data = NULL,
                .length = 0
            },
            .id = 0,
            .domain = symbol_constant
        };
    }

    self->var_alloc_ip = 0;
    self->local_argc = 0,
    self->next_local_id = 0;
}

const SymbolInfo *symbol_table_find(const SymbolTable *symbols, const charspan *s, Domain d) {
    const SymbolInfo *infos_begin = symbols->infos;
    const int entry_n = symbols->length;

    for (int entry_pos = 0; entry_pos < entry_n; entry_pos++) {
        if (charspan_equals_charspan(s, &infos_begin[entry_pos].name) && infos_begin[entry_pos].domain == d) {
            return infos_begin + entry_pos;
        }
    }

    return NULL;
}

const SymbolInfo *symbol_table_push(SymbolTable *symbols, const SymbolInfo *info) {
    const int next_pos = symbols->length;
    const int old_capacity = symbols->capacity;

    if (next_pos >= old_capacity) {
        const int new_capacity = (old_capacity * 3) / 2;
        SymbolInfo *temp_data = realloc(symbols->infos, sizeof(SymbolInfo) * new_capacity);

        if (temp_data != NULL) {   
            symbols->infos = temp_data;
            symbols->capacity = new_capacity;
        } else {
            return NULL;
        }
    }

    symbols->infos[next_pos] = *info;
    symbols->length++;

    return symbols->infos + next_pos;
}



Compiler make_compiler() {
    AnyVec_ActiveLoop temp_loops;
    AnyVec_ActiveLoop_dud(&temp_loops);

    return (Compiler) {
        .globals = make_symbol_table(),
        .locals = make_symbol_table(),
        .loops = temp_loops,
        .curr = (Token) {
            .begin = 0,
            .length = 0,
            .line = 0,
            .tag = tk_unknown
        },
        .prev = (Token) {
            .begin = 0,
            .length = 0,
            .line = 0,
            .tag = tk_unknown
        },
        .errors = 0,
        .chunk_idx = 0,
        .next_native_id = 0,
        .next_str_id = 0,
        .saved_id = 0,
        .flags = cgen_no_flags
    };
}

void compiler_del(Compiler *self) {
    symbol_table_del(&self->globals);
    symbol_table_del(&self->locals);
    AnyVec_ActiveLoop_del(&self->loops);
}

void compiler_map_native(Compiler *self, const charspan *s) {
    SymbolInfo native_fn_info = {
        .name = *s,
        .id = self->next_native_id,
        .domain = symbol_native
    };

    symbol_table_push(&self->globals, &native_fn_info);
    self->next_native_id++;
}

int8_t compiler_peek_past_spaces(const Compiler *self, Lexer *l, const charspan *source, char c) {
    int8_t found = 0;

    for (int i = self->curr.begin + self->curr.length; i < source->length; i++) {
        if (is_space_symbol(source->data[i])) {
            ;
        } else {
            found = source->data[i] == c;
            break;
        }
    }

    return found;
}

int8_t compiler_match_curr(const Compiler *self, TkTag tag) {
    return self->curr.tag == tag;
}

int8_t compiler_match_prev(const Compiler *self, TkTag tag) {
    return self->prev.tag == tag;
}

Token compiler_advance_tk(Compiler *self, Lexer *lexer, const charspan *s) {
    Token temp;

    do {
        temp = lexer_next(lexer, s);

        switch (temp.tag) {
            case tk_spaces: case tk_comment: continue;
            default: break;
        }

        break;
    } while (1);

    return temp;
}

void compiler_eat_tk(Compiler *self, Lexer *lexer, const charspan *s) {
    self->prev = self->curr;
    self->curr = compiler_advance_tk(self, lexer, s);
}

void compiler_warn(Compiler *self, const char *msg, const Token *tk, const charspan *s) {
    self->errors++;
    fprintf(stderr, "Compile Err #%d at line %d:\n\tnote: %s\n", self->errors, tk->line, msg);
}

void compiler_flag_on(Compiler *self, CodegenFlag flag) {
    self->flags |= flag;
}

void compiler_flag_off(Compiler *self, CodegenFlag flag) {
    self->flags &= ~(uint8_t)flag;
}

int8_t compiler_flag_of(const Compiler *self, CodegenFlag flag) {
    switch (flag) {
    case cgen_assign_to: return (self->flags & flag);
    case cgen_access_of: return (self->flags & flag) >> 1;
    case cgen_lhs_local: return (self->flags & flag) >> 2;
    case cgen_lhs_native: return (self->flags & flag) >> 3;
    default: return 0;
    }
}

size_t compiler_emit_op(Compiler *self, Program *pg, Opcode op) {
    AnyVec_Instruction *code_ref = &AnyVec_Chunk_getm(&pg->chunks, self->chunk_idx)->code;
    Instruction temp = {
        .op = op,
        .flag = 0,
        .wide = 0
    };

    AnyVec_Instruction_push(code_ref, &temp);

    return AnyVec_Instruction_len(code_ref);
}

size_t compiler_emit_op_unflagged(Compiler *self, Program *pg, Opcode op, int16_t wide) {
    AnyVec_Instruction *code_ref = &AnyVec_Chunk_getm(&pg->chunks, self->chunk_idx)->code;
    Instruction temp = {
        .op = op,
        .flag = 0,
        .wide = wide
    };

    AnyVec_Instruction_push(code_ref, &temp);

    return AnyVec_Instruction_len(code_ref);
}

size_t compiler_emit_op_flagged(Compiler *self, Program *pg, Opcode op, uint8_t flags, int16_t wide) {
    AnyVec_Instruction *code_ref = &AnyVec_Chunk_getm(&pg->chunks, self->chunk_idx)->code;
    Instruction temp = {
        .op = op,
        .flag = flags,
        .wide = wide
    };

    AnyVec_Instruction_push(code_ref, &temp);

    return AnyVec_Instruction_len(code_ref);
}

void compiler_patch_reserve_inst(Compiler *self, const SymbolTable *scope, Program *pg) {
    const int reserver_ip = scope->var_alloc_ip;
    const int16_t locals_from_params = scope->local_argc;

    // fprintf(stdout, "Debug (compiler_patch_reserve_inst): reserver_ip = %d, locals_from_params = %d, SCOPE.next_local_id = %d\n", reserver_ip, locals_from_params, scope->next_local_id);

    Chunk *current_chunk = AnyVec_Chunk_getm(&pg->chunks, self->chunk_idx);
    current_chunk->code.data[reserver_ip].wide = scope->next_local_id - locals_from_params;
}

const SymbolInfo *compiler_resolve_name(const Compiler *self, const charspan *s) {
    const SymbolInfo *temp = symbol_table_find(&self->globals, s, symbol_native);

    if (temp != NULL) {
        return temp;
    }

    temp = symbol_table_find(&self->globals, s, symbol_func);

    if (temp != NULL) {
        return temp;
    }

    return symbol_table_find(&self->locals, s, symbol_local);
}

const SymbolInfo *compiler_record_function(Compiler *self, Program *pg, const charspan *s, int chunk_id) {
    const SymbolInfo *result = symbol_table_find(&self->globals, s, symbol_func);
    if (result != NULL) {
        return result;
    }

    SymbolInfo new_info = {
        .name = *s,
        .id = self->globals.next_global_id++,
        .domain = symbol_func
    };

    return symbol_table_push(&self->globals, &new_info);
}

const SymbolInfo *compiler_record_local(Compiler *self, Program *pg, const charspan *s) {
    const SymbolInfo *result = symbol_table_find(&self->locals, s, symbol_local);
    if (result != NULL) {
        return result;
    }

    self->locals.next_local_id++;

    SymbolInfo new_info = {
        .name = *s,
        .id = self->locals.next_local_id,
        .domain = symbol_local
    };

    return symbol_table_push(&self->locals, &new_info);
}

const SymbolInfo *compiler_record_constant(Compiler *self, Program *pg, const charspan *s_symbol, Value v) {
    const SymbolInfo *result = symbol_table_find(&self->locals, s_symbol, symbol_constant);
    if (result != NULL) {
        return result;
    }

    // ? C++ equivalent: ... = m_chunks.back().constants;
    AnyVec_Value *constants = &AnyVec_Chunk_getm(&pg->chunks, self->chunk_idx)->constants;
    const int next_const_id = AnyVec_Value_len(constants);

    AnyVec_Value_push(constants, &v);

    SymbolInfo new_info = {
        .name = *s_symbol,
        .id = next_const_id,
        .domain = symbol_constant
    };

    return symbol_table_push(&self->locals, &new_info);
}

const SymbolInfo *compiler_record_string(Compiler *self, Program *pg, const charspan *s) {
    const SymbolInfo *pre_info = symbol_table_find(&self->globals, s, symbol_string);

    if (pre_info != NULL) {
        return pre_info;
    }

    mystr str;
    mystr_res(&str, 10);
    mystr_append_charspan(&str, s, s->length);

    SymbolInfo new_info = {
        .name = (charspan) {
            .data = str.data,
            .length = str.length
        },
        .id = self->next_str_id,
        .domain = symbol_string
    };

    self->next_str_id++;
    AnyVec_mystr_push(&pg->strings, &str);

    return symbol_table_push(&self->globals, &new_info);
}



ActiveLoop *compiler_enter_loop(Compiler *self) {
    ActiveLoop temp;
    ActiveLoop_dud(&temp);

    AnyVec_ActiveLoop_push(&self->loops, &temp);

    return AnyVec_ActiveLoop_getm(&self->loops, AnyVec_ActiveLoop_len(&self->loops) - 1);
}

void compiler_leave_loop(Compiler *self) {
    AnyVec_ActiveLoop_pop(&self->loops);
}

void compiler_track_break_pos(Compiler *self, int pos) {
    ActiveLoop *temp = AnyVec_ActiveLoop_getm(&self->loops, AnyVec_ActiveLoop_len(&self->loops) - 1);

    if (temp != NULL) {
        ScalarVec_int_push(&temp->loop_breaks, pos);
    }
}

void compiler_track_continue_pos(Compiler *self, int pos) {
    ActiveLoop *temp = AnyVec_ActiveLoop_getm(&self->loops, AnyVec_ActiveLoop_len(&self->loops) - 1);

    if (temp != NULL) {
        ScalarVec_int_push(&temp->loop_continues, pos);
    }
}



int8_t compiler_do_list(Compiler *self, Lexer *lexer, const charspan *s, Program *pg) {
    compiler_eat_tk(self, lexer, s); // ? SKIP '['

    int16_t item_count = 0;

    while (!compiler_match_curr(self, tk_eof)) {
        if (compiler_match_curr(self, tk_rbrack)) {
            break;
        }

        if (!compiler_do_or(self, lexer, s, pg)) {
            fprintf(stderr, "Note: See list item #%d around line %d", item_count, self->prev.line);
            return 0;
        }

        item_count++;

        if (compiler_match_curr(self, tk_comma)) {
            compiler_eat_tk(self, lexer, s);
        }
    }
    compiler_eat_tk(self, lexer, s);

    compiler_emit_op_unflagged(self, pg, op_mk_list, item_count);

    return 1;
}

int8_t compiler_do_dict(Compiler *self, Lexer *lexer, const charspan *s, Program *pg) {
    compiler_eat_tk(self, lexer, s); // ? SKIP '{'

    // ? Here, push a blank dictionary to add properties to...
    compiler_emit_op(self, pg, op_mk_dict);

    while (!compiler_match_curr(self, tk_eof)) {
        if (compiler_match_curr(self, tk_rbrace)) {
            break;
        }

        if (!compiler_match_curr(self, tk_string)) {
            compiler_warn(self, "Expected name string for dict field.", &self->curr, s);
            return 0;
        }
        compiler_eat_tk(self, lexer, s);

        charspan temp_key = {
            .data = s->data + self->prev.begin,
            .length = self->prev.length
        };

        const SymbolInfo *key_info = compiler_record_string(self, pg, &temp_key);

        if (key_info == NULL) {
            return 0;
        }
        compiler_emit_op_unflagged(self, pg, op_load_string_k, key_info->id);

        if (!compiler_match_curr(self, tk_colon)) {
            compiler_warn(self, "Expected ':' before field initializer.", &self->curr, s);
            return 0;
        }
        compiler_eat_tk(self, lexer, s);

        if (!compiler_do_or(self, lexer, s, pg)) {
            return 0;
        }

        compiler_emit_op(self, pg, op_set_idx);

        if (!compiler_match_curr(self, tk_semicolon)) {
            compiler_warn(self, "Expected ';' after dict field.", &self->curr, s);
            return 0;
        }
        compiler_eat_tk(self, lexer, s);
    }
    compiler_eat_tk(self, lexer, s);

    return 1;
}

int8_t compiler_do_literal(Compiler *self, Lexer *lexer, const charspan *s, Program *pg) {
    const Token *curr_ref = &self->curr;
    const charspan lexeme = {
        .data = s->data + curr_ref->begin,
        .length = curr_ref->length
    };

    const SymbolInfo *temp_locus = NULL;

    switch (curr_ref->tag) {
        case tk_none:
            compiler_emit_op(self, pg, op_put_none);
            compiler_eat_tk(self, lexer, s);
            return 1;
        case tk_true: case tk_false:
            compiler_emit_op_flagged(self, pg, op_put_bool, curr_ref->tag == tk_true, 0);
            compiler_eat_tk(self, lexer, s);
            return 1;
        case tk_integer:
            temp_locus = compiler_record_constant(
                self,
                pg,
                &lexeme,
                make_value_int(charspan_atoi(&lexeme))
            );
            compiler_eat_tk(self, lexer, s);
            break;
        case tk_real:
            temp_locus = compiler_record_constant(
                self,
                pg,
                &lexeme,
                make_value_real(charspan_atof(&lexeme))
            );
            compiler_eat_tk(self, lexer, s);
            break;
        case tk_string:
            temp_locus = compiler_record_string(
                self,
                pg,
                &lexeme
            );
            compiler_eat_tk(self, lexer, s);
            break;
        case tk_identifier:
            temp_locus = compiler_resolve_name(
                self,
                &lexeme
            );
            compiler_eat_tk(self, lexer, s);
            break;
        case tk_lparen:
            compiler_eat_tk(self, lexer, s);
            if (!compiler_do_or(self, lexer, s, pg)) {
                fprintf(stderr, "Note: See parenthesized expr at around line %d.\n", self->prev.line);
                return 0;
            }
            if (!compiler_match_curr(self, tk_rparen)) {
                compiler_warn(self, "Expected ')' closing parenthesized expr.", &self->curr, s);
                return 0;
            }
            compiler_eat_tk(self, lexer, s);
            return 1;
        case tk_lbrack:
            return compiler_do_list(self, lexer, s, pg);
        case tk_lbrace:
            return compiler_do_dict(self, lexer, s, pg);
        default:
            compiler_warn(self, "Unexpected token in literal, expected none, true, false, or a name.", curr_ref, s);
            return 0;
    }

    if (temp_locus == NULL) {
        compiler_warn(self, "Undeclared name found here.", &self->prev, s);
        return 0;
    }

    // todo: add case for assignment LHS's of table accesses...
    if (self->curr.tag == tk_os_bind_equals) {
        compiler_flag_on(self, cgen_assign_to);

        if (temp_locus->domain == symbol_local) {
            compiler_flag_on(self, cgen_lhs_local);
            self->saved_id = temp_locus->id;
            return 1;
        }
    }

    switch (temp_locus->domain) {
        case symbol_constant:
            compiler_emit_op_unflagged(self, pg, op_put_k, temp_locus->id);
            break;
        case symbol_local:
            compiler_emit_op_unflagged(self, pg, op_load_local, temp_locus->id);
            break;
        case symbol_func:
            // ? NOTE: The pushed ID is for a global procedure, VM or native.
            compiler_emit_op_unflagged(self, pg, op_load_imm_gid, temp_locus->id);
            break;
        case symbol_native:
            compiler_flag_on(self, cgen_lhs_native);
            // ? NOTE: The pushed ID is for a global procedure, VM or native.
            compiler_emit_op_unflagged(self, pg, op_load_imm_gid, temp_locus->id);
            break;
        case symbol_string:
            compiler_emit_op_unflagged(self, pg, op_load_string_k, temp_locus->id);
            break;
        default:
            break;
    }

    return 1;
}

int8_t compiler_do_lhs(Compiler *self, Lexer *lexer, const charspan *s, Program *pg) {
    // int8_t is_on_assignment_lhs = compiler_flag_of(self, cgen_assign_to);

    if (!compiler_do_literal(self, lexer, s, pg)) {
        fprintf(stderr, "Note: See LHS of access-of expression at line %d.\n", self->curr.line);
        return 0;
    }

    if (!compiler_match_prev(self, tk_identifier)) {
        if (!compiler_match_curr(self, tk_os_bind_equals)) {
            compiler_flag_off(self, cgen_assign_to);
        }

        return 1;
    } else if (compiler_match_curr(self, tk_os_bind_equals)) {
        return 1;
    }

    if (compiler_flag_of(self, cgen_lhs_local)) {
        compiler_emit_op_flagged(self, pg, op_load_local, 0, self->saved_id);
        compiler_flag_off(self, cgen_lhs_local);
        compiler_flag_on(self, cgen_access_of);
    }

    while (!compiler_match_curr(self, tk_eof)) {
        if (!compiler_match_curr(self, tk_lbrack)) {
            break;
        }
        compiler_eat_tk(self, lexer, s);

        if (!compiler_do_or(self, lexer, s, pg)) {
            fprintf(stderr, "Note: See RHS of access-of expression at line %d.\n", self->curr.line);
            return 0;
        }

        if (compiler_match_curr(self, tk_rbrack)) {
            compiler_eat_tk(self, lexer, s);
        }

        if (compiler_match_curr(self, tk_os_bind_equals)) {
            compiler_flag_on(self, cgen_assign_to);
        } else {
            compiler_emit_op(self, pg, op_get_idx);
        }
    }

    compiler_flag_on(self, cgen_access_of);

    return 1;
}

int8_t compiler_do_call(Compiler *self, Lexer *lexer, const charspan *s, Program *pg) {
    const Token callee_name = self->curr;
    int arg_count = 0;

    if (!compiler_do_lhs(self, lexer, s, pg)) {
        fprintf(stderr, "Note: See call at line %d.\n", self->curr.line);
        return 0;
    }

    int8_t outer_callee_is_native = compiler_flag_of(self, cgen_lhs_native);

    if (!compiler_match_curr(self, tk_lparen)) {
        return 1;
    }
    compiler_eat_tk(self, lexer, s);

    while (!compiler_match_curr(self, tk_eof)) {
        if (compiler_match_curr(self, tk_rparen)) {
            break;
        }

        compiler_flag_off(self, cgen_lhs_native);

        if (!compiler_do_compare(self, lexer, s, pg)) {
            fprintf(stderr, "Note: See call at line %d.\n", callee_name.line);
            return 0;
        }

        arg_count++;

        if (compiler_match_curr(self, tk_comma)) {
            compiler_eat_tk(self, lexer, s);
        }
    }

    compiler_eat_tk(self, lexer, s);

    if (outer_callee_is_native) {
        compiler_flag_on(self, cgen_lhs_native);
    }

    compiler_emit_op_flagged(self, pg, op_call, compiler_flag_of(self, cgen_lhs_native), arg_count);
    compiler_flag_off(self, cgen_lhs_native);

    return 1;
}

int8_t compiler_do_unary(Compiler *self, Lexer *lexer, const charspan *s, Program *pg) {
    if (compiler_match_curr(self, tk_os_nullish)) {
        compiler_eat_tk(self, lexer, s);

        if (!compiler_do_call(self, lexer, s, pg)) {
            return 0;
        }

        compiler_emit_op(self, pg, op_chk_none);

        return 1;
    }

    return compiler_do_call(self, lexer, s, pg);
}

int8_t compiler_do_null_coal(Compiler *self, Lexer *lexer, const charspan *s, Program *pg) {
    if (!compiler_do_unary(self, lexer, s, pg)) {
        return 0;
    }

    if (!compiler_match_curr(self, tk_os_nullcol)) {
        return 1;
    }
    compiler_eat_tk(self, lexer, s);

    compiler_emit_op_unflagged(self, pg, op_chk_none, 0);

    const uint16_t jump_nil_pos = pg->chunks.data[self->chunk_idx].code.length;
    // ? Pop NIL value and the leftover null-flag from the stack to save space.
    compiler_emit_op_unflagged(self, pg, op_jmp_false, 0);
    compiler_emit_op_flagged(self, pg, op_pop, 1, 0);

    if (!compiler_do_unary(self, lexer, s, pg)) {
        return 0;
    }

    const uint16_t jump_past_pop_pos = pg->chunks.data[self->chunk_idx].code.length;
    compiler_emit_op_flagged(self, pg, op_jmp, 0, 0);

    const uint16_t jump_nil_end_pos = pg->chunks.data[self->chunk_idx].code.length;
    pg->chunks.data[self->chunk_idx].code.data[jump_nil_pos].wide = jump_nil_end_pos - jump_nil_pos;
    // ? Pop null-check false from the stack, exposing the LHS value for other code.
    compiler_emit_op_flagged(self, pg, op_pop, 1, 0);

    // ? Patch pop-skipping jump to avoid incorrectly popping a non-NIL RHS.
    pg->chunks.data[self->chunk_idx].code.data[jump_past_pop_pos].wide = (jump_nil_end_pos + 1) - jump_past_pop_pos;

    return 1;
}

int8_t compiler_do_factor(Compiler *self, Lexer *lexer, const charspan *s, Program *pg) {
    if (!compiler_do_null_coal(self, lexer, s, pg)) {
        return 0;
    }

    Opcode op;

    while (!compiler_match_curr(self, tk_eof)) {
        const Token curr = self->curr;

        switch (curr.tag) {
            case tk_os_times: op = op_mul; break;
            case tk_os_slash: op = op_div; break;
            default: op = op_nop; break;
        }

        if (op == op_nop) {
            break;
        }
        compiler_eat_tk(self, lexer, s);

        if (!compiler_do_null_coal(self, lexer, s, pg)) {
            return 0;
        }
        compiler_emit_op(self, pg, op);
    }

    return 1;
}

int8_t compiler_do_sum(Compiler *self, Lexer *lexer, const charspan *s, Program *pg) {
    if (!compiler_do_factor(self, lexer, s, pg)) {
        return 0;
    }

    Opcode op;

    while (!compiler_match_curr(self, tk_eof)) {
        const Token curr = self->curr;

        switch (curr.tag) {
            case tk_os_plus: op = op_add; break;
            case tk_os_minus: op = op_sub; break;
            default: op = op_nop; break;
        }

        if (op == op_nop) {
            break;
        }
        compiler_eat_tk(self, lexer, s);

        if (!compiler_do_factor(self, lexer, s, pg)) {
            return 0;
        }
        compiler_emit_op(self, pg, op);
    }

    return 1;
}

int8_t compiler_do_equality(Compiler *self, Lexer *lexer, const charspan *s, Program *pg) {
    if (!compiler_do_sum(self, lexer, s, pg)) {
        return 0;
    }

    Opcode op;

    while (!compiler_match_curr(self, tk_eof)) {
        const Token curr = self->curr;

        switch (curr.tag) {
            case tk_os_equals: op = op_eq; break;
            case tk_os_bang_equals: op = op_ne; break;
            default: op = op_nop; break;
        }

        if (op == op_nop) {
            break;
        }
        compiler_eat_tk(self, lexer, s);

        if (!compiler_do_sum(self, lexer, s, pg)) {
            return 0;
        }
        compiler_emit_op(self, pg, op);
    }

    return 1;
}

int8_t compiler_do_compare(Compiler *self, Lexer *lexer, const charspan *s, Program *pg) {
    if (!compiler_do_equality(self, lexer, s, pg)) {
        return 0;
    }

    Opcode op;

    while (!compiler_match_curr(self, tk_eof)) {
        const Token curr = self->curr;

        switch (curr.tag) {
            case tk_os_lesser: op = op_lt; break;
            case tk_os_greater: op = op_gt; break;
            default: op = op_nop; break;
        }

        if (op == op_nop) {
            break;
        }
        compiler_eat_tk(self, lexer, s);

        if (!compiler_do_equality(self, lexer, s, pg)) {
            return 0;
        }
        compiler_emit_op(self, pg, op);
    }

    return 1;
}

int8_t compiler_do_and(Compiler *self, Lexer *lexer, const charspan *s, Program *pg) {
    if (!compiler_do_compare(self, lexer, s, pg)) {
        return 0;
    }

    if (!compiler_match_curr(self, tk_os_and)) {
        return 1;
    }
    compiler_eat_tk(self, lexer, s);

    const uint16_t falsy_jmp_pos = pg->chunks.data[self->chunk_idx].code.length;
    compiler_emit_op_flagged(self, pg, op_jmp_false, 0, 0);
    compiler_emit_op_flagged(self, pg, op_pop, 1, 0); // ? Pop LHS if true, keeping our VM's "single result value" invariant.

    if (!compiler_do_compare(self, lexer, s, pg)) {
        return 0;
    }

    const uint16_t falsy_jmp_end = pg->chunks.data[self->chunk_idx].code.length;
    compiler_emit_op(self, pg, op_nop);

    pg->chunks.data[self->chunk_idx].code.data[falsy_jmp_pos].wide = falsy_jmp_end - falsy_jmp_pos;

    return 1;
}

int8_t compiler_do_or(Compiler *self, Lexer *lexer, const charspan *s, Program *pg) {
    if (!compiler_do_and(self, lexer, s, pg)) {
        return 0;
    }

    if (!compiler_match_curr(self, tk_os_or)) {
        return 1;
    }
    compiler_eat_tk(self, lexer, s);

    const uint16_t truthy_jmp_pos = pg->chunks.data[self->chunk_idx].code.length;
    compiler_emit_op_flagged(self, pg, op_jmp_if, 0, 0);
    compiler_emit_op_flagged(self, pg, op_pop, 1, 0); // ? Pop LHS if true, keeping our VM's "single result value" invariant.

    if (!compiler_do_and(self, lexer, s, pg)) {
        return 0;
    }

    const uint16_t truthy_jmp_end = pg->chunks.data[self->chunk_idx].code.length;
    compiler_emit_op(self, pg, op_nop);

    pg->chunks.data[self->chunk_idx].code.data[truthy_jmp_pos].wide = truthy_jmp_end - truthy_jmp_pos;

    return 1;
}

int8_t compiler_do_vars(Compiler *self, Lexer *lexer, const charspan *s, Program *pg) {
    compiler_eat_tk(self, lexer, s); // ? skip LET

    while (!compiler_match_curr(self, tk_eof)) {
        if (compiler_match_curr(self, tk_semicolon)) {
            break;
        } else if (!compiler_match_curr(self, tk_identifier)) {
            compiler_warn(self, "Expected name in variable declaration here.", &self->curr, s);
            return 0;
        }

        const Token var_name = self->curr;
        const charspan raw_name = {
            .data = s->data + var_name.begin,
            .length = var_name.length
        };

        compiler_eat_tk(self, lexer, s);

        const SymbolInfo *var_locus = compiler_record_local(self, pg, &raw_name);

        if (!compiler_match_curr(self, tk_colon)) {
            compiler_warn(self, "Expected ':' before variable initializer.", &self->curr, s);
            return 0;
        }
        compiler_eat_tk(self, lexer, s);

        if (!compiler_do_or(self, lexer, s, pg)) {
            return 0;
        }
        compiler_emit_op_unflagged(self, pg, op_store_local, var_locus->id);

        if (compiler_match_curr(self, tk_comma)) {
            compiler_eat_tk(self, lexer, s);
        }
    }

    compiler_eat_tk(self, lexer, s);

    return 1;
}

int8_t compiler_do_ifs(Compiler *self, Lexer *lexer, const charspan *s, Program *pg) {
    compiler_eat_tk(self, lexer, s); // ? skip IF

    const int cmp_line = self->curr.line;
    if (!compiler_do_or(self, lexer, s, pg)) {
        return 0;
    }

    const uint16_t jump_else_pos = pg->chunks.data[self->chunk_idx].code.length;
    compiler_emit_op_flagged(self, pg, op_jmp_false, 0, 0);
    compiler_emit_op_flagged(self, pg, op_pop, 1, 0);

    if (!compiler_do_block(self, lexer, s, pg)) {
        return 0;
    }

    if (!compiler_match_curr(self, tk_keyword_else)) {
        const uint16_t skip_tbody_pos = pg->chunks.data[self->chunk_idx].code.length;
        compiler_emit_op_unflagged(self, pg, op_nop, 0);

        pg->chunks.data[self->chunk_idx].code.data[jump_else_pos].wide = skip_tbody_pos - jump_else_pos;

        return 1;
    }

    // * BEGIN ELSE clause ... * //
    compiler_eat_tk(self, lexer, s); // ? consume leading 'ELSE' of ELSE body

    const uint16_t jump_skip_else_pos = pg->chunks.data[self->chunk_idx].code.length;
    compiler_emit_op_unflagged(self, pg, op_jmp, 0);

    const uint16_t begin_else_pos = jump_skip_else_pos + 1;
    if (!compiler_do_nestable_stmt(self, lexer, s, pg)) {
        fprintf(stderr, "Note: See else-clause in the falsy-body around line %d.\n", self->curr.line);
        return 0;
    }

    const uint16_t end_ifs_pos = pg->chunks.data[self->chunk_idx].code.length;
    compiler_emit_op(self, pg, op_nop);

    pg->chunks.data[self->chunk_idx].code.data[jump_else_pos].wide = begin_else_pos - jump_else_pos;
    pg->chunks.data[self->chunk_idx].code.data[jump_skip_else_pos].wide = end_ifs_pos - jump_skip_else_pos;

    return 1;
}

int8_t compiler_do_while(Compiler *self, Lexer *lexer, const charspan *s, Program *pg) {
    compiler_eat_tk(self, lexer, s); // ? skip WHILE

    ActiveLoop *loop_data = compiler_enter_loop(self);

    const uint16_t while_check_pos = pg->chunks.data[self->chunk_idx].code.length;
    if (!compiler_do_or(self, lexer, s, pg)) {
        fprintf(stderr, "Note: See while-loop condition around line %d.\n", self->curr.line);
        compiler_leave_loop(self);
        return 0;
    }

    const uint16_t while_jmp_out_pos = pg->chunks.data[self->chunk_idx].code.length;
    compiler_emit_op_flagged(self, pg, op_jmp_false, 0, 0); // ? flags = 0 ==> forward jump applies!

    if (!compiler_do_block(self, lexer, s, pg)) {
        fprintf(stderr, "Note: See while-body around line %d.\n", self->curr.line);
        compiler_leave_loop(self);
        return 0;
    }

    // ? 1. Patch 2 main loop jumps: the repeat & exit...
    const uint16_t while_jmp_back_pos = pg->chunks.data[self->chunk_idx].code.length;
    compiler_emit_op_flagged(self, pg, op_jmp, 1, 0); // ? flags = 1 ==> backwards jump applies!
    const uint16_t while_exit_pos = pg->chunks.data[self->chunk_idx].code.length;
    compiler_emit_op_flagged(self, pg, op_pop, 1, 0); // ? pop off check after loop quits WHEN it's FALSE

    pg->chunks.data[self->chunk_idx].code.data[while_jmp_back_pos].wide = while_jmp_back_pos - while_check_pos;
    pg->chunks.data[self->chunk_idx].code.data[while_jmp_out_pos].wide = while_exit_pos - while_jmp_out_pos;

    // ? 2. Patch loop breaks and continues...
    const int break_count = ScalarVec_int_len(&loop_data->loop_breaks);
    for (int break_i = 0; break_i < break_count; break_i++) {
        const int break_pos = ScalarVec_int_get(&loop_data->loop_breaks, break_i);

        pg->chunks.data[self->chunk_idx].code.data[break_pos].wide = while_exit_pos - break_pos;
    }

    const int continue_count = ScalarVec_int_len(&loop_data->loop_continues);
    for (int continue_i = 0; continue_i < continue_count; continue_i++) {
        const int continue_pos = ScalarVec_int_get(&loop_data->loop_continues, continue_i);

        pg->chunks.data[self->chunk_idx].code.data[continue_pos].wide = continue_pos - while_check_pos;
        pg->chunks.data[self->chunk_idx].code.data[continue_pos].flag = 1;
    }

    compiler_leave_loop(self);
    return 1;
}

/*
 * ### EXAMPLE:
 * `FOR i : 0, i < 10, i := i + 1; :`
 *
 * `print(i);`
 *
 * `END`
 *
 */
int8_t compiler_do_for(Compiler *self, Lexer *lexer, const charspan *s, Program *pg) {
    compiler_eat_tk(self, lexer, s); // ? skip FOR

    ActiveLoop *loop_data = compiler_enter_loop(self);

    if (!compiler_match_curr(self, tk_identifier)) {
        compiler_warn(self, "Expected name for counter variable in C-style for loop here.", &self->curr, s);
        fprintf(stderr, "\tNote: See line %d.\n", self->curr.line);

        compiler_leave_loop(self);
        return 0;
    }
    compiler_eat_tk(self, lexer, s);

    const int counter_name_line = self->prev.line;
    charspan counter_name = {
        .data = s->data + self->prev.begin,
        .length = self->prev.length
    };
    const SymbolInfo *counter_info = compiler_resolve_name(self, &counter_name);

    if (!compiler_match_curr(self, tk_colon)) {
        compiler_warn(self, "Expected ':' after counter variable in C-style FOR loop here.", &self->curr, s);
        fprintf(stderr, "\tNote: See line %d.\n", self->curr.line);

        compiler_leave_loop(self);
        return 0;
    }
    compiler_eat_tk(self, lexer, s);

    if (!compiler_do_or(self, lexer, s, pg)) {
        fprintf(stderr, "\tNote: invalid initializer of FOR loop counter around line %d.\n", self->prev.line);

        compiler_leave_loop(self);
        return 0;
    }

    if (counter_info->domain == symbol_local) {
        compiler_emit_op_unflagged(self, pg, op_store_local, counter_info->id);
    } else {
        compiler_warn(self, "Invalid name of loop counter around here- shadows a similar non-local name.", &self->prev, s);
        fprintf(stderr, "\tNote: see line %d.\n", counter_name_line);
        return 0;
    }

    if (!compiler_match_curr(self, tk_comma)) {
        compiler_warn(self, "Expected ',' after counter in FOR loop here.", &self->curr, s);
        fprintf(stderr, "\tNote: see line %d.\n", self->curr.line);
        return 0;
    }
    compiler_eat_tk(self, lexer, s);

    const uint16_t loop_begin_pos = pg->chunks.data[self->chunk_idx].code.length;
    if (!compiler_do_or(self, lexer, s, pg)) {
        fprintf(stderr, "\tNote: invalid condition of FOR loop around line %d.\n", self->prev.line);

        compiler_leave_loop(self);
        return 0;
    }
    const uint16_t loop_jump_out_pos = pg->chunks.data[self->chunk_idx].code.length;
    compiler_emit_op_unflagged(self, pg, op_jmp_false, 0);

    if (!compiler_match_curr(self, tk_comma)) {
        compiler_warn(self, "Expected ',' after check in FOR loop here.", &self->curr, s);
        fprintf(stderr, "\tNote: see line %d.\n", self->curr.line);
        return 0;
    }
    compiler_eat_tk(self, lexer, s);

    const uint16_t loop_skip_update_pos = pg->chunks.data[self->chunk_idx].code.length;
    compiler_emit_op_unflagged(self, pg, op_jmp, 0);

    const uint16_t loop_start_update_pos = pg->chunks.data[self->chunk_idx].code.length;
    compiler_emit_op_unflagged(self, pg, op_nop, 0);
    if (!compiler_do_expr_stmt(self, lexer, s, pg)) {
        fprintf(stderr, "\tNote: invalid update clause of FOR loop around line %d.\n", self->prev.line);

        compiler_leave_loop(self);
        return 0;
    }
    // ? Emit & easily patch "loop-repeater" jump AFTER update evaluations...
    const uint16_t loop_repeater_pos = pg->chunks.data[self->chunk_idx].code.length;
    compiler_emit_op_flagged(self, pg, op_jmp, 1, loop_repeater_pos - loop_begin_pos);
    // ? Patch the jump at loop_skip_update_pos since the update clause was fully resolved here...
    pg->chunks.data[self->chunk_idx].code.data[loop_skip_update_pos].wide = loop_repeater_pos + 1 - loop_skip_update_pos;

    if (!compiler_do_block(self, lexer, s, pg)) {
        fprintf(stderr, "\tNote: invalid body of FOR loop around line %d.\n", self->prev.line);

        compiler_leave_loop(self);
        return 0;
    }

    // ? Patch returning jump to update clause after body evaluation...
    const uint16_t loop_jump_to_update_pos = pg->chunks.data[self->chunk_idx].code.length;
    compiler_emit_op_flagged(self, pg, op_jmp, 1, loop_jump_to_update_pos - loop_start_update_pos);

    // ? Patch checked-jump to loop exiting position here since the body is resolved then...
    const uint16_t loop_end_pos = pg->chunks.data[self->chunk_idx].code.length;
    compiler_emit_op(self, pg, op_nop);
    pg->chunks.data[self->chunk_idx].code.data[loop_jump_out_pos].wide = loop_end_pos - loop_jump_out_pos;

    // ? Patch main loop jumps & any break / continue jumps...
    const int break_count = ScalarVec_int_len(&loop_data->loop_breaks);
    for (int break_i = 0; break_i < break_count; break_i++) {
        const int break_pos = ScalarVec_int_get(&loop_data->loop_breaks, break_i);

        pg->chunks.data[self->chunk_idx].code.data[break_pos].wide = loop_end_pos - break_pos;
    }

    const int continue_count = ScalarVec_int_len(&loop_data->loop_continues);
    for (int continue_i = 0; continue_i < continue_count; continue_i++) {
        const int continue_pos = ScalarVec_int_get(&loop_data->loop_continues, continue_i);

        pg->chunks.data[self->chunk_idx].code.data[continue_pos].wide = continue_pos - loop_start_update_pos;
        pg->chunks.data[self->chunk_idx].code.data[continue_pos].flag = 1;
    }

    compiler_leave_loop(self);
    return 1;
}

int8_t compiler_do_break(Compiler *self, Lexer *lexer, const charspan *s, Program *pg) {
    compiler_eat_tk(self, lexer, s); // ? skip BREAK

    if (AnyVec_ActiveLoop_empty(&self->loops)) {
        compiler_warn(self, "Expected an enclosing loop for a 'BREAK' statement here.", &self->prev, s);
        fprintf(stderr, "\tNote: see line %d.\n", self->prev.line);
        return 0;
    }

    if (!compiler_match_curr(self, tk_semicolon)) {
        compiler_warn(self, "Expected a ';' closing a 'BREAK' statement here.", &self->prev, s);
        fprintf(stderr, "\tNote: see line %d.\n", self->prev.line);
        return 0;
    }
    compiler_eat_tk(self, lexer, s);

    const int break_bc_pos = pg->chunks.data[self->chunk_idx].code.length;
    compiler_track_break_pos(self, break_bc_pos);
    compiler_emit_op_unflagged(self, pg, op_jmp, 0);

    return 1;
}

int8_t compiler_do_continue(Compiler *self, Lexer *lexer, const charspan *s, Program *pg) {
    compiler_eat_tk(self, lexer, s); // ? skip BREAK

    if (AnyVec_ActiveLoop_empty(&self->loops)) {
        compiler_warn(self, "Expected an enclosing loop for a 'CONTINUE' statement here.", &self->prev, s);
        fprintf(stderr, "\tNote: see line %d.\n", self->prev.line);
        return 0;
    }

    if (!compiler_match_curr(self, tk_semicolon)) {
        compiler_warn(self, "Expected a ';' closing a 'BREAK' statement here.", &self->prev, s);
        fprintf(stderr, "\tNote: see line %d.\n", self->prev.line);
        return 0;
    }
    compiler_eat_tk(self, lexer, s);

    const int continue_bc_pos = pg->chunks.data[self->chunk_idx].code.length;
    compiler_track_continue_pos(self, continue_bc_pos);
    compiler_emit_op_unflagged(self, pg, op_jmp, 0);

    return 1;
}

int8_t compiler_do_ret(Compiler *self, Lexer *lexer, const charspan *s, Program *pg) {
    compiler_eat_tk(self, lexer, s); // ? skip RET

    compiler_flag_off(self, cgen_assign_to);
    if (!compiler_do_or(self, lexer, s, pg)) {
        fprintf(stderr, "Note: See return-result expression at line %d.\n", self->curr.line);
        fprintf(stderr, "\tNote: see line %d.\n", self->prev.line);
        return 0;
    }
    compiler_emit_op(self, pg, op_ret);

    if (!compiler_match_curr(self, tk_semicolon)) {
        compiler_warn(self, "Expected ';' after return statement.", &self->curr, s);
        fprintf(stderr, "\tNote: see line %d.\n", self->curr.line);
        return 0;
    }
    compiler_eat_tk(self, lexer, s);

    return 1;
}

int8_t compiler_do_expr_stmt(Compiler *self, Lexer *lexer, const charspan *s, Program *pg) {
    // ? Process / emit LHS first...
    compiler_flag_on(self, cgen_assign_to);
    if (!compiler_do_or(self, lexer, s, pg)) {
        compiler_flag_off(self, cgen_assign_to);
        fprintf(stderr, "See expr-stmt at line %d.\n", self->curr.line);
        return 0;
    }

    if (compiler_match_curr(self, tk_os_bind_equals)) {
        compiler_eat_tk(self, lexer, s);

        const int8_t lhs_has_target_local_old = compiler_flag_of(self, cgen_lhs_local);
        const int8_t lhs_had_accesses_of = compiler_flag_of(self, cgen_access_of);
        const int8_t lhs_is_assignable_old = compiler_flag_of(self, cgen_assign_to);

        compiler_flag_off(self, cgen_lhs_local);
        compiler_flag_off(self, cgen_access_of);
        compiler_flag_off(self, cgen_assign_to);

        if (!compiler_do_or(self, lexer, s, pg)) {
            fprintf(stderr, "Note: see RHS of assignment at line %d.\n", self->curr.line);
            return 0;
        }

        if (lhs_is_assignable_old) {
            compiler_flag_on(self, cgen_assign_to);
        }

        if (lhs_had_accesses_of) {
            compiler_flag_on(self, cgen_access_of);
        }

        if (lhs_has_target_local_old) {
            compiler_flag_on(self, cgen_lhs_local);
        }

        // ? If we have consumed only a name = <value>, emit a simple update of that local slot.
        if (compiler_flag_of(self, cgen_lhs_local)) {
            compiler_emit_op_flagged(self, pg, op_store_local, 0, self->saved_id);
            compiler_flag_off(self, cgen_lhs_local);
        } else if (compiler_flag_of(self, cgen_access_of)) {
            compiler_emit_op(self, pg, op_set_idx);
            compiler_flag_off(self, cgen_access_of);
        } else {
            compiler_warn(self, "Invalid LHS of assignment, expected a name or key-access expression.\n", &self->prev, s);
            fprintf(stderr, "\tNote: see line %d.\n", self->prev.line);
            compiler_flag_off(self, cgen_lhs_local);
            compiler_flag_off(self, cgen_access_of);
            return 0;
        }

        compiler_flag_off(self, cgen_lhs_local);
        compiler_flag_off(self, cgen_access_of);
        compiler_flag_off(self, cgen_assign_to);
    }

    if (!compiler_match_curr(self, tk_semicolon)) {
        compiler_warn(self, "Expected ';' after expr-stmt.", &self->curr, s);
        fprintf(stderr, "\tNote: see line %d.\n", self->curr.line);
        return 0;
    }
    compiler_eat_tk(self, lexer, s);

    return 1;
}

int8_t compiler_do_nestable_stmt(Compiler *self, Lexer *lexer, const charspan *s, Program *pg) {
    switch (self->curr.tag) {
    case tk_keyword_let:
        return compiler_do_vars(self, lexer, s, pg);
    case tk_keyword_if:
        return compiler_do_ifs(self, lexer, s, pg);
    case tk_keyword_while:
        return compiler_do_while(self, lexer, s, pg);
    case tk_keyword_for:
        return compiler_do_for(self, lexer, s, pg);
    case tk_keyword_break:
        return compiler_do_break(self, lexer, s, pg);
    case tk_keyword_continue:
        return compiler_do_continue(self, lexer, s, pg);
    case tk_keyword_ret:
        return compiler_do_ret(self, lexer, s, pg);
    case tk_colon:
        return compiler_do_block(self, lexer, s, pg);
    default:
        return compiler_do_expr_stmt(self, lexer, s, pg);
    }
}

int8_t compiler_do_block(Compiler *self, Lexer *lexer, const charspan *s, Program *pg) {
    if (!compiler_match_curr(self, tk_colon)) {
        compiler_warn(self, "Expected ':' starting block.", &self->curr, s);
        fprintf(stderr, "\tNote: see line %d.\n", self->curr.line);
        return 0;
    }
    compiler_eat_tk(self, lexer, s);

    while (!compiler_match_curr(self, tk_eof)) {
        if (compiler_match_curr(self, tk_keyword_end)) {
            break;
        }

        const int stmt_line = self->curr.line;
        if (!compiler_do_nestable_stmt(self, lexer, s, pg)) {
            fprintf(stderr, "Note: See nested statement in block body around line %d.\n", stmt_line);
            return 0;
        }
    }
    compiler_eat_tk(self, lexer, s);

    return 1;
}

int8_t compiler_do_func(Compiler *self, Lexer *lexer, const charspan *s, Program *pg) {
    compiler_eat_tk(self, lexer, s); // ? skip FUN

    if (!compiler_match_curr(self, tk_identifier)) {
        compiler_warn(self, "Expected name for FUN declaration.", &self->curr, s);
        fprintf(stderr, "\tNote: see line %d.\n", self->curr.line);
        return 0;
    }
    compiler_eat_tk(self, lexer, s);

    const charspan name_lexeme = {
        .data = s->data + self->prev.begin,
        .length = self->prev.length
    };
    Chunk temp_chunk;
    Chunk_dud(&temp_chunk);
    AnyVec_Chunk_push(&pg->chunks, &temp_chunk);

    // ? Put index to this procedure's code chunk and reset it to 0 again once we return to top-level code... This works since there's only 1 global scope & 1 nested, local scope per procedure.
    const int16_t old_chunk_idx = 0;
    self->chunk_idx = pg->chunks.length - 1;
    compiler_record_function(self, pg, &name_lexeme, self->chunk_idx);

    if (!compiler_match_curr(self, tk_lparen)) {
        return 0;
    }
    compiler_eat_tk(self, lexer, s);

    while (!compiler_match_curr(self, tk_eof)) {
        if (compiler_match_curr(self, tk_rparen)) {
            break;
        } else if (!compiler_match_curr(self, tk_identifier)) {
            compiler_warn(self, "Expected name in params list here.", &self->curr, s);
            fprintf(stderr, "\tNote: see line %d.\n", self->curr.line);
            return 0;
        }
        
        // ? Eat checked identifier token here, as it's simpler to process it as self->curr.
        const charspan param_name = {
            .data = s->data + self->curr.begin,
            .length = self->curr.length
        };
        
        compiler_record_local(self, pg, &param_name);
        self->locals.local_argc++;

        compiler_eat_tk(self, lexer, s);

        if (compiler_match_curr(self, tk_comma)) {
            compiler_eat_tk(self, lexer, s);
        }
    }
    compiler_eat_tk(self, lexer, s);

    self->locals.var_alloc_ip = pg->chunks.data[self->chunk_idx].code.length;
    compiler_emit_op_unflagged(self, pg, op_reserve, 0);

    if (!compiler_do_block(self, lexer, s, pg)) {
        fprintf(stderr, "Note: See in FUN declaration around line %d.\n", self->curr.line);
        return 0;
    } else {
        compiler_patch_reserve_inst(self, &self->locals, pg);
        symbol_table_clear(&self->locals);
        self->chunk_idx = 0;
    }

    return 1;
}

int8_t compiler_do_stmt(Compiler *self, Lexer *lexer, const charspan *s, Program *pg) {
    switch (self->curr.tag) {
    case tk_keyword_let:
        return compiler_do_vars(self, lexer, s, pg);
    case tk_keyword_if:
        return compiler_do_ifs(self, lexer, s, pg);
    case tk_keyword_while:
        return compiler_do_while(self, lexer, s, pg);
    case tk_keyword_for:
        return compiler_do_for(self, lexer, s, pg);
    case tk_keyword_ret:
        return compiler_do_ret(self, lexer, s, pg);
    case tk_keyword_fun:
        return compiler_do_func(self, lexer, s, pg);
    default:
        return compiler_do_expr_stmt(self, lexer, s, pg);
    }
}

int8_t compiler_do_source(Compiler *self, Lexer *lexer, const charspan *s, Program *pg) {
    compiler_eat_tk(self, lexer, s); // ? remove the unknown token placeholder by getting the 1st token into self->curr... this is needed for correct parsing --> bytecode

    // ! IMPORTANT: push an empty bytecode chunk so that an OOB terminate doesn't happen when ~ L248 tries to push a constant, etc. for AnyVec_<type>.
    Chunk temp;
    Chunk_new(&temp); // ? initialize empty chunk
    AnyVec_Chunk_push(&pg->chunks, &temp); // ? copy the empty chunk into this Vec, but don't touch temp again... just did scuffed destructive moves??

    self->locals.var_alloc_ip = pg->chunks.data[self->chunk_idx].code.length;
    compiler_emit_op_unflagged(self, pg, op_reserve, 0);

    while (!compiler_match_curr(self, tk_eof)) {
        if (!compiler_do_stmt(self, lexer, s, pg)) {
            // ? Recover parsing by skipping to a LET declaration as a synchronization point.
            while (!compiler_match_curr(self, tk_eof)) {
                if (compiler_match_curr(self, tk_keyword_let)) {
                    break;
                }

                compiler_eat_tk(self, lexer, s);
            }
        }
    }

    compiler_emit_op(self, pg, op_ret); // ! NOTE: emit a redundant RET in case the user forgets one- otherwise the VM reads an invalid IP!
    compiler_patch_reserve_inst(self, &self->locals, pg);

    pg->entry_id = 0;
    
    fprintf(stderr, "Compilation finished with \x1b[1;31m%d\x1b[0m errors.\n\n", self->errors);

    return self->errors == 0;
}
