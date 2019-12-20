#ifndef TIGER_ESCAPE_ESCAPE_H_
#define TIGER_ESCAPE_ESCAPE_H_

#include "tiger/absyn/absyn.h"

namespace ESC {
class EscapeEntry;
void FindEscape(A::Exp* exp);
static void traverseVar(S::Table<ESC::EscapeEntry> *escapeEnv, A::Var *var, int level);

static void traverseDec(S::Table<ESC::EscapeEntry> *escapeEnv, A::Dec *dec, int level);

static void traverseExp(S::Table<ESC::EscapeEntry> *, A::Exp *exp ,int level);
}  // namespace ESC


#endif