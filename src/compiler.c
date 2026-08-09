#include <stdint.h>
#include <stdio.h>

#include "compiler.h"



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
        .saved_id = 0
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
    fprintf(stderr, "Compile Err #%d at line %d:\n\tNote: %s\n", self->errors, tk->line, msg);
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



uint8_t compiler_do_list(Compiler *self, Lexer *lexer, const charspan *s, Program *pg, CompHints hints) {
    compiler_eat_tk(self, lexer, s); // ? SKIP '['

    int16_t item_count = 0;

    while (!compiler_match_curr(self, tk_eof)) {
        if (compiler_match_curr(self, tk_rbrack)) {
            break;
        }

        const CompHints item_hints = compiler_do_or(self, lexer, s, pg, hints);
        if (!compile_hints_check_flag(item_hints, cgen_visit_ok)) {
            fprintf(stderr, "\tNote: See list item #%d around line %d\n", item_count, self->prev.line);
            return item_hints;
        }

        item_count++;

        if (compiler_match_curr(self, tk_comma)) {
            compiler_eat_tk(self, lexer, s);
        }
    }
    compiler_eat_tk(self, lexer, s);

    compiler_emit_op_unflagged(self, pg, op_mk_list, item_count);

    return hints;
}

uint8_t compiler_do_dict(Compiler *self, Lexer *lexer, const charspan *s, Program *pg, CompHints hints) {
    compiler_eat_tk(self, lexer, s); // ? SKIP '{'

    // ? Here, push a blank dictionary to add properties to...
    compiler_emit_op(self, pg, op_mk_dict);

    while (!compiler_match_curr(self, tk_eof)) {
        if (compiler_match_curr(self, tk_rbrace)) {
            break;
        }

        if (!compiler_match_curr(self, tk_string)) {
            compiler_warn(self, "Expected name string for dict field.", &self->curr, s);
            return cgen_parse_err;
        }
        compiler_eat_tk(self, lexer, s);

        charspan temp_key = {
            .data = s->data + self->prev.begin,
            .length = self->prev.length
        };

        const SymbolInfo *key_info = compiler_record_string(self, pg, &temp_key);

        if (key_info == NULL) {
            return cgen_dead;
        }
        compiler_emit_op_unflagged(self, pg, op_load_string_k, key_info->id);

        if (!compiler_match_curr(self, tk_colon)) {
            compiler_warn(self, "Expected ':' before field initializer.", &self->curr, s);
            return cgen_parse_err;
        }
        compiler_eat_tk(self, lexer, s);

        const CompHints field_initializer_hints = compiler_do_or(self, lexer, s, pg, hints);
        if (!compile_hints_check_flag(field_initializer_hints, cgen_visit_ok)) {
            return field_initializer_hints;
        }

        compiler_emit_op(self, pg, op_set_idx);

        if (!compiler_match_curr(self, tk_semicolon)) {
            compiler_warn(self, "Expected ';' after dict field.", &self->curr, s);
            return cgen_parse_err;
        }
        compiler_eat_tk(self, lexer, s);
    }
    compiler_eat_tk(self, lexer, s);

    return hints;
}

uint8_t compiler_do_literal(Compiler *self, Lexer *lexer, const charspan *s, Program *pg, CompHints hints) {
    const Token *curr_ref = &self->curr;
    const charspan lexeme = {
        .data = s->data + curr_ref->begin,
        .length = curr_ref->length
    };

    const SymbolInfo *temp_locus = NULL;
    CompHints temp_hints = cgen_visit_ok;

    switch (curr_ref->tag) {
        case tk_none:
            compiler_emit_op(self, pg, op_put_none);
            compiler_eat_tk(self, lexer, s);

            return hints;
        case tk_true: case tk_false:
            compiler_emit_op_flagged(self, pg, op_put_bool, curr_ref->tag == tk_true, 0);
            compiler_eat_tk(self, lexer, s);

            return hints;
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
        case tk_keyword_err:
            compiler_eat_tk(self, lexer, s); // ? skip 'ERR'
            compiler_emit_op(self, pg, op_load_err_ref);

            return hints;
        case tk_identifier:
            temp_locus = compiler_resolve_name(
                self,
                &lexeme
            );
            compiler_eat_tk(self, lexer, s);

            break;
        case tk_lparen:
            compiler_eat_tk(self, lexer, s);
            temp_hints = compiler_do_or(self, lexer, s, pg, hints);

            if (!compile_hints_check_flag(temp_hints, cgen_visit_ok)) {
                fprintf(stderr, "\tNote: See parenthesized expr at around line %d.\n", self->prev.line);
                return temp_hints;
            }

            if (!compiler_match_curr(self, tk_rparen)) {
                compiler_warn(self, "Expected ')' closing parenthesized expr.", &self->curr, s);
                return cgen_parse_err;
            }

            compiler_eat_tk(self, lexer, s);

            return hints;
        case tk_lbrack:
            return compiler_do_list(self, lexer, s, pg, hints);
        case tk_lbrace:
            return compiler_do_dict(self, lexer, s, pg, hints);
        default:
            compiler_warn(self, "Unexpected token in literal, expected none, true, false, or a name.", curr_ref, s);

            return cgen_parse_err;
    }

    if (temp_locus == NULL) {
        compiler_warn(self, "Undeclared name found here.", &self->prev, s);

        return cgen_dead;
    }

    if (self->curr.tag == tk_os_bind_equals) {
        hints |= cgen_assign_to;

        if (temp_locus->domain == symbol_local) {
            hints |= cgen_lhs_local;
            self->saved_id = temp_locus->id;

            return hints;
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
            hints |= cgen_lhs_native;
            // ? NOTE: The pushed ID is for a global procedure, VM or native.
            compiler_emit_op_unflagged(self, pg, op_load_imm_gid, temp_locus->id);

            break;
        case symbol_string:
            compiler_emit_op_unflagged(self, pg, op_load_string_k, temp_locus->id);

            break;
        default:
            break;
    }

    return hints;
}

uint8_t compiler_do_lhs(Compiler *self, Lexer *lexer, const charspan *s, Program *pg, CompHints hints) {
    const CompHints target_hints = compiler_do_literal(self, lexer, s, pg, hints);
    if (!compile_hints_check_flag(target_hints, cgen_visit_ok)) {
        fprintf(stderr, "\tNote: See LHS of access-of expression at line %d.\n", self->curr.line);
        return target_hints;
    }

    if (compiler_match_curr(self, tk_os_bind_equals) && compile_hints_check_flag(target_hints, cgen_assign_to)) {
        return target_hints;
    }

    if (compile_hints_check_flag(target_hints, cgen_lhs_local)) {
        compiler_emit_op_flagged(self, pg, op_load_local, 0, self->saved_id);
    }

    CompHints access_hints = target_hints;

    while (!compiler_match_curr(self, tk_eof)) {
        if (!compiler_match_curr(self, tk_lbrack)) {
            break;
        }
        compiler_eat_tk(self, lexer, s);

        access_hints |= cgen_access_of;

        const CompHints key_hints = compiler_do_or(self, lexer, s, pg, access_hints);
        if (!compile_hints_check_flag(key_hints, cgen_visit_ok)) {
            fprintf(stderr, "\tNote: See RHS of access-of expression at line %d.\n", self->curr.line);
            return key_hints;
        }

        if (compiler_match_curr(self, tk_rbrack)) {
            compiler_eat_tk(self, lexer, s);
        }

        if (compiler_match_curr(self, tk_os_bind_equals)) {
            access_hints |= cgen_assign_to;
        } else {
            compiler_emit_op(self, pg, op_get_idx);
        }
    }

    return access_hints;
}

uint8_t compiler_do_call(Compiler *self, Lexer *lexer, const charspan *s, Program *pg, CompHints hints) {
    const Token callee_name = self->curr;
    int arg_count = 0;

    const CompHints lhs_hints = compiler_do_lhs(self, lexer, s, pg, hints);
    if (!compile_hints_check_flag(lhs_hints, cgen_visit_ok)) {
        fprintf(stderr, "\tNote See call at line %d.\n", self->curr.line);
        return lhs_hints;
    }

    if (!compiler_match_curr(self, tk_lparen)) {
        return lhs_hints;
    }
    compiler_eat_tk(self, lexer, s);

    while (!compiler_match_curr(self, tk_eof)) {
        if (compiler_match_curr(self, tk_rparen)) {
            break;
        }

        const CompHints call_hints = compiler_do_compare(self, lexer, s, pg, hints);
        if (!compile_hints_check_flag(call_hints, cgen_visit_ok)) {
            fprintf(stderr, "\tNote See call at line %d.\n", callee_name.line);
            return call_hints;
        }

        arg_count++;

        if (compiler_match_curr(self, tk_comma)) {
            compiler_eat_tk(self, lexer, s);
        }
    }

    compiler_eat_tk(self, lexer, s);

    const uint8_t callee_is_native = compile_hints_check_flag(lhs_hints, cgen_lhs_native);

    compiler_emit_op_flagged(self, pg, op_call, callee_is_native, arg_count);

    return hints & ~cgen_assign_to;
}

uint8_t compiler_do_unary(Compiler *self, Lexer *lexer, const charspan *s, Program *pg, CompHints hints) {
    if (compiler_match_curr(self, tk_os_nullish)) {
        compiler_eat_tk(self, lexer, s);

        const CompHints maybe_call_hints = compiler_do_call(self, lexer, s, pg, hints);
        if (!compile_hints_check_flag(maybe_call_hints, cgen_visit_ok)) {
            return maybe_call_hints;
        }

        compiler_emit_op(self, pg, op_chk_none);

        return hints;
    }

    return compiler_do_call(self, lexer, s, pg, hints);
}

uint8_t compiler_do_null_coal(Compiler *self, Lexer *lexer, const charspan *s, Program *pg, CompHints hints) {
    const CompHints lhs_hints = compiler_do_unary(self, lexer, s, pg, hints);
    if (!compile_hints_check_flag(lhs_hints, cgen_visit_ok)) {
        return lhs_hints;
    }

    if (!compiler_match_curr(self, tk_os_nullcol)) {
        return lhs_hints;
    }
    compiler_eat_tk(self, lexer, s);

    compiler_emit_op_unflagged(self, pg, op_chk_none, 0);

    const uint16_t jump_nil_pos = pg->chunks.data[self->chunk_idx].code.length;
    // ? Pop NIL value and the leftover null-flag from the stack to save space.
    compiler_emit_op_unflagged(self, pg, op_jmp_false, 0);
    compiler_emit_op_flagged(self, pg, op_pop, 1, 0);

    const CompHints rhs_hints = compiler_do_unary(self, lexer, s, pg, hints);
    if (!compile_hints_check_flag(rhs_hints, cgen_visit_ok)) {
        return rhs_hints;
    }

    const uint16_t jump_past_pop_pos = pg->chunks.data[self->chunk_idx].code.length;
    compiler_emit_op_flagged(self, pg, op_jmp, 0, 0);

    const uint16_t jump_nil_end_pos = pg->chunks.data[self->chunk_idx].code.length;
    pg->chunks.data[self->chunk_idx].code.data[jump_nil_pos].wide = jump_nil_end_pos - jump_nil_pos;
    // ? Pop null-check false from the stack, exposing the LHS value for other code.
    compiler_emit_op_flagged(self, pg, op_pop, 1, 0);

    // ? Patch pop-skipping jump to avoid incorrectly popping a non-NIL RHS.
    pg->chunks.data[self->chunk_idx].code.data[jump_past_pop_pos].wide = (jump_nil_end_pos + 1) - jump_past_pop_pos;

    return hints;
}

uint8_t compiler_do_factor(Compiler *self, Lexer *lexer, const charspan *s, Program *pg, CompHints hints) {
    const CompHints lhs_hints = compiler_do_null_coal(self, lexer, s, pg, hints);
    if (!compile_hints_check_flag(lhs_hints, cgen_visit_ok)) {
        return lhs_hints;
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

        const CompHints rhs_hints = compiler_do_null_coal(self, lexer, s, pg, hints);
        if (!compile_hints_check_flag(rhs_hints, cgen_visit_ok)) {
            return rhs_hints;
        }
        compiler_emit_op(self, pg, op);
    }

    return lhs_hints;
}

uint8_t compiler_do_sum(Compiler *self, Lexer *lexer, const charspan *s, Program *pg, CompHints hints) {
    const CompHints lhs_hints = compiler_do_factor(self, lexer, s, pg, hints);
    if (!compile_hints_check_flag(lhs_hints, cgen_visit_ok)) {
        return lhs_hints;
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

        const CompHints rhs_hints = compiler_do_factor(self, lexer, s, pg, hints);
        if (!compile_hints_check_flag(rhs_hints, cgen_visit_ok)) {
            return rhs_hints;
        }
        compiler_emit_op(self, pg, op);
    }

    return lhs_hints;
}

uint8_t compiler_do_equality(Compiler *self, Lexer *lexer, const charspan *s, Program *pg, CompHints hints) {
    const CompHints lhs_hints = compiler_do_sum(self, lexer, s, pg, hints);
    if (!compile_hints_check_flag(lhs_hints, cgen_visit_ok)) {
        return lhs_hints;
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

        const CompHints rhs_hints = compiler_do_sum(self, lexer, s, pg, hints);
        if (!compile_hints_check_flag(rhs_hints, cgen_visit_ok)) {
            return rhs_hints;
        }
        compiler_emit_op(self, pg, op);
    }

    return lhs_hints;
}

uint8_t compiler_do_compare(Compiler *self, Lexer *lexer, const charspan *s, Program *pg, CompHints hints) {
    const CompHints lhs_hints = compiler_do_equality(self, lexer, s, pg, hints);
    if (!compile_hints_check_flag(lhs_hints, cgen_visit_ok)) {
        return lhs_hints;
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

        const CompHints rhs_hints = compiler_do_equality(self, lexer, s, pg, hints);
        if (!compile_hints_check_flag(rhs_hints, cgen_visit_ok)) {
            return rhs_hints;
        }
        compiler_emit_op(self, pg, op);
    }

    return lhs_hints;
}

uint8_t compiler_do_and(Compiler *self, Lexer *lexer, const charspan *s, Program *pg, CompHints hints) {
    const CompHints lhs_hints = compiler_do_compare(self, lexer, s, pg, hints);
    if (!compile_hints_check_flag(lhs_hints, cgen_visit_ok)) {
        return lhs_hints;
    }

    if (!compiler_match_curr(self, tk_os_and)) {
        return lhs_hints;
    }
    compiler_eat_tk(self, lexer, s);

    const uint16_t falsy_jmp_pos = pg->chunks.data[self->chunk_idx].code.length;
    compiler_emit_op_flagged(self, pg, op_jmp_false, 0, 0);
    compiler_emit_op_flagged(self, pg, op_pop, 1, 0); // ? Pop LHS if true, keeping our VM's "single result value" invariant.

    const CompHints rhs_hints = compiler_do_compare(self, lexer, s, pg, hints);
    if (!compile_hints_check_flag(rhs_hints, cgen_visit_ok)) {
        return rhs_hints;
    }

    const uint16_t falsy_jmp_end = pg->chunks.data[self->chunk_idx].code.length;
    compiler_emit_op(self, pg, op_nop);

    pg->chunks.data[self->chunk_idx].code.data[falsy_jmp_pos].wide = falsy_jmp_end - falsy_jmp_pos;

    return lhs_hints;
}

uint8_t compiler_do_or(Compiler *self, Lexer *lexer, const charspan *s, Program *pg, CompHints hints) {
    const CompHints lhs_hints = compiler_do_and(self, lexer, s, pg, hints);
    if (!compile_hints_check_flag(lhs_hints, cgen_visit_ok)) {
        return lhs_hints;
    }

    if (!compiler_match_curr(self, tk_os_or)) {
        return lhs_hints;
    }
    compiler_eat_tk(self, lexer, s);

    const uint16_t truthy_jmp_pos = pg->chunks.data[self->chunk_idx].code.length;
    compiler_emit_op_flagged(self, pg, op_jmp_if, 0, 0);
    compiler_emit_op_flagged(self, pg, op_pop, 1, 0); // ? Pop LHS if true, keeping our VM's "single result value" invariant.

    const CompHints rhs_hints = compiler_do_and(self, lexer, s, pg, hints);
    if (!compile_hints_check_flag(rhs_hints, cgen_visit_ok)) {
        return rhs_hints;
    }

    const uint16_t truthy_jmp_end = pg->chunks.data[self->chunk_idx].code.length;
    compiler_emit_op(self, pg, op_nop);

    pg->chunks.data[self->chunk_idx].code.data[truthy_jmp_pos].wide = truthy_jmp_end - truthy_jmp_pos;

    return lhs_hints;
}

uint8_t compiler_do_vars(Compiler *self, Lexer *lexer, const charspan *s, Program *pg, CompHints hints) {
    compiler_eat_tk(self, lexer, s); // ? skip LET

    while (!compiler_match_curr(self, tk_eof)) {
        if (compiler_match_curr(self, tk_semicolon)) {
            break;
        } else if (!compiler_match_curr(self, tk_identifier)) {
            compiler_warn(self, "Expected name in variable declaration here.", &self->curr, s);
            return cgen_parse_err;
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
            return cgen_parse_err;
        }
        compiler_eat_tk(self, lexer, s);

        const CompHints var_initializer_hints = compiler_do_or(self, lexer, s, pg, hints);
        if (!compile_hints_check_flag(var_initializer_hints, cgen_visit_ok)) {
            return var_initializer_hints;
        }
        compiler_emit_op_unflagged(self, pg, op_store_local, var_locus->id);

        if (compiler_match_curr(self, tk_comma)) {
            compiler_eat_tk(self, lexer, s);
        }
    }

    compiler_eat_tk(self, lexer, s);

    return hints;
}

uint8_t compiler_do_ifs(Compiler *self, Lexer *lexer, const charspan *s, Program *pg, CompHints hints) {
    compiler_eat_tk(self, lexer, s); // ? skip IF

    const int cmp_line = self->curr.line;
    const CompHints initial_check_hints = compiler_do_or(self, lexer, s, pg, hints);
    if (!compile_hints_check_flag(initial_check_hints, cgen_visit_ok)) {
        return initial_check_hints;
    }

    const uint16_t jump_else_pos = pg->chunks.data[self->chunk_idx].code.length;
    compiler_emit_op_flagged(self, pg, op_jmp_false, 0, 0);
    // compiler_emit_op_flagged(self, pg, op_pop, 1, 0);

    const CompHints truthy_path_body_hints = compiler_do_block(self, lexer, s, pg, hints);
    if (!compile_hints_check_flag(truthy_path_body_hints, cgen_visit_ok)) {
        return truthy_path_body_hints;
    }

    if (!compiler_match_curr(self, tk_keyword_else)) {
        const uint16_t skip_tbody_pos = pg->chunks.data[self->chunk_idx].code.length;
        compiler_emit_op_unflagged(self, pg, op_nop, 0);

        pg->chunks.data[self->chunk_idx].code.data[jump_else_pos].wide = skip_tbody_pos - jump_else_pos;

        return hints;
    }

    // * BEGIN ELSE clause ... * //
    compiler_eat_tk(self, lexer, s); // ? consume leading 'ELSE' of ELSE body

    const uint16_t jump_skip_else_pos = pg->chunks.data[self->chunk_idx].code.length;
    compiler_emit_op_unflagged(self, pg, op_jmp, 0);

    const uint16_t begin_else_pos = jump_skip_else_pos + 1;
    const CompHints falsy_path_stmt_hints = compiler_do_nestable_stmt(self, lexer, s, pg, hints);
    if (!compile_hints_check_flag(falsy_path_stmt_hints, cgen_visit_ok)) {
        fprintf(stderr, "\tNote See else-clause in the falsy-body around line %d.\n", self->curr.line);
        return falsy_path_stmt_hints;
    }

    const uint16_t end_ifs_pos = pg->chunks.data[self->chunk_idx].code.length;
    compiler_emit_op(self, pg, op_nop);

    pg->chunks.data[self->chunk_idx].code.data[jump_else_pos].wide = begin_else_pos - jump_else_pos;
    pg->chunks.data[self->chunk_idx].code.data[jump_skip_else_pos].wide = end_ifs_pos - jump_skip_else_pos;

    return hints;
}

uint8_t compiler_do_while(Compiler *self, Lexer *lexer, const charspan *s, Program *pg, CompHints hints) {
    compiler_eat_tk(self, lexer, s); // ? skip WHILE

    ActiveLoop *loop_data = compiler_enter_loop(self);

    const uint16_t while_check_pos = pg->chunks.data[self->chunk_idx].code.length;
    const CompHints loop_cond_hints = compiler_do_or(self, lexer, s, pg, hints);
    if (!compile_hints_check_flag(loop_cond_hints, cgen_visit_ok)) {
        fprintf(stderr, "\tNote See while-loop condition around line %d.\n", self->curr.line);
        compiler_leave_loop(self);
        return loop_cond_hints;
    }

    const uint16_t while_jmp_out_pos = pg->chunks.data[self->chunk_idx].code.length;
    compiler_emit_op_flagged(self, pg, op_jmp_false, 0, 0); // ? flags = 0 ==> forward jump applies!

    const CompHints loop_body_hints = compiler_do_block(self, lexer, s, pg, hints);
    if (!compile_hints_check_flag(loop_body_hints, cgen_visit_ok)) {
        fprintf(stderr, "\tNote See while-body around line %d.\n", self->curr.line);
        compiler_leave_loop(self);
        return loop_body_hints;
    }

    // ? 1. Patch 2 main loop jumps: the repeat & exit...
    const uint16_t while_jmp_back_pos = pg->chunks.data[self->chunk_idx].code.length;
    compiler_emit_op_flagged(self, pg, op_jmp, 1, 0); // ? flags = 1 ==> backwards jump applies!
    const uint16_t while_exit_pos = pg->chunks.data[self->chunk_idx].code.length;
    // compiler_emit_op_flagged(self, pg, op_pop, 1, 0); // ? pop off check after loop quits WHEN it's FALSE

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
    return hints;
}

uint8_t compiler_do_for(Compiler *self, Lexer *lexer, const charspan *s, Program *pg, CompHints hints) {
    compiler_eat_tk(self, lexer, s); // ? skip FOR

    ActiveLoop *loop_data = compiler_enter_loop(self);

    if (!compiler_match_curr(self, tk_identifier)) {
        compiler_warn(self, "Expected name for counter variable in C-style for loop here.", &self->curr, s);
        fprintf(stderr, "\tNote See line %d.\n", self->curr.line);

        compiler_leave_loop(self);
        return cgen_parse_err;
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
        return cgen_parse_err;
    }
    compiler_eat_tk(self, lexer, s);

    if (!compiler_do_or(self, lexer, s, pg, hints)) {
        fprintf(stderr, "\tNote: invalid initializer of FOR loop counter around line %d.\n", self->prev.line);

        compiler_leave_loop(self);
        return cgen_dead;
    }

    if (counter_info->domain == symbol_local) {
        compiler_emit_op_unflagged(self, pg, op_store_local, counter_info->id);
    } else {
        compiler_warn(self, "Invalid name of loop counter around here- shadows a similar non-local name.", &self->prev, s);
        fprintf(stderr, "\tNote: see line %d.\n", counter_name_line);
        return cgen_dead;
    }

    if (!compiler_match_curr(self, tk_comma)) {
        compiler_warn(self, "Expected ',' after counter in FOR loop here.", &self->curr, s);
        fprintf(stderr, "\tNote: see line %d.\n", self->curr.line);
        return cgen_parse_err;
    }
    compiler_eat_tk(self, lexer, s);

    const uint16_t loop_begin_pos = pg->chunks.data[self->chunk_idx].code.length;
    const CompHints loop_cond_hints = compiler_do_or(self, lexer, s, pg, hints);
    if (!compile_hints_check_flag(loop_cond_hints, cgen_visit_ok)) {
        fprintf(stderr, "\tNote: invalid condition of FOR loop around line %d.\n", self->prev.line);

        compiler_leave_loop(self);
        return loop_cond_hints;
    }
    const uint16_t loop_jump_out_pos = pg->chunks.data[self->chunk_idx].code.length;
    compiler_emit_op_unflagged(self, pg, op_jmp_false, 0);

    if (!compiler_match_curr(self, tk_comma)) {
        compiler_warn(self, "Expected ',' after check in FOR loop here.", &self->curr, s);
        fprintf(stderr, "\tNote: see line %d.\n", self->curr.line);
        return cgen_parse_err;
    }
    compiler_eat_tk(self, lexer, s);

    const uint16_t loop_skip_update_pos = pg->chunks.data[self->chunk_idx].code.length;
    compiler_emit_op_unflagged(self, pg, op_jmp, 0);

    const uint16_t loop_start_update_pos = pg->chunks.data[self->chunk_idx].code.length;
    compiler_emit_op_unflagged(self, pg, op_nop, 0);

    const CompHints update_clause_hints = compiler_do_expr_stmt(self, lexer, s, pg, hints);
    if (!compile_hints_check_flag(update_clause_hints, cgen_visit_ok)) {
        fprintf(stderr, "\tNote: invalid update clause of FOR loop around line %d.\n", self->prev.line);

        compiler_leave_loop(self);
        return update_clause_hints;
    }
    // ? Emit & easily patch "loop-repeater" jump AFTER update evaluations...
    const uint16_t loop_repeater_pos = pg->chunks.data[self->chunk_idx].code.length;
    compiler_emit_op_flagged(self, pg, op_jmp, 1, loop_repeater_pos - loop_begin_pos);
    // ? Patch the jump at loop_skip_update_pos since the update clause was fully resolved here...
    pg->chunks.data[self->chunk_idx].code.data[loop_skip_update_pos].wide = loop_repeater_pos + 1 - loop_skip_update_pos;

    const CompHints for_body_hints = compiler_do_block(self, lexer, s, pg, hints);
    if (!compile_hints_check_flag(for_body_hints, cgen_visit_ok)) {
        fprintf(stderr, "\tNote: invalid body of FOR loop around line %d.\n", self->prev.line);

        compiler_leave_loop(self);
        return for_body_hints;
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
    return hints;
}

uint8_t compiler_do_break(Compiler *self, Lexer *lexer, const charspan *s, Program *pg, CompHints hints) {
    compiler_eat_tk(self, lexer, s); // ? skip BREAK

    if (AnyVec_ActiveLoop_empty(&self->loops)) {
        compiler_warn(self, "Expected an enclosing loop for a 'BREAK' statement here.", &self->prev, s);
        fprintf(stderr, "\tNote: see line %d.\n", self->prev.line);
        return cgen_dead;
    }

    if (!compiler_match_curr(self, tk_semicolon)) {
        compiler_warn(self, "Expected a ';' closing a 'BREAK' statement here.", &self->prev, s);
        fprintf(stderr, "\tNote: see line %d.\n", self->prev.line);
        return cgen_parse_err;
    }
    compiler_eat_tk(self, lexer, s);

    const int break_bc_pos = pg->chunks.data[self->chunk_idx].code.length;
    compiler_track_break_pos(self, break_bc_pos);
    compiler_emit_op_unflagged(self, pg, op_jmp, 0);

    return hints;
}

uint8_t compiler_do_continue(Compiler *self, Lexer *lexer, const charspan *s, Program *pg, CompHints hints) {
    compiler_eat_tk(self, lexer, s); // ? skip BREAK

    if (AnyVec_ActiveLoop_empty(&self->loops)) {
        compiler_warn(self, "Expected an enclosing loop for a 'CONTINUE' statement here.", &self->prev, s);
        fprintf(stderr, "\tNote: see line %d.\n", self->prev.line);
        return cgen_dead;
    }

    if (!compiler_match_curr(self, tk_semicolon)) {
        compiler_warn(self, "Expected a ';' closing a 'BREAK' statement here.", &self->prev, s);
        fprintf(stderr, "\tNote: see line %d.\n", self->prev.line);
        return cgen_parse_err;
    }
    compiler_eat_tk(self, lexer, s);

    const int continue_bc_pos = pg->chunks.data[self->chunk_idx].code.length;
    compiler_track_continue_pos(self, continue_bc_pos);
    compiler_emit_op_unflagged(self, pg, op_jmp, 0);

    return hints;
}

uint8_t compiler_do_ret(Compiler *self, Lexer *lexer, const charspan *s, Program *pg, CompHints hints) {
    compiler_eat_tk(self, lexer, s); // ? skip RET

    const CompHints result_hints = compiler_do_or(self, lexer, s, pg, hints & ~cgen_assign_to);
    if (!compile_hints_check_flag(result_hints, cgen_visit_ok)) {
        fprintf(stderr, "\tNote 1: See return-result expression at line %d.\n", self->curr.line);
        return result_hints;
    }
    compiler_emit_op(self, pg, op_ret);

    if (!compiler_match_curr(self, tk_semicolon)) {
        compiler_warn(self, "Expected ';' after return statement.", &self->curr, s);
        fprintf(stderr, "\tNote: see line %d.\n", self->curr.line);
        return cgen_parse_err;
    }
    compiler_eat_tk(self, lexer, s);

    return hints;
}

uint8_t compiler_do_throw(Compiler *self, Lexer *lexer, const charspan *s, Program *pg, CompHints hints) {
    const int throw_stmt_line = self->curr.line;
    compiler_eat_tk(self, lexer, s); // ? skip 'THROW'

    const CompHints thrown_hints = compiler_do_or(self, lexer, s, pg, hints);
    if (!compile_hints_check_flag(thrown_hints, cgen_visit_ok)) {
        return thrown_hints;
    }

    compiler_emit_op_unflagged(self, pg, op_raise_err, throw_stmt_line);

    if (!compiler_match_curr(self, tk_semicolon)) {
        compiler_warn(self, "Expected ';' after THROW-stmt.", &self->curr, s);
        fprintf(stderr, "\tNote: see line %d.\n", self->curr.line);
        return cgen_parse_err;
    }
    compiler_eat_tk(self, lexer, s);

    return hints;
}

uint8_t compiler_do_try_catch(Compiler *self, Lexer *lexer, const charspan *s, Program *pg, CompHints hints) {
    compiler_eat_tk(self, lexer, s); // ? skip TRY

    const int try_clause_line = self->prev.line;
    const CompHints try_block_hints = compiler_do_block(self, lexer, s, pg, hints);
    if (!compile_hints_check_flag(try_block_hints, cgen_visit_ok)) {
        return try_block_hints;
    }

    const int jump_skip_catch_ip = pg->chunks.data[self->chunk_idx].code.length;
    compiler_emit_op_unflagged(self, pg, op_jmp, 0);
    compiler_emit_op(self, pg, op_catch_err);

    if (!compiler_match_curr(self, tk_keyword_catch)) {
        compiler_warn(self, "Expected 'CATCH' after throwable 'TRY' clause.", &self->curr, s);
        fprintf(stderr, "\tNote: see line %d.\n", self->curr.line);
        return cgen_parse_err;
    }
    compiler_eat_tk(self, lexer, s);

    const CompHints catch_body_hints = compiler_do_block(self, lexer, s, pg, hints);
    if (!compile_hints_check_flag(catch_body_hints, cgen_visit_ok)) {
        return catch_body_hints;
    }

    const int end_catch_ip = pg->chunks.data[self->chunk_idx].code.length;
    pg->chunks.data[self->chunk_idx].code.data[jump_skip_catch_ip].wide = end_catch_ip - jump_skip_catch_ip;

    return hints;
}

uint8_t compiler_do_expr_stmt(Compiler *self, Lexer *lexer, const charspan *s, Program *pg, CompHints hints) {
    // ? Process / emit LHS first...
    const CompHints dest_hints = compiler_do_or(self, lexer, s, pg, hints | cgen_assign_to);
    if (!compile_hints_check_flag(dest_hints, cgen_visit_ok)) {
        fprintf(stderr, "\tNote: See expr-stmt at line %d.\n", self->curr.line);
        return dest_hints;
    }

    if (compiler_match_curr(self, tk_os_bind_equals)) {
        compiler_eat_tk(self, lexer, s);

        const CompHints assign_src_hints = compiler_do_or(self, lexer, s, pg, hints & ~cgen_assign_to);
        if (!compile_hints_check_flag(assign_src_hints, cgen_visit_ok)) {
            fprintf(stderr, "\tNote: see RHS of assignment at line %d.\n", self->curr.line);
            return assign_src_hints;
        }

        // ? If we have consumed only a name = <value>, emit a simple update of that local slot.
        if (compile_hints_check_flag(dest_hints, cgen_lhs_local)) {
            compiler_emit_op_flagged(self, pg, op_store_local, 0, self->saved_id);
        } else if (compile_hints_check_flag(dest_hints, cgen_access_of)) {
            compiler_emit_op(self, pg, op_set_idx);
        } else {
            compiler_warn(self, "Invalid LHS of assignment, expected a name or key-access expression.\n", &self->prev, s);
            fprintf(stderr, "\tNote: see line %d.\n", self->prev.line);
            return cgen_dead;
        }
    }

    if (!compiler_match_curr(self, tk_semicolon)) {
        compiler_warn(self, "Expected ';' after expr-stmt.", &self->curr, s);
        fprintf(stderr, "\tNote: see line %d.\n", self->curr.line);
        return cgen_parse_err;
    }
    compiler_eat_tk(self, lexer, s);

    return hints;
}

uint8_t compiler_do_nestable_stmt(Compiler *self, Lexer *lexer, const charspan *s, Program *pg, CompHints hints) {
    switch (self->curr.tag) {
    case tk_keyword_let:
        return compiler_do_vars(self, lexer, s, pg, hints);
    case tk_keyword_if:
        return compiler_do_ifs(self, lexer, s, pg, hints);
    case tk_keyword_while:
        return compiler_do_while(self, lexer, s, pg, hints);
    case tk_keyword_for:
        return compiler_do_for(self, lexer, s, pg, hints);
    case tk_keyword_break:
        return compiler_do_break(self, lexer, s, pg, hints);
    case tk_keyword_continue:
        return compiler_do_continue(self, lexer, s, pg, hints);
    case tk_keyword_ret:
        return compiler_do_ret(self, lexer, s, pg, hints);
    case tk_keyword_throw:
        return compiler_do_throw(self, lexer, s, pg, hints);
    case tk_keyword_try:
        return compiler_do_try_catch(self, lexer, s, pg, hints);
    case tk_colon:
        return compiler_do_block(self, lexer, s, pg, hints);
    default:
        return compiler_do_expr_stmt(self, lexer, s, pg, hints);
    }
}

uint8_t compiler_do_block(Compiler *self, Lexer *lexer, const charspan *s, Program *pg, CompHints hints) {
    if (!compiler_match_curr(self, tk_colon)) {
        compiler_warn(self, "Expected ':' starting block.", &self->curr, s);
        fprintf(stderr, "\tNote: see line %d.\n", self->curr.line);
        return cgen_parse_err;
    }
    compiler_eat_tk(self, lexer, s);

    int stmt_count = 0;

    while (!compiler_match_curr(self, tk_eof)) {
        if (compiler_match_curr(self, tk_keyword_end)) {
            if (stmt_count == 0) {
                compiler_emit_op(self, pg, op_nop); // ? prevent empty bodies of conditional code which mess with jumps
            }

            break;
        }

        const int stmt_line = self->curr.line;
        if (!compiler_do_nestable_stmt(self, lexer, s, pg, hints)) {
            fprintf(stderr, "\tNote: See nested statement in block body around line %d.\n", stmt_line);
            return cgen_dead;
        }

        stmt_count++;
    }
    compiler_eat_tk(self, lexer, s);

    return hints;
}

uint8_t compiler_do_func(Compiler *self, Lexer *lexer, const charspan *s, Program *pg, CompHints hints) {
    compiler_eat_tk(self, lexer, s); // ? skip FUN

    if (!compiler_match_curr(self, tk_identifier)) {
        compiler_warn(self, "Expected name for FUN declaration.", &self->curr, s);
        fprintf(stderr, "\tNote: see line %d.\n", self->curr.line);
        return cgen_parse_err;
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
        return cgen_dead;
    }
    compiler_eat_tk(self, lexer, s);

    while (!compiler_match_curr(self, tk_eof)) {
        if (compiler_match_curr(self, tk_rparen)) {
            break;
        } else if (!compiler_match_curr(self, tk_identifier)) {
            compiler_warn(self, "Expected name in params list here.", &self->curr, s);
            fprintf(stderr, "\tNote: see line %d.\n", self->curr.line);
            return cgen_parse_err;
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

    if (!compiler_do_block(self, lexer, s, pg, hints)) {
        fprintf(stderr, "\tNote: See in FUN declaration around line %d.\n", self->curr.line);
        return cgen_dead;
    } else {
        compiler_patch_reserve_inst(self, &self->locals, pg);
        symbol_table_clear(&self->locals);
        self->chunk_idx = 0;
    }

    return hints;
}

uint8_t compiler_do_stmt(Compiler *self, Lexer *lexer, const charspan *s, Program *pg, CompHints hints) {
    switch (self->curr.tag) {
    case tk_keyword_let:
        return compiler_do_vars(self, lexer, s, pg, hints);
    case tk_keyword_if:
        return compiler_do_ifs(self, lexer, s, pg, hints);
    case tk_keyword_while:
        return compiler_do_while(self, lexer, s, pg, hints);
    case tk_keyword_for:
        return compiler_do_for(self, lexer, s, pg, hints);
    case tk_keyword_ret:
        return compiler_do_ret(self, lexer, s, pg, hints);
    case tk_keyword_throw:
        return compiler_do_throw(self, lexer, s, pg, hints);
    case tk_keyword_try:
        return compiler_do_try_catch(self, lexer, s, pg, hints);
    case tk_keyword_fun:
        return compiler_do_func(self, lexer, s, pg, hints);
    default:
        return compiler_do_expr_stmt(self, lexer, s, pg, hints);
    }
}

int8_t compiler_do_source(Compiler *self, Lexer *lexer, const charspan *s, Program *pg) {
    compiler_eat_tk(self, lexer, s); // ? remove the unknown token placeholder by getting the 1st token into self->curr... this is needed for correct parsing during bytecode emission.

    CompHints initial_hints = cgen_visit_ok;

    // ! IMPORTANT: push an empty bytecode chunk so that an OOB terminate doesn't happen via accessing an empty code buf for top-level code.
    Chunk temp;
    Chunk_new(&temp); // ? initialize empty chunk
    AnyVec_Chunk_push(&pg->chunks, &temp); // ? copy the empty chunk into this Vec, but don't touch temp again... just did scuffed destructive moves??

    self->locals.var_alloc_ip = pg->chunks.data[self->chunk_idx].code.length;
    compiler_emit_op_unflagged(self, pg, op_reserve, 0);

    while (!compiler_match_curr(self, tk_eof) && self->errors <= TBASIC_MAX_COMPILE_ERRORS) {
        CompHints checked_hints = compiler_do_stmt(self, lexer, s, pg, initial_hints);
        if (compile_hints_check_flag(checked_hints, cgen_dead)) {
            fprintf(stderr, "\n\tFatal error, aborting.\n");
            break;
        } else if (compile_hints_check_flag(checked_hints, cgen_parse_err)) {
            // ? Recover parsing by skipping to a FUN declaration as a synchronization point.
            while (!compiler_match_curr(self, tk_eof)) {
                if (compiler_match_curr(self, tk_keyword_fun)) {
                    compiler_eat_tk(self, lexer, s);
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
