#include "tiger/translate/translate.h"

#include <cstdio>
#include <set>
#include <string>

#include "tiger/errormsg/errormsg.h"
#include "tiger/frame/temp.h"
#include "tiger/semant/semant.h"
#include "tiger/semant/types.h"
#include "tiger/util/util.h"

#include "string.h"

namespace TR
{

static F::FragList *frags = new F::FragList(nullptr, nullptr);
void addfrag(F::Frag *frag)
{
  frags = new F::FragList(frag, frags);
}
class Access
{
public:
  Level *level;
  F::Access *access;

  Access(Level *level, F::Access *access) : level(level), access(access) {}
  static Access *AllocLocal(Level *level, bool escape)
  {

    return nullptr;
  }
};

class AccessList
{
public:
  Access *head;
  AccessList *tail;

  AccessList(Access *head, AccessList *tail) : head(head), tail(tail) {}
};

class Level
{
public:
  F::Frame *frame;
  Level *parent;

  Level(F::Frame *frame, Level *parent) : frame(frame), parent(parent) {}
  AccessList *Formals(Level *level) { return nullptr; }

  static Level *NewLevel(Level *parent, TEMP::Label *name,
                         U::BoolList *formals);
};

class PatchList
{
public:
  TEMP::Label **head;
  PatchList *tail;

  PatchList(TEMP::Label **head, PatchList *tail) : head(head), tail(tail) {}
};

class Cx
{
public:
  PatchList *trues;
  PatchList *falses;
  T::Stm *stm;

  Cx(PatchList *trues, PatchList *falses, T::Stm *stm)
      : trues(trues), falses(falses), stm(stm) {}
};

class Exp
{
public:
  enum Kind
  {
    EX,
    NX,
    CX
  };

  Kind kind;

  Exp(Kind kind) : kind(kind) {}

  virtual T::Exp *UnEx() const = 0;
  virtual T::Stm *UnNx() const = 0;
  virtual Cx UnCx() const = 0;
};

class ExpAndTy
{
public:
  TR::Exp *exp;
  TY::Ty *ty;

  ExpAndTy(TR::Exp *exp, TY::Ty *ty) : exp(exp), ty(ty) {}
};

class ExExp : public Exp
{
public:
  T::Exp *exp;

  ExExp(T::Exp *exp) : Exp(EX), exp(exp) {}

  T::Exp *UnEx() const override
  {
    return this->exp;
  }
  T::Stm *UnNx() const override
  {
    return new T::ExpStm(this->exp);
  }
  Cx UnCx() const override {}
};

class NxExp : public Exp
{
public:
  T::Stm *stm;

  NxExp(T::Stm *stm) : Exp(NX), stm(stm) {}

  T::Exp *UnEx() const override
  {
    return new T::EseqExp(this->stm, new T::ConstExp(0));
  }
  T::Stm *UnNx() const override
  {
    return this->stm;
  }
  Cx UnCx() const override {}
};

void do_patch(PatchList *tList, TEMP::Label *label)
{
  for (; tList; tList = tList->tail)
    *(tList->head) = label;
}

class CxExp : public Exp
{
public:
  Cx cx;

  CxExp(struct Cx cx) : Exp(CX), cx(cx) {}
  CxExp(PatchList *trues, PatchList *falses, T::Stm *stm)
      : Exp(CX), cx(trues, falses, stm) {}

  T::Exp *UnEx() const override
  {
    TEMP::Temp *r = TEMP::Temp::NewTemp();
    TEMP::Label *t = TEMP::NewLabel();
    TEMP::Label *f = TEMP::NewLabel();
    do_patch(this->cx.trues, t);
    do_patch(this->cx.falses, f);
    return new T::EseqExp(new T::MoveStm(new T::TempExp(r), new T::ConstExp(1)),
                          new T::EseqExp(this->cx.stm,
                                         new T::EseqExp(new T::LabelStm(f),
                                                        new T::EseqExp(new T::MoveStm(new T::TempExp(r), new T::ConstExp(0)),
                                                                       new T::EseqExp(new T::LabelStm(t), new T::TempExp(r))))));
  }
  T::Stm *UnNx() const override {}
  Cx UnCx() const override
  {
    return this->cx;
  }
};

PatchList *join_patch(PatchList *first, PatchList *second)
{
  if (!first)
    return second;
  for (; first->tail; first = first->tail)
    ;
  first->tail = second;
  return first;
}

Level *Outermost()
{
  static Level *lv = nullptr;
  if (lv != nullptr)
    return lv;

  lv = new Level(nullptr, nullptr);
  return lv;
}
F::FragList *TranslateProgram(A::Exp *root)
{
  // TODO: Put your codes here (lab5).

  F::FragList *frags = new F::FragList(nullptr, nullptr);
  TR::ExpAndTy exp_and_ty = root->Translate(E::BaseVEnv(), E::BaseTEnv(), Outermost(), nullptr);
  TR::Exp *body = exp_and_ty.exp;
  T::Stm *stm = F::F_procEntryExit1(nullptr, body->UnNx());
  return frags;
}

} // namespace TR

namespace A
{

TR::ExpAndTy SimpleVar::Translate(S::Table<E::EnvEntry> *venv,
                                  S::Table<TY::Ty> *tenv, TR::Level *level,
                                  TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  // wrong  to do
  E::EnvEntry *value = venv->Look(this->sym);

  TR::ExExp *exexp = new TR::ExExp(new T::TempExp(TEMP::Temp::NewTemp()));
  return TR::ExpAndTy(exexp, ((E::VarEntry *)value)->ty);
}

TR::ExpAndTy FieldVar::Translate(S::Table<E::EnvEntry> *venv,
                                 S::Table<TY::Ty> *tenv, TR::Level *level,
                                 TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  E::EnvEntry *var;
  if (this->var->kind == SIMPLE)
  {
    //check if var is a record
    var = venv->Look(((SimpleVar *)this->var)->sym);
  }
  if (this->var->kind == FIELD)
  {
    //check if var is a record
    var = venv->Look(((FieldVar *)this->var)->sym);
  }

  // get type symbol fields
  TY::FieldList *f = ((TY::RecordTy *)((E::VarEntry *)var)->ty)->fields;
  while (f && f->head)
  {
    if (!strcmp(f->head->name->Name().c_str(), this->sym->Name().c_str()))
      return TR::ExpAndTy(nullptr, f->head->ty);
    f = f->tail;
  }
  // wrong  to do
  TR::ExExp *exexp = new TR::ExExp(new T::TempExp(TEMP::Temp::NewTemp()));
  return TR::ExpAndTy(exexp, TY::VoidTy::Instance());
}

TR::ExpAndTy SubscriptVar::Translate(S::Table<E::EnvEntry> *venv,
                                     S::Table<TY::Ty> *tenv, TR::Level *level,
                                     TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  E::EnvEntry *var = venv->Look(((SimpleVar *)this->var)->sym);
  TY::Ty *vartype = ((E::VarEntry *)var)->ty;

  TR::ExExp *exexp = new TR::ExExp(new T::TempExp(TEMP::Temp::NewTemp()));
  // wrong  to do
  return TR::ExpAndTy(exexp, vartype);
}

TR::ExpAndTy VarExp::Translate(S::Table<E::EnvEntry> *venv,
                               S::Table<TY::Ty> *tenv, TR::Level *level,
                               TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  return var->Translate(venv, tenv, level, label);
}

TR::ExpAndTy NilExp::Translate(S::Table<E::EnvEntry> *venv,
                               S::Table<TY::Ty> *tenv, TR::Level *level,
                               TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  return TR::ExpAndTy(nullptr, TY::NilTy::Instance());
}

TR::ExpAndTy IntExp::Translate(S::Table<E::EnvEntry> *venv,
                               S::Table<TY::Ty> *tenv, TR::Level *level,
                               TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).

  TR::ExExp *exexp = new TR::ExExp(new T::ConstExp(this->i));
  return TR::ExpAndTy(exexp, TY::IntTy::Instance());
}

TR::ExpAndTy StringExp::Translate(S::Table<E::EnvEntry> *venv,
                                  S::Table<TY::Ty> *tenv, TR::Level *level,
                                  TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  TEMP::Label *l = TEMP::NewLabel();
  F::StringFrag *strfrag = new F::StringFrag(l, this->s);
  // need to add to frags
  TR::addfrag(strfrag);
  TR::ExExp *exexp = new TR::ExExp(new T::NameExp(label));
  return TR::ExpAndTy(exexp, TY::StringTy::Instance());
}

TR::ExpAndTy CallExp::Translate(S::Table<E::EnvEntry> *venv,
                                S::Table<TY::Ty> *tenv, TR::Level *level,
                                TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  E::EnvEntry *value = venv->Look(this->func);
  TY::TyList *func_arg_type_list = ((E::FunEntry *)value)->formals;
  ExpList *arg_list = this->args;
  while (arg_list && arg_list->head && func_arg_type_list && func_arg_type_list->head)
  {
    TY::Ty *arg_type = arg_list->head->Translate(venv, tenv, level, label).ty;
    TY::Ty *func_arg_type = func_arg_type_list->head;
    arg_list = arg_list->tail;
    func_arg_type_list = func_arg_type_list->tail;
  }
  TEMP::Label *l = ((E::FunEntry *)value)->label;
  //static_link
  T::ExpList *staticlink;
  //calculate static_link
  // to do
  TR::ExExp *exexp = new TR::ExExp(new T::CallExp(new T::NameExp(l), staticlink));
  return TR::ExpAndTy(exexp, ((E::FunEntry *)value)->result);
}

TR::ExpAndTy OpExp::Translate(S::Table<E::EnvEntry> *venv,
                              S::Table<TY::Ty> *tenv, TR::Level *level,
                              TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).

  T::Exp *leftexp = this->left->Translate(venv, tenv, level, label).exp->UnEx();
  T::Exp *rightexp = this->right->Translate(venv, tenv, level, label).exp->UnEx();
  switch (this->oper)
  {
  case A::PLUS_OP:
  case A::MINUS_OP:
  case A::TIMES_OP:
  case A::DIVIDE_OP:
  {
    TR::ExExp *exexp = new TR::ExExp(new T::BinopExp(T::BinOp(this->oper), leftexp, rightexp));
    return TR::ExpAndTy(exexp, TY::IntTy::Instance());
  }
  case A::EQ_OP:
  case A::NEQ_OP:
  case A::LT_OP:
  case A::LE_OP:
  case A::GT_OP:
  case A::GE_OP:
  {
    TEMP::Label *t = TEMP::NewLabel();
    T::CjumpStm *cjumpstm = new T::CjumpStm(T::RelOp(this->oper - A::EQ_OP), leftexp, rightexp, nullptr, nullptr);
    TR::PatchList *trues = new TR::PatchList(&cjumpstm->true_label, nullptr);
    TR::PatchList *falses = new TR::PatchList(&cjumpstm->false_label, nullptr);
    TR::Cx *cx = new TR::Cx(trues, falses, cjumpstm);
    return TR::ExpAndTy(new TR::CxExp(*cx), TY::IntTy::Instance());
  }
  }
  assert(0);
  return TR::ExpAndTy(nullptr, TY::IntTy::Instance());
}

TR::ExpAndTy RecordExp::Translate(S::Table<E::EnvEntry> *venv,
                                  S::Table<TY::Ty> *tenv, TR::Level *level,
                                  TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  TY::Ty *record = tenv->Look(this->typ);

  //EFieldList * this->fields;
  // wrong  to do
  //return TR::ExpAndTy(new TR::ExExp(), record);

  return TR::ExpAndTy(nullptr, record);
}

TR::ExExp *Tr_Seq(TR::Exp *left, TR::Exp *right)
{
  /*Don't handle the situation where left is NULL*/
  T::EseqExp *e;
  if (right)
  {
    e = new T::EseqExp(left->UnNx(), right->UnEx());
  }
  else
  {
    assert(0);
  }
  return new TR::ExExp(e);
}

TR::ExpAndTy SeqExp::Translate(S::Table<E::EnvEntry> *venv,
                               S::Table<TY::Ty> *tenv, TR::Level *level,
                               TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  ExpList *explist = this->seq;
  TY::Ty *result;
  TR::ExExp *trexp = NULL;
  while (explist && explist->head)
  {
    Exp *exp = explist->head;
    TR::ExpAndTy expandty = exp->Translate(venv, tenv, level, label);
    result = expandty.ty;
    trexp = Tr_Seq(trexp, expandty.exp);
    explist = explist->tail;
  }
  return TR::ExpAndTy(trexp, result);
}

TR::ExpAndTy AssignExp::Translate(S::Table<E::EnvEntry> *venv,
                                  S::Table<TY::Ty> *tenv, TR::Level *level,
                                  TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).

  TR::ExpAndTy leftvalue = this->var->Translate(venv, tenv, level, label);
  TR::ExpAndTy right = this->exp->Translate(venv, tenv, level, label);
  TR::NxExp *assignexp = new TR::NxExp(new T::MoveStm(leftvalue.exp->UnEx(), right.exp->UnEx()));
  return TR::ExpAndTy(assignexp, TY::VoidTy::Instance());
}

TR::ExpAndTy IfExp::Translate(S::Table<E::EnvEntry> *venv,
                              S::Table<TY::Ty> *tenv, TR::Level *level,
                              TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  TR::ExpAndTy condition = this->test->Translate(venv, tenv, level, label);
  TR::ExpAndTy thenexp = this->then->Translate(venv, tenv, level, label);
  TR::Cx test_ = condition.exp->UnCx();
  TEMP::Temp *r = TEMP::Temp::NewTemp();
  TEMP::Label *t = TEMP::NewLabel();
  TEMP::Label *f = TEMP::NewLabel();
  do_patch(test_.trues, t);
  do_patch(test_.falses, f);

  T::Stm *s = condition.exp->UnCx().stm;
  ((T::CjumpStm *)s)->true_label = t;
  T::EseqExp *ifexp;
  if (elsee)
  {
    TR::ExpAndTy else_tr = this->elsee->Translate(venv, tenv, level, label);
    if (else_tr.ty != TY::VoidTy::Instance())
    {
      ifexp = new T::EseqExp(test_.stm,
                             new T::EseqExp(
                                 new T::LabelStm(t),
                                 new T::EseqExp(
                                     new T::MoveStm(new T::TempExp(r), thenexp.exp->UnEx()),
                                     new T::EseqExp(new T::LabelStm(f),
                                                    new T::EseqExp(new T::MoveStm(new T::TempExp(r), else_tr.exp->UnEx()), new T::TempExp(r))))));
      return TR::ExpAndTy(TR::ExExp(ifexp), thenexp.ty);
    }
    else
    {
      return TR::ExpAndTy(TR::NxExp(ifexp), thenexp.ty);
    }
  }
  else
  {
  }

  return TR::ExpAndTy(TR::Exexp(ifexp), thenexp.ty);
}

TR::ExpAndTy WhileExp::Translate(S::Table<E::EnvEntry> *venv,
                                 S::Table<TY::Ty> *tenv, TR::Level *level,
                                 TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  return TR::ExpAndTy(nullptr, TY::VoidTy::Instance());
}

TR::ExpAndTy ForExp::Translate(S::Table<E::EnvEntry> *venv,
                               S::Table<TY::Ty> *tenv, TR::Level *level,
                               TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  return TR::ExpAndTy(nullptr, TY::VoidTy::Instance());
}

TR::ExpAndTy BreakExp::Translate(S::Table<E::EnvEntry> *venv,
                                 S::Table<TY::Ty> *tenv, TR::Level *level,
                                 TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  return TR::ExpAndTy(nullptr, TY::VoidTy::Instance());
}

TR::ExpAndTy LetExp::Translate(S::Table<E::EnvEntry> *venv,
                               S::Table<TY::Ty> *tenv, TR::Level *level,
                               TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  return TR::ExpAndTy(nullptr, TY::VoidTy::Instance());
}

TR::ExpAndTy ArrayExp::Translate(S::Table<E::EnvEntry> *venv,
                                 S::Table<TY::Ty> *tenv, TR::Level *level,
                                 TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  return TR::ExpAndTy(nullptr, TY::VoidTy::Instance());
}

TR::ExpAndTy VoidExp::Translate(S::Table<E::EnvEntry> *venv,
                                S::Table<TY::Ty> *tenv, TR::Level *level,
                                TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  return TR::ExpAndTy(nullptr, TY::VoidTy::Instance());
}

TR::Exp *FunctionDec::Translate(S::Table<E::EnvEntry> *venv,
                                S::Table<TY::Ty> *tenv, TR::Level *level,
                                TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  return nullptr;
}

TR::Exp *VarDec::Translate(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                           TR::Level *level, TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  return nullptr;
}

TR::Exp *TypeDec::Translate(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                            TR::Level *level, TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  return nullptr;
}

TY::Ty *NameTy::Translate(S::Table<TY::Ty> *tenv) const
{
  // TODO: Put your codes here (lab5).
  return TY::VoidTy::Instance();
}

TY::Ty *RecordTy::Translate(S::Table<TY::Ty> *tenv) const
{
  // TODO: Put your codes here (lab5).
  return TY::VoidTy::Instance();
}

TY::Ty *ArrayTy::Translate(S::Table<TY::Ty> *tenv) const
{
  // TODO: Put your codes here (lab5).
  return TY::VoidTy::Instance();
}

} // namespace A
