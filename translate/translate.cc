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

#include "iostream"
namespace TR
{

TR::Exp *Tr_Seq(TR::Exp *left, TR::Exp *right);
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
  static Access *AllocLocal(Level *level, bool escape);
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
Access *Access::AllocLocal(Level *level, bool escape)
{
  return new Access(level, F::F_allocLocal(level->frame, escape));
}
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
  Cx UnCx() const override
  {
    Cx *uncx = new TR::Cx(nullptr, nullptr, nullptr);
    uncx->stm = new T::CjumpStm(T::NE_OP, this->exp, new T::ConstExp(0), NULL, NULL);
    uncx->trues = new PatchList(&(((T::CjumpStm *)uncx->stm)->true_label), NULL);
    uncx->falses = new PatchList(&(((T::CjumpStm *)uncx->stm)->false_label), NULL);
    return *uncx;
  }
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

TR::Exp *Nil()
{
  return new TR::ExExp(new T::ConstExp(0));
}
void do_patch(PatchList *tList, TEMP::Label *label)
{
  for (; tList && tList->head; tList = tList->tail)
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
  T::Stm *UnNx() const override
  {
    //don't know it will be called or not
    return new T::ExpStm(this->UnEx());
  }
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

  lv = new Level(F::F_newFrame(TEMP::NamedLabel("tigermain"), nullptr), nullptr);
  return lv;
}

void procEntryExit(Level *level, TR::Exp *func_body);
F::FragList *TranslateProgram(A::Exp *root)
{
  // TODO: Put your codes here (lab5).

  TR::Level *main_level = Outermost();

  TR::ExpAndTy mainexp = root->Translate(E::BaseVEnv(), E::BaseTEnv(), main_level, nullptr);

  TR::procEntryExit(main_level, mainexp.exp);

  //debug
  F::FragList *p = frags;
  while (p && p->head)
  {
    if (p->head->kind == F::Frag::PROC)
    {

      printf("------====FRAG:%s=====-------\n",
             ((F::ProcFrag *)(p->head))->frame->label->Name().c_str());
      T::Stm *s = ((F::ProcFrag *)(p->head))->body;
      FILE *out = stdout;
      s->Print(out, 0);
    }

    p = p->tail;
  }
  return frags;
}

void procEntryExit(Level *level, TR::Exp *func_body)
{
  // pack F_procEntryExit 1,2,3

  T::MoveStm *adden = new T::MoveStm(new T::TempExp(F::F_RV()), func_body->UnEx()); //STORE func return value

  T::Stm *func_body_stm = F_procEntryExit1(level->frame, adden);

  func_body = new TR::NxExp(adden);

  F::ProcFrag *head = new F::ProcFrag(func_body_stm, level->frame);
  addfrag(head);

  // The added frag is the head of the new frags.
}
TR::NxExp *generate_assign(TR::Exp *leftvalue, TR::Exp *right)
{
  TR::NxExp *assignexp = new TR::NxExp(new T::MoveStm(leftvalue->UnEx(), right->UnEx()));
  return assignexp;
}
Level *Level::NewLevel(Level *parent, TEMP::Label *name,
                       U::BoolList *formals)
{
  // make all formals escape
  U::BoolList *link_added_formals = new U::BoolList(true, formals);
  F::Frame *frame = F::F_newFrame(name, link_added_formals);
  TR::Level *level = new Level(frame, parent);
  return level;
}

TR::Exp *Tr_Seq(TR::Exp *left, TR::Exp *right)
{
  /*Don't handle the situation where left is NULL*/
  T::Exp *e;
  if (right)
  {
    e = new T::EseqExp(left->UnNx(), right->UnEx());
  }
  else
  {
    e = new T::EseqExp(left->UnNx(), new T::ConstExp(0));
  }
  return new TR::ExExp(e);
}
} // namespace TR

namespace A
{

TR::ExpAndTy SimpleVar::Translate(S::Table<E::EnvEntry> *venv,
                                  S::Table<TY::Ty> *tenv, TR::Level *level,
                                  TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  E::EnvEntry *entry = venv->Look(((SimpleVar *)this)->sym);
  TR::Access *access = ((E::VarEntry *)entry)->access;
  T::Exp *frame = new T::TempExp(F::F_FP());
  while (level != access->level)
  {
    frame = new T::MemExp(new T::BinopExp(T::PLUS_OP, frame, new T::ConstExp(-wordsize))); //static link is the first escaped arg;
    level = level->parent;
  }
  T::Exp *var_mem = access->access->ToExp(frame);

  return TR::ExpAndTy(new TR::ExExp(var_mem), ((E::VarEntry *)entry)->ty);
}

TR::ExpAndTy FieldVar::Translate(S::Table<E::EnvEntry> *venv,
                                 S::Table<TY::Ty> *tenv, TR::Level *level,
                                 TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  // get type symbol field num
  int order = 0;
  E::EnvEntry *var;
  if (this->var->kind == SIMPLE)
  {
    var = venv->Look(((SimpleVar *)this->var)->sym);
  }
  else
  {
    assert(0);
  }
  assert(((E::VarEntry *)var)->ty->kind == TY::Ty::RECORD);
  TY::FieldList *f = ((TY::RecordTy *)((E::VarEntry *)var)->ty)->fields;
  while (f && f->head)
  {
    if (!strcmp(f->head->name->Name().c_str(), this->sym->Name().c_str()))
    {
      break;
    }
    f = f->tail;
    order++;
  }
  // get the record var access
  TR::ExpAndTy var_exp_ty = this->var->Translate(venv, tenv, level, label);
  assert(var_exp_ty.exp->kind == TR::Exp::EX);

  TR::ExExp *access = (TR::ExExp *)(var_exp_ty.exp);
  T::MemExp *fieldvar_mem = new T::MemExp(new T::BinopExp(T::PLUS_OP, access->UnEx(), new T::ConstExp(order * wordsize))); //static link is the first escaped arg;

  // TEMP::Temp * fv =TEMP::Temp::NewTemp();
  // T::MoveStm *fieldvar = new T::MoveStm(new T::TempExp(fv) ,fieldvar_mem );

  return TR::ExpAndTy(new TR::ExExp(fieldvar_mem), f->head->ty);
}

TR::ExpAndTy SubscriptVar::Translate(S::Table<E::EnvEntry> *venv,
                                     S::Table<TY::Ty> *tenv, TR::Level *level,
                                     TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).

  TR::ExpAndTy var_exp_ty = this->var->Translate(venv, tenv, level, label);
  TR::ExpAndTy subscript_exp_ty = this->subscript->Translate(venv, tenv, level, label);

  TR::ExExp *frame = (TR::ExExp *)(var_exp_ty.exp);
  T::MemExp *subvar_mem = new T::MemExp(new T::BinopExp(T::PLUS_OP, frame->UnEx(),
                                                        new T::BinopExp(T::MUL_OP, subscript_exp_ty.exp->UnEx(), new T::ConstExp(wordsize)))); //static link is the first escaped arg;
 
  return TR::ExpAndTy(new TR::ExExp(subvar_mem), var_exp_ty.ty);
}

TR::ExpAndTy VarExp::Translate(S::Table<E::EnvEntry> *venv,
                               S::Table<TY::Ty> *tenv, TR::Level *level,
                               TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  return this->var->Translate(venv, tenv, level, label);
}

TR::ExpAndTy NilExp::Translate(S::Table<E::EnvEntry> *venv,
                               S::Table<TY::Ty> *tenv, TR::Level *level,
                               TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  return TR::ExpAndTy(TR::Nil(), TY::NilTy::Instance());
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
  TR::ExExp *exexp = new TR::ExExp(new T::NameExp(l));
  return TR::ExpAndTy(exexp, TY::StringTy::Instance());
}

TR::ExpAndTy CallExp::Translate(S::Table<E::EnvEntry> *venv,
                                S::Table<TY::Ty> *tenv, TR::Level *caller,
                                TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  E::EnvEntry *value = venv->Look(this->func);

  TR::Level *callee = ((E::FunEntry *)value)->level;
  // TY::TyList *func_arg_type_list = ((E::FunEntry *)value)->formals;
  ExpList *arg_list = this->args;

  T::ExpList *targs = new T::ExpList(NULL, NULL);
  T::ExpList *tail = targs;

  for (; arg_list; arg_list = arg_list->tail)
  {
    TR::ExpAndTy argval = arg_list->head->Translate(venv, tenv, caller, label);
    tail->tail = new T::ExpList(argval.exp->UnEx(), NULL);
    tail = tail->tail;
  }
  targs = targs->tail;

  //static_link
  //calculate static_link
  T::Exp *staticlink = new T::TempExp(F::F_FP());
  while (caller != callee->parent)
  {
    staticlink = new T::MemExp(new T::BinopExp(T::PLUS_OP, staticlink, new T::ConstExp(-wordsize)));
    //static link is the first escaped arg;
    caller = caller->parent;
  }
  if (callee->parent != NULL)
  {
    // NOT A EXTERNALCALL
    targs = new T::ExpList(staticlink, targs);
  }

  TEMP::Label *l = TEMP::NamedLabel(this->func->Name());

  T::CallExp *callexp = new T::CallExp(new T::NameExp(l), targs);
  TR::ExExp *exexp = new TR::ExExp(callexp);

  return TR::ExpAndTy(exexp, ((E::FunEntry *)value)->result);
}

TR::ExpAndTy OpExp::Translate(S::Table<E::EnvEntry> *venv,
                              S::Table<TY::Ty> *tenv, TR::Level *level,
                              TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  TR::ExpAndTy leftexp_ty = this->left->Translate(venv, tenv, level, label);
  T::Exp *leftexp, *rightexp;

  leftexp = leftexp_ty.exp->UnEx();

  rightexp = this->right->Translate(venv, tenv, level, label).exp->UnEx();
  if (this->oper == A::EQ_OP && leftexp_ty.ty->kind == TY::Ty::STRING)
  {
    //SHOULD USE STRINGEQUAL
    leftexp = F::F_externalCall("stringEqual", new T::ExpList(leftexp, new T::ExpList(rightexp, NULL)));
    rightexp = new T::ConstExp(1);
  }
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
    TR::PatchList *trues = new TR::PatchList(&(cjumpstm->true_label), nullptr);
    TR::PatchList *falses = new TR::PatchList(&(cjumpstm->false_label), nullptr);
    TR::Cx *cx = new TR::Cx(trues, falses, cjumpstm);
    return TR::ExpAndTy(new TR::CxExp(*cx), TY::IntTy::Instance());
  }
  }
  assert(0);
  return TR::ExpAndTy(TR::Nil(), TY::IntTy::Instance());
}

T::Stm *Tr_mk_record_array(T::ExpList *fields, T::Exp *r, int offset, int size)
{

  if (size > 1)
  {
    if (offset < size - 2)
    {
      return new T::SeqStm(
          new T::MoveStm(new T::MemExp(
                             new T::BinopExp(T::PLUS_OP, r, new T::ConstExp(offset * wordsize))),
                         fields->head),
          Tr_mk_record_array(fields->tail, r, offset + 1, size));
    }
    else
    {
      return new T::SeqStm(
          new T::MoveStm(new T::MemExp(
                             new T::BinopExp(T::PLUS_OP, r, new T::ConstExp(offset * wordsize))),
                         fields->head),
          new T::MoveStm(new T::MemExp(
                             new T::BinopExp(T::PLUS_OP, r, new T::ConstExp((offset + 1) * wordsize))),
                         fields->tail->head));
    }
  }
  else
  {
    return new T::MoveStm(new T::MemExp(
                              new T::BinopExp(T::PLUS_OP, r, new T::ConstExp(offset * wordsize))),
                          fields->head);
  }
}

TR::ExpAndTy RecordExp::Translate(S::Table<E::EnvEntry> *venv,
                                  S::Table<TY::Ty> *tenv, TR::Level *level,
                                  TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  TY::Ty *record = tenv->Look(this->typ);

  int count = 0;
  A::EFieldList *f = this->fields;
  T::ExpList *fi = new T::ExpList(nullptr, nullptr);
  T::ExpList *h = fi;
  while (f && f->head)
  {
    TR::ExpAndTy f_val_exp = f->head->exp->Translate(venv, tenv, level, label);
    TR::Exp *f_val = f_val_exp.exp;
    TY::Ty *f_val_ty = f_val_exp.ty;
    fi->tail = new T::ExpList(f_val->UnEx(), NULL);
    count++;
    f = f->tail;
    fi = fi->tail;
  }
  TEMP::Temp *r = TEMP::Temp::NewTemp();
  T::Stm *stm =
      new T::MoveStm(
          new T::TempExp(r),
          F::F_externalCall("allocRecord", new T::ExpList(new T::ConstExp(count * wordsize), NULL)));
  stm = new T::SeqStm(stm, Tr_mk_record_array(h->tail, new T::TempExp(r), 0, count));
  return TR::ExpAndTy(new TR::ExExp(new T::EseqExp(stm, new T::TempExp(r))), record);
}

TR::ExpAndTy SeqExp::Translate(S::Table<E::EnvEntry> *venv,
                               S::Table<TY::Ty> *tenv, TR::Level *level,
                               TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  ExpList *explist = this->seq;
  TY::Ty *result;
  TR::Exp *trexp = TR::Nil();
  while (explist && explist->head)
  {
    Exp *exp = explist->head;

    TR::ExpAndTy expandty = exp->Translate(venv, tenv, level, label);

    result = expandty.ty;
    trexp = TR::Tr_Seq(trexp, expandty.exp);
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

  if (elsee)
  {

    TR::ExpAndTy else_tr = this->elsee->Translate(venv, tenv, level, label);
    TEMP::Label *meeting = TEMP::NewLabel();
    assert(meeting);
    T::EseqExp *ifexp = new T::EseqExp(test_.stm,
                                       new T::EseqExp(
                                           new T::LabelStm(t),
                                           new T::EseqExp(
                                               new T::MoveStm(new T::TempExp(r), thenexp.exp->UnEx()),
                                               new T::EseqExp(new T::JumpStm(new T::NameExp(meeting), new TEMP::LabelList(meeting, NULL)),
                                                              new T::EseqExp(new T::LabelStm(f),
                                                                             new T::EseqExp(new T::MoveStm(new T::TempExp(r), else_tr.exp->UnEx()),
                                                                                            new T::EseqExp(new T::LabelStm(meeting),
                                                                                                           new T::TempExp(r))))))));
    if (else_tr.ty != TY::VoidTy::Instance())
    {
      return TR::ExpAndTy(new TR::ExExp(ifexp), thenexp.ty);
    }
    else
    {
      return TR::ExpAndTy(new TR::NxExp(new T::ExpStm(ifexp)), thenexp.ty);
    }
  }
  else
  {
    T::SeqStm *ifexp = new T::SeqStm(test_.stm,
                                     new T::SeqStm(
                                         new T::LabelStm(t),
                                         new T::SeqStm(
                                             thenexp.exp->UnNx(),
                                             new T::LabelStm(f))));

    return TR::ExpAndTy(new TR::NxExp(ifexp), thenexp.ty);
  }
  assert(0);
  return TR::ExpAndTy(nullptr, TY::VoidTy::Instance());
}

TR::ExpAndTy WhileExp::Translate(S::Table<E::EnvEntry> *venv,
                                 S::Table<TY::Ty> *tenv, TR::Level *level,
                                 TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  TEMP::Label *body_label = TEMP::NewLabel();
  TEMP::Label *done = TEMP::NewLabel();

  TEMP::Label *tst = TEMP::NewLabel();

  TR::ExpAndTy condition = this->test->Translate(venv, tenv, level, label);
  TR::Cx test_ = condition.exp->UnCx();

  do_patch(test_.trues, body_label);
  do_patch(test_.falses, done);

  TR::ExpAndTy body_ = this->body->Translate(venv, tenv, level, done);

  T::SeqStm *while_exp = new T::SeqStm(new T::LabelStm(tst),
                                       new T::SeqStm(
                                           test_.stm, new T::SeqStm(
                                                          new T::LabelStm(body_label),
                                                          new T::SeqStm(
                                                              body_.exp->UnNx(),
                                                              new T::SeqStm(new T::JumpStm(new T::NameExp(tst), new TEMP::LabelList(tst, NULL)),
                                                                            new T::LabelStm(done))))));

  return TR::ExpAndTy(new TR::NxExp(while_exp), TY::VoidTy::Instance());
}

TR::ExpAndTy ForExp::Translate(S::Table<E::EnvEntry> *venv,
                               S::Table<TY::Ty> *tenv, TR::Level *level,
                               TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).

  T::TempExp *limit = new T::TempExp(TEMP::Temp::NewTemp());
  TEMP::Label *incloop_label = TEMP::NewLabel();
  TEMP::Label *body_label = TEMP::NewLabel();
  TEMP::Label *done = TEMP::NewLabel();
  TR::Access *iter_access = TR::Access::AllocLocal(level, this->escape);
  //VarEntry

  venv->BeginScope();
  E::VarEntry *iterator = new E::VarEntry(iter_access, TY::IntTy::Instance(), true);
  venv->Enter(this->var, iterator);
  TR::ExpAndTy low = this->lo->Translate(venv, tenv, level, label);
  TR::ExpAndTy high = this->hi->Translate(venv, tenv, level, label);
  TR::ExpAndTy body_ = this->body->Translate(venv, tenv, level, done);

  //i++
  T::Exp *loopvar = iter_access->access->ToExp(new T::TempExp(F::F_FP()));
  T::MoveStm *incloop = new T::MoveStm(loopvar,
                                       new T::BinopExp(T::PLUS_OP, loopvar, new T::ConstExp(1)));

  T::SeqStm *init = new T::SeqStm(new T::MoveStm(loopvar, low.exp->UnEx()),
                                  new T::MoveStm(limit, high.exp->UnEx()));

  /*if(i < limit) {i++; goto body;}*/
  assert(body_label);
  T::SeqStm *test = new T::SeqStm(new T::CjumpStm(T::LT_OP, loopvar, limit, incloop_label, done),
                                  new T::SeqStm(new T::LabelStm(incloop_label),
                                                new T::SeqStm(incloop,
                                                              new T::JumpStm(new T::NameExp(body_label), new TEMP::LabelList(body_label, NULL)))));

  T::CjumpStm *checklohi = new T::CjumpStm(T::LE_OP, loopvar, limit, body_label, done);

  T::SeqStm *for_exp = new T::SeqStm(init, new T::SeqStm(checklohi,
                                                         new T::SeqStm(new T::LabelStm(body_label),
                                                                       new T::SeqStm(body_.exp->UnNx(),
                                                                                     new T::SeqStm(test, new T::LabelStm(done))))));
  venv->EndScope();
  return TR::ExpAndTy(new TR::NxExp(for_exp), TY::VoidTy::Instance());
}

TR::ExpAndTy BreakExp::Translate(S::Table<E::EnvEntry> *venv,
                                 S::Table<TY::Ty> *tenv, TR::Level *level,
                                 TEMP::Label *done) const
{
  // TODO: Put your codes here (lab5).
  assert(done);
  T::JumpStm *breakexp = new T::JumpStm(new T::NameExp(done), new TEMP::LabelList(done, NULL));
  return TR::ExpAndTy(new TR::NxExp(breakexp), TY::VoidTy::Instance());
}

TR::ExpAndTy LetExp::Translate(S::Table<E::EnvEntry> *venv,
                               S::Table<TY::Ty> *tenv, TR::Level *level,
                               TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).

  venv->BeginScope();
  tenv->BeginScope();
  DecList *iter = this->decs;

  TR::Exp *trSeq = TR::Nil();
  while (iter && iter->head)
  {
    TR::Exp *dec_exp = (iter->head)->Translate(venv, tenv, level, label);
    trSeq = TR::Tr_Seq(trSeq, dec_exp);
    iter = iter->tail;
  }

  TR::ExpAndTy body_exp = this->body->Translate(venv, tenv, level, label);
  TR::Exp *trlet = Tr_Seq(trSeq, body_exp.exp);
  tenv->EndScope();
  venv->EndScope();
  return TR::ExpAndTy(trlet, body_exp.ty);
}

TR::ExpAndTy ArrayExp::Translate(S::Table<E::EnvEntry> *venv,
                                 S::Table<TY::Ty> *tenv, TR::Level *level,
                                 TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  TY::Ty *value_type = tenv->Look(this->typ);

  TR::ExpAndTy size_exp = this->size->Translate(venv, tenv, level, label);
  TR::ExpAndTy init_exp = this->init->Translate(venv, tenv, level, label);

  T::Exp *callinitArray = F::F_externalCall("initArray",
                                            new T::ExpList(size_exp.exp->UnEx(),
                                                           new T::ExpList(init_exp.exp->UnEx(), NULL)));
  return TR::ExpAndTy(new TR::ExExp(callinitArray), value_type);
}

TR::ExpAndTy VoidExp::Translate(S::Table<E::EnvEntry> *venv,
                                S::Table<TY::Ty> *tenv, TR::Level *level,
                                TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  return TR::ExpAndTy(TR::Nil(), TY::VoidTy::Instance());
}

static TY::TyList *make_formal_tylist(S::Table<TY::Ty> *tenv, A::FieldList *params)
{
  if (params == nullptr)
  {
    return nullptr;
  }

  TY::Ty *ty = tenv->Look(params->head->typ);

  return new TY::TyList(ty->ActualTy(), make_formal_tylist(tenv, params->tail));
}

U::BoolList *makeFormalEscList(A::FieldList *params)
{
  if (params == NULL)
  {
    return NULL;
  }
  return new U::BoolList(params->head->escape, makeFormalEscList(params->tail));
}
TR::Exp *FunctionDec::Translate(S::Table<E::EnvEntry> *venv,
                                S::Table<TY::Ty> *tenv, TR::Level *level,
                                TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  //Assume that all params are escaped.

  FunDecList *fd = this->functions;
  //First loop:push function names into venv to handle recursive function

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
    U::BoolList *args = makeFormalEscList(f->params);

    TR::Level *newlevel = TR::Level::NewLevel(level, TEMP::NamedLabel(f->name->Name()), args);
    TY::TyList *formalTys = make_formal_tylist(tenv, f->params);
    TEMP::Label *func_label = TEMP::NamedLabel(f->name->Name());
    assert(func_label);
    venv->Enter(f->name, new E::FunEntry(newlevel, func_label, formalTys, resultTy));
    fd = fd->tail;
  }
  fd = this->functions;

  //Second loop:handle

  while (fd && fd->head)
  {
    venv->BeginScope();
    FunDec *f = fd->head;

    TY::TyList *formalTys = make_formal_tylist(tenv, f->params);
    A::FieldList *l;
    TY::TyList *t;
    // TR::AccessList accesslist =  ;
    E::EnvEntry *funentry = venv->Look(f->name);
    TR::Level *func_level = ((E::FunEntry *)funentry)->level;

    F::Frame *frame = func_level->frame;
    F::AccessList *formal_accesslist = frame->formals;
    //formal enter the venv

    // //DEBUG
    //    FILE *out = stdout;
    //    A::FieldList::Print(out, f->params, 10);
    // //DEBUG
    for (l = f->params, t = formalTys; l; l = l->tail, t = t->tail)
    {
      venv->Enter(l->head->name, new E::VarEntry(
                                     new TR::Access(func_level, formal_accesslist->head),
                                     t->head, false));
      formal_accesslist = formal_accesslist->tail;
    }

    TR::Exp *body_exp = f->body->Translate(venv, tenv, func_level, label).exp;

    TR::procEntryExit(func_level, body_exp);

    venv->EndScope();
    fd = fd->tail;
  }
  return TR::Nil();
}

TR::Exp *VarDec::Translate(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                           TR::Level *level, TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  TR::ExpAndTy init_exp = this->init->Translate(venv, tenv, level, label);
  TY::Ty *type_of_var;
  if (this->typ)
    type_of_var = tenv->Look(this->typ);
  else
    type_of_var = init_exp.ty;

  F::Access *varaccess = F::F_allocLocal(level->frame, this->escape);

  E::VarEntry *entry = new E::VarEntry(new TR::Access(level, varaccess), type_of_var, false);
  venv->Enter(this->var, entry);

  TR::NxExp *res = TR::generate_assign(new TR::ExExp(varaccess->ToExp(new T::TempExp(F::F_FP()))), init_exp.exp);
  return res;
}

TR::Exp *TypeDec::Translate(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                            TR::Level *level, TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).

  NameAndTyList *d = this->types;
  while (d && d->head)
  {
    NameAndTy *h = d->head;
    tenv->Enter(h->name, new TY::NameTy(h->name, NULL));
    d = d->tail;
  }

  d = this->types;
  while (d && d->head)
  {
    NameAndTy *h = d->head;

    TY::Ty *value_type = h->ty->Translate(tenv);
    tenv->Set(h->name, value_type);
    d = d->tail;
  }
  return TR::Nil();
}

TY::Ty *NameTy::Translate(S::Table<TY::Ty> *tenv) const
{
  // TODO: Put your codes here (lab5).
  TY::Ty *value_type = tenv->Look(this->name);

  if (value_type)
  {
    if (value_type->kind == TY::Ty::NAME)
    {
      TY::NameTy *name_ty;
      name_ty = (TY::NameTy *)tenv->Look(((TY::NameTy *)value_type)->sym);
      while (name_ty && name_ty->kind == TY::Ty::NAME)
      {
        name_ty = (TY::NameTy *)tenv->Look(name_ty->sym);
      }
    }
    else
      return value_type;
  }
  assert(0);
  return TY::VoidTy::Instance();
}

static TY::FieldList *make_fieldlist(S::Table<TY::Ty> *tenv, A::FieldList *fields)
{
  if (fields == nullptr)
  {
    return nullptr;
  }
  TY::Ty *ty = tenv->Look(fields->head->typ);
  return new TY::FieldList(new TY::Field(fields->head->name, ty),
                           make_fieldlist(tenv, fields->tail));
}
TY::Ty *RecordTy::Translate(S::Table<TY::Ty> *tenv) const
{
  // TODO: Put your codes here (lab5).
  TY::FieldList *f = make_fieldlist(tenv, this->record);
  return new TY::RecordTy(f);
}
TY::Ty *ArrayTy::Translate(S::Table<TY::Ty> *tenv) const
{
  TY::Ty *value_type = tenv->Look(this->array);
  if (value_type)
    return new TY::ArrayTy(value_type);
}

} // namespace A
