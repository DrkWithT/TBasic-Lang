#include "optimizer.h"



static void clear_inst(Instruction *ip) {
    ip->op = op_nop;
    ip->flag = 0;
    ip->wide = 0;
}

static void replace_binary_inst(Instruction *current, Opcode first_op, Opcode second_op, Opcode super_op) {
    Instruction *prev_0 = current - 1;
    Instruction *prev_1 = current - 2;

    if (prev_1->op == first_op && prev_0->op == second_op) {
        const uint16_t first_n = prev_1->wide;
        const uint16_t second_n = prev_0->wide;

        prev_1->op = super_op;
        prev_1->flag = first_n;
        prev_1->wide = second_n;
    } else {
        const uint16_t first_n = prev_0->wide;
        const uint16_t second_n = prev_1->wide;

        prev_1->op = super_op;
        prev_1->flag = first_n;
        prev_1->wide = second_n;
    }
    
    clear_inst(prev_0);
    clear_inst(current);
}

static void replace_kl_mul(Instruction *current) {
    replace_binary_inst(current, op_put_k, op_load_local, op_mul_kl);
}

static void replace_kl_div(Instruction *current) {
    replace_binary_inst(current, op_put_k, op_load_local, op_div_kl);
}

static void replace_kl_add(Instruction *current) {
    replace_binary_inst(current, op_put_k, op_load_local, op_add_kl);
}

static void replace_kl_sub(Instruction *current) {
    replace_binary_inst(current, op_put_k, op_load_local, op_sub_kl);
}

static void replace_kk_mul(Instruction *current) {
    replace_binary_inst(current, op_put_k, op_put_k, op_mul_kk);
}

static void replace_kk_div(Instruction *current) {
    replace_binary_inst(current, op_put_k, op_put_k, op_div_kk);
}

static void replace_kk_add(Instruction *current) {
    replace_binary_inst(current, op_put_k, op_put_k, op_add_kk);
}

static void replace_kk_sub(Instruction *current) {
    replace_binary_inst(current, op_put_k, op_put_k, op_sub_kk);
}

static void replace_ll_mul(Instruction *current) {
    replace_binary_inst(current, op_load_local, op_load_local, op_mul_ll);
}

static void replace_ll_div(Instruction *current) {
    replace_binary_inst(current, op_load_local, op_load_local, op_div_ll);
}

static void replace_ll_add(Instruction *current) {
    replace_binary_inst(current, op_load_local, op_load_local, op_add_ll);
}

static void replace_ll_sub(Instruction *current) {
    replace_binary_inst(current, op_load_local, op_load_local, op_sub_ll);
}


static const OptState tb_optimizer_states[] = {
    // Start
    {.f = NULL, .edges = {
        (OptEdge) {.op = op_put_k, .state_id = 1},
        (OptEdge) {.op = op_load_local, .state_id = 2},
        (OptEdge) {.op = op_nop, .state_id = 0},
        (OptEdge) {.op = op_nop, .state_id = 0},
    }},
    // K
    {.f = NULL, .edges = {
        (OptEdge) {.op = op_load_local, .state_id = 3},
        (OptEdge) {.op = op_put_k, .state_id = 4},
        (OptEdge) {.op = op_nop, .state_id = 0},
        (OptEdge) {.op = op_nop, .state_id = 0},
    }},
    // L
    {.f = NULL, .edges = {
        (OptEdge) {.op = op_load_local, .state_id = 5},
        (OptEdge) {.op = op_put_k, .state_id = 3},
        (OptEdge) {.op = op_nop, .state_id = 0},
        (OptEdge) {.op = op_nop, .state_id = 0},
    }},
    // KL
    {.f = NULL, .edges = {
        (OptEdge) {.op = op_mul, .state_id = 6},
        (OptEdge) {.op = op_div, .state_id = 7},
        (OptEdge) {.op = op_add, .state_id = 8},
        (OptEdge) {.op = op_sub, .state_id = 9},
    }},
    // KK
    {.f = NULL, .edges = {
        (OptEdge) {.op = op_mul, .state_id = 10},
        (OptEdge) {.op = op_div, .state_id = 11},
        (OptEdge) {.op = op_add, .state_id = 12},
        (OptEdge) {.op = op_sub, .state_id = 13},
    }},
    // LL
    {.f = NULL, .edges = {
        (OptEdge) {.op = op_mul, .state_id = 14},
        (OptEdge) {.op = op_div, .state_id = 15},
        (OptEdge) {.op = op_add, .state_id = 16},
        (OptEdge) {.op = op_sub, .state_id = 17},
    }},
    {.f = replace_kl_mul, .edges = {
        (OptEdge) {.op = op_nop, 0},
        (OptEdge) {.op = op_nop, 0},
        (OptEdge) {.op = op_nop, 0},
        (OptEdge) {.op = op_nop, 0},
    }},
    {.f = replace_kl_div, .edges = {
        (OptEdge) {.op = op_nop, 0},
        (OptEdge) {.op = op_nop, 0},
        (OptEdge) {.op = op_nop, 0},
        (OptEdge) {.op = op_nop, 0},
    }},
    {.f = replace_kl_add, .edges = {
        (OptEdge) {.op = op_nop, 0},
        (OptEdge) {.op = op_nop, 0},
        (OptEdge) {.op = op_nop, 0},
        (OptEdge) {.op = op_nop, 0},
    }},
    {.f = replace_kl_sub, .edges = {
        (OptEdge) {.op = op_nop, 0},
        (OptEdge) {.op = op_nop, 0},
        (OptEdge) {.op = op_nop, 0},
        (OptEdge) {.op = op_nop, 0},
    }},
    {.f = replace_kk_mul, .edges = {
        (OptEdge) {.op = op_nop, 0},
        (OptEdge) {.op = op_nop, 0},
        (OptEdge) {.op = op_nop, 0},
        (OptEdge) {.op = op_nop, 0},
    }},
    {.f = replace_kk_div, .edges = {
        (OptEdge) {.op = op_nop, 0},
        (OptEdge) {.op = op_nop, 0},
        (OptEdge) {.op = op_nop, 0},
        (OptEdge) {.op = op_nop, 0},
    }},
    {.f = replace_kk_add, .edges = {
        (OptEdge) {.op = op_nop, 0},
        (OptEdge) {.op = op_nop, 0},
        (OptEdge) {.op = op_nop, 0},
        (OptEdge) {.op = op_nop, 0},
    }},
    {.f = replace_kk_sub, .edges = {
        (OptEdge) {.op = op_nop, 0},
        (OptEdge) {.op = op_nop, 0},
        (OptEdge) {.op = op_nop, 0},
        (OptEdge) {.op = op_nop, 0},
    }},
    {.f = replace_ll_mul, .edges = {
        (OptEdge) {.op = op_nop, 0},
        (OptEdge) {.op = op_nop, 0},
        (OptEdge) {.op = op_nop, 0},
        (OptEdge) {.op = op_nop, 0},
    }},
    {.f = replace_ll_div, .edges = {
        (OptEdge) {.op = op_nop, 0},
        (OptEdge) {.op = op_nop, 0},
        (OptEdge) {.op = op_nop, 0},
        (OptEdge) {.op = op_nop, 0},
    }},
    {.f = replace_ll_add, .edges = {
        (OptEdge) {.op = op_nop, 0},
        (OptEdge) {.op = op_nop, 0},
        (OptEdge) {.op = op_nop, 0},
        (OptEdge) {.op = op_nop, 0},
    }},
    {.f = replace_ll_sub, .edges = {
        (OptEdge) {.op = op_nop, 0},
        (OptEdge) {.op = op_nop, 0},
        (OptEdge) {.op = op_nop, 0},
        (OptEdge) {.op = op_nop, 0},
    }},
};



void optimizer_dud(Optimizer *self) {
    self->states = tb_optimizer_states;
    self->sp = NULL;
    self->ip = NULL;
}

void optimizer_del(Optimizer *self) {
    (void) self;
}

const OptState *optimizer_transit(Optimizer *self, Opcode op) {
    const OptState *curr_state = self->sp;
    const OptEdge *curr_edges = curr_state->edges;

    for (int i = 0; i < MAX_OPTIMIZER_STATE_EDGES; i++) {
        if (curr_edges[i].op == op) {
            return self->states + curr_edges[i].state_id;
        }
    }

    // ? 0th optimizer state (AKA sStart) is a fallback upon invalid / unfound transitions
    return self->states;
}

void optimizer_apply(Optimizer *self, AnyVec_Instruction *code) {
    const Instruction *temp_end_ip = code->data + code->length;

    self->ip = code->data;         // ? begin scanning from 1st instruction
    self->sp = self->states;    // ? begin transitioning to any optimizer function from `sStart`

    while (self->ip != temp_end_ip) {
        self->sp = optimizer_transit(self, self->ip->op);

        const ReplacerFn state_fn = self->sp->f;

        if (state_fn != NULL) {
            state_fn(self->ip);
            self->sp = self->states; // ? return to start state
        }

        self->ip++;
    }
}
