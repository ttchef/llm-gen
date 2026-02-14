
#ifndef GENERATE_H
#define GENERATE_H

#include <stdint.h>

struct Context;
int32_t generate_ai_answer(struct Context* ctx, char** text, char** response);

#endif // GENERATE_H
