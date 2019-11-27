#include "tiger/codegen/codegen.h"
#include "tiger/translate/translate.h"
#include "tiger/frame/temp.h"
#include "tiger/frame/frame.h"

#include "tiger/translate/tree.h"
namespace CG
{

static TEMP::Temp *munchExp(T::Exp *exp);
static void munchStm(T::Stm *stm);
static AS::InstrList *instrList, *cur;
static TEMP::Temp *savedrbx, *savedrbp, *savedr12, *savedr13, *savedr14, *savedr15;
static void emit(AS::Instr *inst)
{
  if (!instrList)
    instrList = cur = new AS::InstrList(inst, NULL);
  else
  {
    cur->tail = new AS::InstrList(inst, NULL);
    cur = cur->tail;
  }
}
static TEMP::Temp *munchOpExp(T::BinopExp *exp)
{
  return nullptr;
}

static TEMP::Temp *munchMemExp(T::MemExp *exp)
{
  return nullptr;
}
static TEMP::Temp *munchNameExp(T::NameExp *exp)
{
  TEMP::Temp *r = TEMP::Temp::NewTemp();
  std::string assem = "movq $"+ exp ->name->Name()+",`d0";
  emit(new AS::OperInstr(assem ,
                         new TEMP::TempList(r, NULL), NULL, new AS::Targets(nullptr)));
  return r;
}
static TEMP::TempList *munchArgs(int i, T::ExpList *args)
{
  if (!args)
    return NULL;
  TEMP::TempList *right = munchArgs(i + 1, args->tail);
  TEMP::Temp *left = munchExp(args->head);
  emit(new AS::OperInstr("pushl `s0", new TEMP::TempList(F::F_SP(), NULL),
                         new TEMP::TempList(left, new TEMP::TempList(F::F_SP(), NULL)), NULL));
  return new TEMP::TempList(left, right);
}

//caller_saved register: rax rdi rsi rdx rcx r8 r9 r10 r11
//callee_saved register: rbx rbp r12 r13 r14 r15
static void saveCallerRegs(void)
{
  TEMP::TempList *regs = F::F_callerSaveRegs();
  int offset = -4;
  for (; regs; regs = regs->tail)
  {
    TEMP::Temp *reg = regs->head; 
    std::string assem = "movl `s0, "+std::to_string (offset) +"(`s1)";
    emit(new AS::OperInstr(assem, NULL, new TEMP::TempList(reg, new TEMP::TempList(F::F_FP(), NULL)), nullptr));
    offset -= wordsize;
  }
}

int TempListLength(TEMP::TempList *l)
{
  if (!l)
    return 0;
  return TempListLength(l->tail) + 1;
}
static TEMP::Temp *munchExp(T::Exp *exp)
{
  switch (exp->kind)
  {
  case T::Exp::BINOP:
  {
    return munchOpExp((T::BinopExp *)exp);
  }
  case T::Exp::MEM:
  {
    return munchMemExp((T::MemExp *)exp);
  }
  case T::Exp::TEMP:
  {
    return ((T::TempExp *)exp)->temp;
  }
  case T::Exp::ESEQ:
  {
    munchStm(((T::EseqExp *)exp)->stm);
    return munchExp(((T::EseqExp *)exp)->exp);
  }
  case T::Exp::NAME:
  {

    return munchNameExp((T::NameExp *)exp);
  }
  case T::Exp::CONST:
  {
    TEMP::Temp *r = TEMP::Temp::NewTemp();
    emit(new AS::OperInstr("movl $%d, `d0" + ((T::ConstExp *)exp)->consti, new TEMP::TempList(r, NULL), nullptr, nullptr));
    return r;
  }
  case T::Exp::CALL:
  {
    //  munchArgs(e->u.CALL.args);
    T::Exp *func_exp = (((T::CallExp *)exp)->fun);
    assert(func_exp->kind == T::Exp::NAME);
    std::string func_name = TEMP::LabelString(((T::NameExp *)func_exp)->name);
    T::ExpList *args = ((T::CallExp *)exp)->args;
    TEMP::TempList *argtemps = munchArgs(0, args);
    int argnum = TempListLength(argtemps);
    saveCallerRegs();
    //to do
  }
  }
}
static void munchMoveStm(T::MoveStm *stm)
{
  T::Exp *dst = stm->dst;
  T::Exp *src = stm->src;

  if (dst->kind == T::Exp::TEMP && src->kind == T::Exp::TEMP)
  {
    emit(new AS::MoveInstr("movl `s0, `d0",
                           new TEMP::TempList(((T::TempExp *)dst)->temp, NULL),
                           new TEMP::TempList(((T::TempExp *)src)->temp, NULL)));
    return;
  }
  TEMP::Temp *left = munchExp(src);
  if (dst->kind == T::Exp::TEMP)
  {
    emit(new AS::MoveInstr("movl `s0, `d0",
                           new TEMP::TempList(((T::TempExp *)dst)->temp, NULL),
                           new TEMP::TempList(left, NULL)));
    return;
  }

  assert(dst->kind == T::Exp::MEM);
  dst = ((T::MemExp *)dst)->exp;
  //to do T::exp:::binop
  TEMP::Temp *right = munchExp(dst);
  emit(new AS::OperInstr("movl `s0, (`s1)", 0,
                         new TEMP::TempList(left, new TEMP::TempList(right, nullptr)),
                         new AS::Targets(NULL)));
}
static void munchStm(T::Stm *stm)
{
  switch (stm->kind)
  {
  /*movq mem,reg; movq reg,mem; movq irr,mem; movq reg,reg*/
  /* need to munch src exp first */
  case T::Stm::MOVE:
  {
    return munchMoveStm((T::MoveStm *)stm);
  }
  case T::Stm::LABEL:
  {
    TEMP::Label *label = ((T::LabelStm *)stm)->label;
    emit(new AS::LabelInstr("%s:" + label->Name(), label));
    return;
  }
  case T::Stm::CJUMP:
  {
    std::string op;
    TEMP::Temp *left = munchExp(((T::CjumpStm *)stm)->left);
    TEMP::Temp *right = munchExp(((T::CjumpStm *)stm)->right);
    TEMP::Label *true_label = ((T::CjumpStm *)stm)->true_label;
    switch (((T::CjumpStm *)stm)->op)
    {
    case T::EQ_OP:
      op = "je";
      break;
    case T::NE_OP:
      op = "jne";
      break;
    case T::LT_OP:
      op = "jl";
      break;
    case T::GT_OP:
      op = "jg";
      break;
    case T::LE_OP:
      op = "jle";
      break;
    case T::GE_OP:
      op = "jge";
      break;
    }
    emit(new AS::OperInstr("cmp `s0,`s1", NULL,
                           new TEMP::TempList(left, new TEMP::TempList(right, NULL)),
                           new AS::Targets(NULL)));

    emit(new AS::OperInstr(op + "`j0", NULL, NULL, new AS::Targets(new TEMP::LabelList(true_label, NULL))));
  }
  case T::Stm::JUMP:
  {
    std::string label = ((T::NameExp *)(((T::JumpStm *)stm)->exp))->name->Name();
    emit(new AS::OperInstr(label, NULL, NULL,
                           new AS::Targets(((T::JumpStm *)stm)->jumps)));
  }
  case T::Stm::EXP:
  {
    munchExp(((T::ExpStm *)stm)->exp);
    break;
  }
  }
}
static void saveCalleeRegs()
{

  savedrbx = TEMP::Temp::NewTemp();
  savedrbp = TEMP::Temp::NewTemp(); //Saved rbp here.
  savedr12 = TEMP::Temp::NewTemp();
  savedr13 = TEMP::Temp::NewTemp();
  savedr14 = TEMP::Temp::NewTemp();
  savedr15 = TEMP::Temp::NewTemp();
  emit(new AS::MoveInstr("movq `s0,`d0", new TEMP::TempList(savedrbx, NULL), new TEMP::TempList(F::F_RBX(), NULL)));
  emit(new AS::MoveInstr("movq `s0,`d0", new TEMP::TempList(savedrbp, NULL), new TEMP::TempList(F::F_RBP(), NULL)));
  emit(new AS::MoveInstr("movq `s0,`d0", new TEMP::TempList(savedr12, NULL), new TEMP::TempList(F::F_R12(), NULL)));
  emit(new AS::MoveInstr("movq `s0,`d0", new TEMP::TempList(savedr13, NULL), new TEMP::TempList(F::F_R13(), NULL)));
  emit(new AS::MoveInstr("movq `s0,`d0", new TEMP::TempList(savedr14, NULL), new TEMP::TempList(F::F_R14(), NULL)));
  emit(new AS::MoveInstr("movq `s0,`d0", new TEMP::TempList(savedr15, NULL), new TEMP::TempList(F::F_R15(), NULL)));
}
static void restoreCalleeRegs(void)
{
  emit(new AS::MoveInstr("movq `s0,`d0", new TEMP::TempList(F::F_RBX(), NULL), new TEMP::TempList(savedrbx, NULL)));
  emit(new AS::MoveInstr("movq `s0,`d0", new TEMP::TempList(F::F_RBP(), NULL), new TEMP::TempList(savedrbp, NULL)));
  emit(new AS::MoveInstr("movq `s0,`d0", new TEMP::TempList(F::F_R12(), NULL), new TEMP::TempList(savedr12, NULL)));
  emit(new AS::MoveInstr("movq `s0,`d0", new TEMP::TempList(F::F_R13(), NULL), new TEMP::TempList(savedr13, NULL)));
  emit(new AS::MoveInstr("movq `s0,`d0", new TEMP::TempList(F::F_R14(), NULL), new TEMP::TempList(savedr14, NULL)));
  emit(new AS::MoveInstr("movq `s0,`d0", new TEMP::TempList(F::F_R15(), NULL), new TEMP::TempList(savedr15, NULL)));
}
AS::InstrList *Codegen(F::Frame *f, T::StmList *stmList)
{
  // TODO: Put your codes here (lab6).

  AS::InstrList *list;

  saveCalleeRegs();
  for (; stmList; stmList = stmList->tail)
  {
    munchStm(stmList->head);
  }
  restoreCalleeRegs();
  return F::F_procEntryExit2(instrList);
}

} // namespace CG