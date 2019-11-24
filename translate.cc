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

  lv = new Level(F::F_newFrame(TEMP::NamedLabel("main"), nullptr), nullptr);
  return lv;
}

void procEntryExit(Level *level, TR::Exp *func_body);
F::FragList *TranslateProgram(A::Exp *root)
{
  // TODO: Put your codes here (lab5).

  TR::Level *main_level = Outermost();

  F::FragList *frags = new F::FragList(nullptr, nullptr);
  TR::ExpAndTy mainexp = root->Translate(E::BaseVEnv(), E::BaseTEnv(), main_level, nullptr);

  TR::procEntryExit(main_level, mainexp.exp);

  return frags->tail;
}

void procEntryExit(Level *level, TR::Exp *func_body)
{
  // pack F_procEntryExit1,2,3

  T::MoveStm *adden = new T::MoveStm(new T::TempExp(F::F_RV()), func_body->UnEx()); //STORE func return value

  T::Stm  *stm = F_procEntryExit1(level->frame, adden);

  F::ProcFrag *head = new F::ProcFrag(stm, level->frame);
  // F_frag head = F_ProcFrag(stm, level->frame);

  // The added frag is the head of the new frags.
  // fragtail->tail = F_FragList(head, NULL);
  // fragtail = fragtail->tail;
}
Level *Level::NewLevel(Level *parent, TEMP::Label *name,
                       U::BoolList *formals)
{
  return nullptr;
  TR::Level *level = new Level(nullptr, parent);
  U::BoolList *link_added_formals = new U::BoolList(true, formals);
  // F_frame frame = F_newFrame(name, link_added_formals);
  // level->frame = frame;
  // level->parent = parent;
  return level;
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
  T::Exp *var_exp = access->access->ToExp(frame);

  return TR::ExpAndTy(new TR::ExExp(var_exp), ((E::VarEntry *)entry)->ty);
}

TR::ExpAndTy FieldVar::Translate(S::Table<E::EnvEntry> *venv,
                                 S::Table<TY::Ty> *tenv, TR::Level *level,
                                 TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  // get type symbol field num
  int order = 0;

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

  TR::ExExp * frame = ( TR::ExExp *)(var_exp_ty.exp);
  T::MemExp * res = new T::MemExp(new T::BinopExp(T::PLUS_OP, frame->UnEx(), new T::ConstExp(order * wordsize))); //static link is the first escaped arg;
 

  return TR::ExpAndTy(new TR::ExExp(res), var_exp_ty.ty);
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

  if (elsee)
  {

    TR::ExpAndTy else_tr = this->elsee->Translate(venv, tenv, level, label);
    TEMP::Label *meeting = TEMP::NewLabel();

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

  TEMP::Label *incloop_label = TEMP::NewLabel();
  TEMP::Label *body_label = TEMP::NewLabel();
  TEMP::Label *done = TEMP::NewLabel();
  TR::Access *iter_access = TR::Access::AllocLocal(level, this->escape);
  //VarEntry

  venv->BeginScope();
  E::VarEntry *iterator = new E::VarEntry(iter_access, TY::IntTy::Instance(), true);
  venv->Enter(var, iterator);
  TR::ExpAndTy low = this->lo->Translate(venv, tenv, level, label);
  TR::ExpAndTy high = this->hi->Translate(venv, tenv, level, label);
  TR::ExpAndTy body_ = this->body->Translate(venv, tenv, level, done);
  venv->EndScope();
  //i++
  T::Exp *loopvar = iter_access->access->ToExp(new T::TempExp(F::F_FP()));
  T::MoveStm *incloop = new T::MoveStm(loopvar,
                                       new T::BinopExp(T::PLUS_OP, loopvar, new T::ConstExp(1)));

  /*if(i < hi) {i++; goto body;}*/
  T::SeqStm *test = new T::SeqStm(new T::CjumpStm(T::LE_OP, loopvar, high.exp->UnEx(), incloop_label, done),
                                  new T::SeqStm(new T::LabelStm(incloop_label),
                                                new T::SeqStm(incloop,
                                                              new T::JumpStm(new T::NameExp(body_label), new TEMP::LabelList(body_label, NULL)))));

  T::CjumpStm *checklohi = new T::CjumpStm(T::LE_OP, low.exp->UnEx(), high.exp->UnEx(), body_label, done);

  T::SeqStm *for_exp = new T::SeqStm(checklohi,
                                     new T::SeqStm(new T::LabelStm(body_label),
                                                   new T::SeqStm(body_.exp->UnNx(),
                                                                 new T::SeqStm(test, new T::LabelStm(done)))));
  return TR::ExpAndTy(new TR::NxExp(for_exp), TY::VoidTy::Instance());
}

TR::ExpAndTy BreakExp::Translate(S::Table<E::EnvEntry> *venv,
                                 S::Table<TY::Ty> *tenv, TR::Level *level,
                                 TEMP::Label *done) const
{
  // TODO: Put your codes here (lab5).
  T::JumpStm *breakexp = new T::JumpStm(new T::NameExp(done), new TEMP::LabelList(done, NULL));
  return TR::ExpAndTy(new TR::NxExp(breakexp), TY::VoidTy::Instance());
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
  //Assume that all params are escaped.
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
