#include "tiger/semant/semant.h"
#include "tiger/errormsg/errormsg.h"

#include "string.h"

extern EM::ErrorMsg errormsg;

using VEnvType = S::Table<E::EnvEntry> *;
using TEnvType = S::Table<TY::Ty> *;

#define FOR_ITER_CHECK 1

namespace
{
static TY::TyList *make_formal_tylist(TEnvType tenv, A::FieldList *params)
{
  if (params == nullptr)
  {
    return nullptr;
  }

  TY::Ty *ty = tenv->Look(params->head->typ);
  if (ty == nullptr)
  {
    errormsg.Error(params->head->pos, "undefined type %s",
                   params->head->typ->Name().c_str());
  }

  return new TY::TyList(ty->ActualTy(), make_formal_tylist(tenv, params->tail));
}

static TY::FieldList *make_fieldlist(TEnvType tenv, A::FieldList *fields)
{
  if (fields == nullptr)
  {
    return nullptr;
  }

  TY::Ty *ty = tenv->Look(fields->head->typ);
  if (ty == NULL)
  {
    errormsg.Error(fields->head->pos, "undefined type %s",
                   fields->head->typ->Name().c_str());
  }
  return new TY::FieldList(new TY::Field(fields->head->name, ty),
                           make_fieldlist(tenv, fields->tail));
}

} // namespace

namespace A
{

TY::Ty *SimpleVar::SemAnalyze(VEnvType venv, TEnvType tenv,
                              int labelcount) const
{
  // TODO: Put your codes here (lab4).
  if (this->sym)
  {
    E::EnvEntry *value = venv->Look(this->sym);
    if ((E::VarEntry *)value)
    {
      return ((E::VarEntry *)value)->ty;
    }
    else
    {
      errormsg.Error(0, "undefined variable %s", this->sym->Name().c_str());
    }
  }
  return TY::VoidTy::Instance();
}

TY::Ty *FieldVar::SemAnalyze(VEnvType venv, TEnvType tenv,
                             int labelcount) const
{
  // TODO: Put your codes here (lab4).

  if (this->var->kind == SIMPLE)
  {
    E::EnvEntry *var = venv->Look(((SimpleVar *)this->var)->sym);
    //check if var is a record
    if (((E::VarEntry *)var)->ty->kind != TY::Ty::RECORD)
    {
      errormsg.Error(0, "not a record type");
      return TY::VoidTy::Instance();
    }

    // get type symbol fields
    TY::FieldList *f = ((TY::RecordTy *)((E::VarEntry *)var)->ty)->fields;
    while (f && f->head)
    {
      if (!strcmp(f->head->name->Name().c_str(), this->sym->Name().c_str()))
        return f->head->ty;
      f = f->tail;
    }
    errormsg.Error(0, "field %s doesn't exist", this->sym->Name().c_str());
  }
  return TY::VoidTy::Instance();
}

TY::Ty *SubscriptVar::SemAnalyze(VEnvType venv, TEnvType tenv,
                                 int labelcount) const
{
  // TODO: Put your codes here (lab4).
  E::EnvEntry *var = venv->Look(((SimpleVar *)this->var)->sym);
  if (var)
  {
    TY::Ty *vartype = ((E::VarEntry *)var)->ty;
    if (vartype && vartype->kind != TY::Ty::ARRAY)
    {
      errormsg.Error(0, "array type required");
       return TY::VoidTy::Instance();
    }
    return vartype;
  }
   return TY::VoidTy::Instance();
}

TY::Ty *VarExp::SemAnalyze(VEnvType venv, TEnvType tenv, int labelcount) const
{
  // TODO: Put your codes here (lab4).

  return var->SemAnalyze(venv, tenv, labelcount);
}

TY::Ty *NilExp::SemAnalyze(VEnvType venv, TEnvType tenv, int labelcount) const
{
  // TODO: Put your codes here (lab4).
  return TY::NilTy::Instance();
}

TY::Ty *IntExp::SemAnalyze(VEnvType venv, TEnvType tenv, int labelcount) const
{
  // TODO: Put your codes here (lab4).
  return TY::IntTy::Instance();
}

TY::Ty *StringExp::SemAnalyze(VEnvType venv, TEnvType tenv,
                              int labelcount) const
{
  // TODO: Put your codes here (lab4).
  return TY::StringTy::Instance();
}

TY::Ty *CallExp::SemAnalyze(VEnvType venv, TEnvType tenv,
                            int labelcount) const
{
  // TODO: Put your codes here (lab4).

  E::EnvEntry *value = venv->Look(this->func);
  if ((E::FunEntry *)value)
  {
    //check all the params is defined variable

    TY::TyList *func_arg_type_list = ((E::FunEntry *)value)->formals;
    ExpList *arg_list = this->args;
    while (arg_list && arg_list->head && func_arg_type_list && func_arg_type_list->head)
    {
      TY::Ty *arg_type = arg_list->head->SemAnalyze(venv, tenv, labelcount);
      TY::Ty *func_arg_type = func_arg_type_list->head;
      //check params type
      if (arg_type != func_arg_type)
      {
        errormsg.Error(0, "para type mismatch");
      }
      arg_list = arg_list->tail;
      func_arg_type_list = func_arg_type_list->tail;
    }
    if (arg_list)
    {
      errormsg.Error(0, "too many params in function %s", this->func->Name().c_str());
    }
    return ((E::FunEntry *)value)->result;
  }
  else
  {
    errormsg.Error(0, "undefined function %s", this->func->Name().c_str());
  }

  return TY::VoidTy::Instance();
}

TY::Ty *OpExp::SemAnalyze(VEnvType venv, TEnvType tenv, int labelcount) const
{
  // TODO: Put your codes here (lab4).

  TY::Ty *leftexp = this->left->SemAnalyze(venv, tenv, labelcount);
  TY::Ty *rightexp = this->right->SemAnalyze(venv, tenv, labelcount);

  if (oper == PLUS_OP || oper == MINUS_OP || oper == TIMES_OP || oper == DIVIDE_OP)
  {
    if (leftexp == NULL || rightexp == NULL)
    {
      errormsg.Error(0, "integer required");
      return TY::IntTy::Instance();
    }
    if (leftexp->kind != TY::Ty::INT || rightexp->kind != TY::Ty::INT)
    {
      errormsg.Error(0, "integer required");
      return TY::IntTy::Instance();
    }
  }

  if (leftexp != rightexp)
  {
    errormsg.Error(0, "same type required");
    return TY::IntTy::Instance();
  }
  return TY::IntTy::Instance();
}

TY::Ty *RecordExp::SemAnalyze(VEnvType venv, TEnvType tenv,
                              int labelcount) const
{
  // TODO: Put your codes here (lab4).

  //make a new record instance

  TY::Ty *record = tenv->Look(this->typ);
  if (!record)
  {
    //unknown type
    errormsg.Error(0, "undefined type %s", this->typ->Name().c_str());
  }
  //record fileds initialization this->fields;
  //record->fields

  return record;
}

TY::Ty *SeqExp::SemAnalyze(VEnvType venv, TEnvType tenv, int labelcount) const
{
  // TODO: Put your codes here (lab4).

  ExpList *explist = this->seq;
  TY::Ty *result;
  while (explist && explist->head)
  {
    Exp *exp = explist->head;
    result = exp->SemAnalyze(venv, tenv, labelcount);
    explist = explist->tail;
  }
  return result;
}

TY::Ty *AssignExp::SemAnalyze(VEnvType venv, TEnvType tenv,
                              int labelcount) const
{
  // TODO: Put your codes here (lab4).

  // check the leftvalue is validated
  TY::Ty *left_type = this->var->SemAnalyze(venv, tenv, labelcount);

  //if readonly then print error
  if (this->var->kind == Var::SIMPLE)
  {
    E::EnvEntry *leftvalue_type = venv->Look(((SimpleVar *)var)->sym);
    if (leftvalue_type->readonly && labelcount == FOR_ITER_CHECK)
    {
      errormsg.Error(0, "loop variable can't be assigned");
    }
  }

  //check the leftvalu type equals exp type
  TY::Ty *right_type = this->exp->SemAnalyze(venv, tenv, labelcount);

  if (left_type->kind != right_type->kind)
  {
    errormsg.Error(0, "unmatched assign exp");
  }
  return right_type;
}

TY::Ty *IfExp::SemAnalyze(VEnvType venv, TEnvType tenv, int labelcount) const
{
  // TODO: Put your codes here (lab4).
  TY::Ty *t = this->test->SemAnalyze(venv, tenv, labelcount);
  TY::Ty *then_type = this->then->SemAnalyze(venv, tenv, labelcount);
  if (this->elsee == NULL && then_type != TY::VoidTy::Instance())
  {
    errormsg.Error(0, "if-then exp's body must produce no value");
    return TY::VoidTy::Instance();
  }
  if (this->elsee)
  {
    TY::Ty *else_type = this->elsee->SemAnalyze(venv, tenv, labelcount);
    if (else_type->kind != then_type->kind)
    {
      errormsg.Error(0, "then exp and else exp type mismatch");
    }
  }

  return then_type;
}

TY::Ty *WhileExp::SemAnalyze(VEnvType venv, TEnvType tenv,
                             int labelcount) const
{
  // TODO: Put your codes here (lab4)
  if (this->body->SemAnalyze(venv, tenv, labelcount) != TY::VoidTy::Instance())
  {
    errormsg.Error(0, "while body must produce no value");
  }
  return TY::VoidTy::Instance();
}

TY::Ty *ForExp::SemAnalyze(VEnvType venv, TEnvType tenv, int labelcount) const
{
  // TODO: Put your codes here (lab4).
  if (this->lo->SemAnalyze(venv, tenv, labelcount) != TY::IntTy::Instance())
  {
    errormsg.Error(0, "for exp's range type is not integer");
  }
  if (this->hi->SemAnalyze(venv, tenv, labelcount) != TY::IntTy::Instance())
  {
    errormsg.Error(0, "for exp's range type is not integer");
  }

  E::VarEntry *iterator = new E::VarEntry(TY::IntTy::Instance(), true);
  venv->Enter(var, iterator);
  this->body->SemAnalyze(venv, tenv, FOR_ITER_CHECK);

  return TY::VoidTy::Instance();
}

TY::Ty *BreakExp::SemAnalyze(VEnvType venv, TEnvType tenv,
                             int labelcount) const
{
  // TODO: Put your codes here (lab4).
  return TY::VoidTy::Instance();
}

TY::Ty *LetExp::SemAnalyze(VEnvType venv, TEnvType tenv, int labelcount) const
{
  // TODO: Put your codes here (lab4). s
  DecList *d = decs;
  TY::Ty *result;
  venv->BeginScope();
  tenv->BeginScope();
  while (d && d->head)
  {
    d->head->SemAnalyze(venv, tenv, labelcount);
    d = d->tail;
  }
  result = this->body->SemAnalyze(venv, tenv, labelcount);
  venv->EndScope();
  tenv->EndScope();

  return result;
}

TY::Ty *ArrayExp::SemAnalyze(VEnvType venv, TEnvType tenv,
                             int labelcount) const
{
  // TODO: Put your codes here (lab4).

  TY::Ty *arr_type = tenv->Look(this->typ);
  //arr size this->size
  //check init value is validated
  TY::Ty *init_type = this->init->SemAnalyze(venv, tenv, labelcount);

  if (arr_type->kind != init_type->kind)
  {
    errormsg.Error(0, "type mismatch");
    return arr_type;
  }
  return arr_type;
}

TY::Ty *VoidExp::SemAnalyze(VEnvType venv, TEnvType tenv,
                            int labelcount) const
{
  // TODO: Put your codes here (lab4).
  return TY::VoidTy::Instance();
}

void FunctionDec::SemAnalyze(VEnvType venv, TEnvType tenv,
                             int labelcount) const
{
  // TODO: Put your codes here (lab4).
  FunDecList *fd = this->functions;
  while (fd && fd->head)
  {
    FunDec *f = fd->head;
    TY::Ty *resultTy;
    if (f->result)
    {
      resultTy = tenv->Look(f->result);
    }
    else
    {
      resultTy = TY::VoidTy::Instance();
    }

    TY::TyList *formalTys = make_formal_tylist(tenv, f->params);
    if (venv->Look(f->name))
    {
      // function  name already exists
      errormsg.Error(0, "two functions have the same name");
    }
    else
      venv->Enter(f->name, new E::FunEntry(formalTys, resultTy));
    fd = fd->tail;
  }
  fd = this->functions;
  while (fd && fd->head)
  {
    venv->BeginScope();
    FunDec *f = fd->head;

    TY::TyList *formalTys = make_formal_tylist(tenv, f->params);
    A::FieldList *l;
    TY::TyList *t;
    // local variable enter the venv
    for (l = f->params, t = formalTys; l; l = l->tail, t = t->tail)
    {
      venv->Enter(l->head->name, new E::VarEntry(t->head, false));
    }
    TY::Ty *returnvalue = f->body->SemAnalyze(venv, tenv, labelcount);

    //it's a procedure
    //check body return value or not
    if (f->result == NULL)
    {
      if (returnvalue != TY::VoidTy::Instance())
      {
        errormsg.Error(0, "procedure returns value");
      }
    }

    venv->EndScope();

    fd = fd->tail;
  }
}

void VarDec::SemAnalyze(VEnvType venv, TEnvType tenv, int labelcount) const
{
  // TODO: Put your codes here (lab4).

  if (this->typ)
  {
    TY::Ty *value_type = tenv->Look(this->typ);
    if (value_type)
    {
      E::VarEntry *entry = new E::VarEntry(value_type, false);
      venv->Enter(this->var, entry);
    }
    else
    {
      //unknown type
      errormsg.Error(0, "undefined type %s", this->typ->Name().c_str());
      return;
    }

    if (this->init)
    {
      //check the init type equals the value_type
      TY::Ty *init_type = this->init->SemAnalyze(venv, tenv, labelcount);
      if (value_type->kind == TY::Ty::RECORD)
      {
        if (init_type && value_type != init_type)
        {
          errormsg.Error(0, "type mismatch");
        }
      }
      else if (init_type && value_type->kind != init_type->kind)
      {

        errormsg.Error(0, "type mismatch");
      }
    }
  }
  else
  {
    TY::Ty *value_type = this->init->SemAnalyze(venv, tenv, labelcount);
    if (value_type)
    {
      if (value_type->kind == TY::Ty::NIL)
      {
        errormsg.Error(0, "init should not be nil without type specified");
      }

      E::VarEntry *entry = new E::VarEntry(value_type, false);
      venv->Enter(this->var, entry);
    }
  }
}

void TypeDec::SemAnalyze(VEnvType venv, TEnvType tenv, int labelcount) const
{

  NameAndTyList *d = this->types;
  while (d && d->head)
  {
    NameAndTy *h = d->head;
    if (tenv->Look(h->name))
    {
      //type name already exists
      errormsg.Error(0, "two types have the same name");
    }
    else
      tenv->Enter(h->name, new TY::NameTy(h->name, NULL));
    d = d->tail;
  }

  d = this->types;
  while (d && d->head)
  {
    NameAndTy *h = d->head;

    TY::Ty *value_type = h->ty->SemAnalyze(tenv);

    if (value_type && value_type == TY::VoidTy::Instance())
    {
      // encounter illegal cycle;
      break;
    }
    tenv->Set(h->name, value_type);
    d = d->tail;
  }
  // TODO: Put your codes here (lab4).
}

TY::Ty *NameTy::SemAnalyze(TEnvType tenv) const
{
  // TODO: Put your codes here (lab4).
  TY::Ty *value_type = tenv->Look(this->name);

  if (value_type)
  {
    if (value_type->kind == TY::Ty::NAME)
    // continue finding
    {
      TY::NameTy *name_ty;
      name_ty = (TY::NameTy *)tenv->Look(((TY::NameTy *)value_type)->sym);
      while (name_ty && name_ty->kind == TY::Ty::NAME)
      {
        if (!strcmp(name_ty->sym->Name().c_str(), this->name->Name().c_str()))
        {
          errormsg.Error(0, "illegal type cycle");
          return TY::VoidTy::Instance();
        }
        name_ty = (TY::NameTy *)tenv->Look(name_ty->sym);
      }
    }
    else
      return value_type;
  }
  else
  {
    errormsg.Error(0, "undefined type %s", this->name->Name());
    return TY::VoidTy::Instance();
  }
} // namespace A

TY::Ty *RecordTy::SemAnalyze(TEnvType tenv) const
{
  // TODO: Put your codes here (lab4).

  TY::FieldList *f = make_fieldlist(tenv, this->record);
  return new TY::RecordTy(f);
}

TY::Ty *ArrayTy::SemAnalyze(TEnvType tenv) const
{
  // TODO: Put your codes here (lab4).
  TY::Ty *value_type = tenv->Look(this->array);
  if (value_type)
    return new TY::ArrayTy(value_type);
  else
  {
    return TY::VoidTy::Instance();
  }
}

} // namespace A

namespace SEM
{
void SemAnalyze(A::Exp *root)
{
  if (root)
    root->SemAnalyze(E::BaseVEnv(), E::BaseTEnv(), 0);
}

} // namespace SEM
