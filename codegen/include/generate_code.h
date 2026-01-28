#pragma once
#include "../include/types.h"
#include "mips_code.h"

CodeList *generate_code(Node *node);

void optimize_delay_slot(CodeList *list);

void optimize_nop(CodeList *list);

void optimize_branch(CodeList *list);

void optimize_per_function(CodeList *list);
