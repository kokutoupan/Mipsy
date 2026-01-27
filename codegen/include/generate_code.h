#pragma once
#include "../include/types.h"
#include "mips_code.h"

CodeList *generate_code(Node *node);

void optimize_nop(CodeList *list);

void optimize_address(CodeList *list);

void optimize_load_move(CodeList *list);

void optimize_addiu_chain(CodeList *list);

void optimize_branch(CodeList *list);
