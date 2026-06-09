#ifndef TBASIC_OPTIMIZER_H
#define TBASIC_OPTIMIZER_H

#include "bytecode.h"



#define MAX_OPTIMIZER_STATE_EDGES 4

typedef void(*ReplacerFn)(Instruction *current);

typedef struct tb_opt_edge_t {
    Opcode op;
    uint8_t state_id;
} OptEdge;

typedef struct tb_opt_state_t {
    ReplacerFn f;
    OptEdge edges[MAX_OPTIMIZER_STATE_EDGES];
} OptState;

/**
 ## Optimizer Notes
 1. Is a state machine where transitions to substitutions go by next opcodes. A single instruction cursor advances forward.
 2. Substitutions are handled by function pointers (`ReplacerFn`)
 ### STATES
  - sStart: PUT_CONST -> sK, GET_LOCAL -> sL, * -> sStart
  - sK: GET_LOCAL -> sKL, GET_CONST -> sKK, * -> sStart
  - sL: PUT_CONST -> sKL, GET_LOCAL -> sLL, * -> sStart
  - sKL: MUL -> sKLM, DIV -> sKLD, ADD -> sKLA, SUB -> sKLS, * -> sStart
  - sKK: MUL -> sKKM, DIV -> sKKD, ADD -> sKKA, SUB -> sKKS, * -> sStart
  - sLL: MUL -> sLLM, DIV -> sLLD, ADD -> sLLA, SUB -> sLLS, * -> sStart
  - sKL(M | D | A | S) -> ReplacerFn -> sStart
  - sKK(M | D | A | S) -> ReplacerFn -> sStart
  - sLL(M | D | A | S) -> ReplacerFn -> sStart
 ### Possible run:
 - Before: NOP 0 0, PUT_CONST 0 1, GET_LOCAL 0 1, ADD, SET_LOCAL 0 1
 - States: sStart -> sStart -> sK -> sKL -> sKLA -> sStart
 - After: NOP, ADD_KL 1 1, NOP, NOP, SET_LOCAL 0 1
 */
typedef struct tb_optimizer_t {
    const OptState *states;
    const OptState *sp;
    Instruction *ip;
} Optimizer;

void optimizer_dud(Optimizer *self);
void optimizer_del(Optimizer *self);

const OptState *optimizer_transit(Optimizer *self, Opcode op);

void optimizer_apply(Optimizer *self, AnyVec_Instruction *code);

#endif