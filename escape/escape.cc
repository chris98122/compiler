#include "tiger/escape/escape.h"

namespace ESC
{

class EscapeEntry
{
public:
  int depth;
  bool *escape;

  EscapeEntry(int depth, bool *escape) : depth(depth), escape(escape) {}
};

void FindEscape(A::Exp *exp)
{
  // TODO: Put your codes here (lab6).
  S::Table<ESC::EscapeEntry> *escapeEnv = new S::Table<EscapeEntry>();
  traverseExp(escapeEnv, exp, 1);
}
static void traverseVar(S::Table<ESC::EscapeEntry> *escapeEnv, A::Var *var, int depth)
{
  switch (var->kind)
  {
  case A::Var::SIMPLE:
  {
    ESC::EscapeEntry *entry = escapeEnv->Look(((A::SimpleVar *)var)->sym);
    if (depth > entry->depth)
    {
      (*(entry->escape)) = true;
    }
    break;
  }
  case A::Var::FIELD:
  {
    traverseVar(escapeEnv, ((A::FieldVar *)var)->var, depth);
    break;
  }
  case A::Var::SUBSCRIPT:
  {
    traverseVar(escapeEnv, ((A::SubscriptVar *)var)->var, depth);
    traverseExp(escapeEnv, ((A::SubscriptVar *)var)->subscript, depth);
    break;
  }
  }
}
static void traverseDec(S::Table<ESC::EscapeEntry> *escapeEnv, A::Dec *dec, int level)
{
  switch (dec->kind)
  {
  case A::Dec::VAR:
  {
    ((A::VarDec *)dec)->escape = false;
    escapeEnv->Enter(((A::VarDec *)dec)->var, new ESC::EscapeEntry(level, &(((A::VarDec *)dec)->escape)));
    traverseExp(escapeEnv, ((A::VarDec *)dec)->init, level);
    break;
  }
  case A::Dec::FUNCTION:
  {
    A::FunDecList *decs = ((A::FunctionDec *)dec)->functions;
    for (; decs; decs = decs->tail)
    {
      A::FunDec *fundec = decs->head;
      escapeEnv->BeginScope();
      A::FieldList *formals = fundec->params;
      for (; formals; formals = formals->tail)
      {
        A::Field *formal = formals->head;
        formal->escape = false;
        escapeEnv->Enter(formal->name, new ESC::EscapeEntry(level+ 1, &(formal->escape)));
      }
      traverseExp(escapeEnv, fundec->body,level + 1);
      escapeEnv->EndScope();
    }
    break;
  }
  case A::Dec::TYPE:
  default:
    break;
  }
}

static void traverseExp(S::Table<ESC::EscapeEntry> *escapeEnv, A::Exp *exp, int level)
{
  switch (exp->kind)
  {
  case A::Exp::VAR:
  {
    traverseVar(escapeEnv, ((A::VarExp *)exp)->var, level);
    break;
  }
  case A::Exp::CALL:
  {
    A::ExpList *args = ((A::CallExp *)exp)->args;
    for (; args; args = args->tail)
      traverseExp(escapeEnv, args->head, level);
    break;
  }
  case A::Exp::OP:
  {
    traverseExp(escapeEnv, ((A::OpExp *)exp)->left, level);
    traverseExp(escapeEnv, ((A::OpExp *)exp)->right, level);
    break;
  }
  case A::Exp::RECORD:
  {
    A::EFieldList *fields = ((A::RecordExp *)exp)->fields;
    for (; fields; fields = fields->tail)
      traverseExp(escapeEnv, fields->head->exp, level);
    break;
  }
  case A::Exp::SEQ:
  {
    A::ExpList *seq = ((A::SeqExp *)exp)->seq;
    for (; seq; seq = seq->tail)
      traverseExp(escapeEnv, seq->head, level);
    break;
  }
  case A::Exp::ASSIGN:
  {
    traverseVar(escapeEnv, ((A::AssignExp *)exp)->var, level);
    traverseExp(escapeEnv, ((A::AssignExp *)exp)->exp, level);
    break;
  }
  case A::Exp::IF:
  {
    traverseExp(escapeEnv, ((A::IfExp *)exp)->test, level);
    traverseExp(escapeEnv, ((A::IfExp *)exp)->then, level);
    if (((A::IfExp *)exp)->elsee)
    {
      traverseExp(escapeEnv, ((A::IfExp *)exp)->elsee, level);
    }
    break;
  }
  case A::Exp::WHILE:
  {
    traverseExp(escapeEnv, ((A::WhileExp *)exp)->test, level);
    traverseExp(escapeEnv, ((A::WhileExp *)exp)->body, level);
    break;
  }
  case A::Exp::FOR:
  {
    ((A::ForExp *)exp)->escape = false;
    escapeEnv->Enter(((A::ForExp *)exp)->var, new EscapeEntry(level, &(((A::ForExp *)exp)->escape)));

    traverseExp(escapeEnv, ((A::ForExp *)exp)->lo, level);
    traverseExp(escapeEnv, ((A::ForExp *)exp)->hi, level);
    traverseExp(escapeEnv, ((A::ForExp *)exp)->body, level);
    break;
  }
  case A::Exp::LET:
  {
    A::DecList *decs = ((A::LetExp *)exp)->decs;
    for (; decs; decs = decs->tail)
      traverseDec(escapeEnv, decs->head, level);
    traverseExp(escapeEnv, ((A::LetExp *)exp)->body, level);
    break;
  }
  case A::Exp::ARRAY:
  {
    traverseExp(escapeEnv, ((A::ArrayExp *)exp)->size, level);
    traverseExp(escapeEnv, ((A::ArrayExp *)exp)->init, level);
    break;
  }

  case A::Exp::STRING:
  case A::Exp::BREAK:
  case A::Exp::NIL:
  case A::Exp::INT:
  case A::Exp::VOID:
    break;

  default:
    break;
  }
}

} // namespace ESC