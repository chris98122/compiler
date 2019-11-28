#include "tiger/frame/frame.h"

#include <string>

namespace F
{

static TEMP::Temp *rbp = NULL;
static TEMP::Temp *rax = NULL;
static TEMP::Temp *rdi = NULL;
static TEMP::Temp *rsi = NULL;
static TEMP::Temp *rdx = NULL;
static TEMP::Temp *rcx = NULL;
static TEMP::Temp *r8 = NULL;
static TEMP::Temp *r9 = NULL;

static TEMP::Temp *r10 = NULL;
static TEMP::Temp *r11 = NULL;
static TEMP::Temp *r12 = NULL;
static TEMP::Temp *r13 = NULL;
static TEMP::Temp *r14 = NULL;
static TEMP::Temp *r15 = NULL;

static TEMP::Temp *rsp = NULL;
static TEMP::Temp *rbx = NULL;

//caller_saved register: rax rdi rsi rdx rcx r8 r9 r10 r11
TEMP::TempList *F_callerSaveRegs()
{
  return new TEMP::TempList(F_RAX(),
                            new TEMP::TempList(F_RDI(),
                                               new TEMP::TempList(F_RSI(),
                                                                  new TEMP::TempList(F_RDX(),
                                                                                     new TEMP::TempList(F_RCX(),
                                                                                                        new TEMP::TempList(F_R8(),
                                                                                                                           new TEMP::TempList(F_R9(),
                                                                                                                                              new TEMP::TempList(F_R10(),
                                                                                                                                                                 new TEMP::TempList(F_R11(), NULL)))))))));
}
TEMP::Temp *F_RBP(void)
{
  if (!rbp)
    rbp = TEMP::Temp::NewTemp();
  return rbp;
}

TEMP::Temp *F_FP(void)
{
  if (!rbp)
    rbp = TEMP::Temp::NewTemp();
  return rbp;
}
TEMP::Temp *F_SP(void)
{
  if (!rsp)
    rsp = TEMP::Temp::NewTemp();
  return rsp;
}
TEMP::Temp *F_ZERO(void)
{
}
TEMP::Temp *F_RA(void)
{
}

TEMP::Temp *F_RAX(void)
{
  if (!rax)
    rax = TEMP::Temp::NewTemp();
  return rax;
}
TEMP::Temp *F_RV(void) //return value of the callee
{
  if (!rax)
    rax = TEMP::Temp::NewTemp();
  return rax;
}

TEMP::Temp *F_RDI()
{
  if (!rdi)
    rdi = TEMP::Temp::NewTemp();
  return rdi;
}
TEMP::Temp *F_RSI()
{
  if (!rsi)
    rsi = TEMP::Temp::NewTemp();
  return rsi;
}
TEMP::Temp *F_RDX()
{
  if (!rdx)
    rdx = TEMP::Temp::NewTemp();
  return rdx;
}
TEMP::Temp *F_RCX()
{
  if (!rcx)
    rcx = TEMP::Temp::NewTemp();
  return rcx;
}
TEMP::Temp *F_R8()
{
  if (!r8)
    r8 = TEMP::Temp::NewTemp();
  return r8;
}
TEMP::Temp *F_R9()
{
  if (!r9)
    r9 = TEMP::Temp::NewTemp();
  return r9;
}

TEMP::Temp *F_R10()
{
  if (!r10)
    r10 = TEMP::Temp::NewTemp();
  return r10;
}

TEMP::Temp *F_R11()
{
  if (!r11)
    r11 = TEMP::Temp::NewTemp();
  return r11;
}

TEMP::Temp *F_R12()
{
  if (!r12)
    r12 = TEMP::Temp::NewTemp();
  return r12;
}

TEMP::Temp *F_R13()
{
  if (!r13)
    r13 = TEMP::Temp::NewTemp();
  return r13;
}

TEMP::Temp *F_R14()
{
  if (!r14)
    r14 = TEMP::Temp::NewTemp();
  return r14;
}

TEMP::Temp *F_R15()
{
  if (!r15)
    r15 = TEMP::Temp::NewTemp();
  return r15;
}

TEMP::Temp *F_RBX()
{
  if (!rbx)
    rbx = TEMP::Temp::NewTemp();
  return rbx;
}
class X64Frame : public Frame
{
public:
  // TODO: Put your codes here (lab6).
  X64Frame(TEMP::Label *label,
           F::AccessList *formals,
           F::AccessList *locals,
           T::StmList *view_shift, int s_offset)
      : Frame(label, formals, locals, view_shift, s_offset) {}
};

class InFrameAccess : public F::Access
{
public:
  int offset;
  T::Exp *ToExp(T::Exp *framePtr) const override;

  InFrameAccess(int offset) : Access(INFRAME), offset(offset) {}
};

T::Exp *InFrameAccess ::ToExp(T::Exp *framePtr) const
{
  return new T::MemExp(new T::BinopExp(T::PLUS_OP, framePtr, new T::ConstExp(this->offset)));
}

class InRegAccess : public F::Access
{
public:
  TEMP::Temp *reg;
  T::Exp *ToExp(T::Exp *framePtr) const override;
  InRegAccess(TEMP::Temp *reg) : Access(INREG), reg(reg) {}
};

Frame *F_newFrame(TEMP::Label *name, U::BoolList *escapes)
{
  F::AccessList *formals = new AccessList(nullptr, nullptr);
  T::StmList *view_shift = new T::StmList(nullptr, nullptr);

  T::StmList *v_tail = view_shift;
  F::AccessList *f_tail = formals;
  bool escape;
  TEMP::Temp *temp = TEMP::Temp::NewTemp();

  int formal_off = wordsize; // The seventh arg was located at 8(%rbp)
  /*If the formal is escape, then allocate it on the frame.
	  Else,allocate it on the temp.*/
  X64Frame *newframe = new X64Frame(name, NULL, NULL, NULL, 0);

  int num = 1; //the num of formals
  for (; escapes; escapes = escapes->tail, num++)
  {
    escape = escapes->head;
    if (escape)
    {
      switch (num)
      {
      case 1:
        v_tail->tail = new T::StmList(new T::MoveStm(
                                          new T::MemExp(
                                              new T::BinopExp(
                                                  T::PLUS_OP, new T::TempExp(F::F_FP()), new T::ConstExp(newframe->s_offset))),
                                          new T::TempExp(F_RDI())),
                                      NULL);
        newframe->s_offset -= wordsize;
        f_tail->tail = new AccessList(new F::InFrameAccess(newframe->s_offset), NULL);
        f_tail = f_tail->tail;
        v_tail = v_tail->tail;
        break;
      case 2:
        v_tail->tail = new T::StmList(new T::MoveStm(
                                          new T::MemExp(
                                              new T::BinopExp(
                                                  T::PLUS_OP, new T::TempExp(F::F_FP()), new T::ConstExp(newframe->s_offset))),
                                          new T::TempExp(F_RSI())),
                                      NULL);
        newframe->s_offset -= wordsize;
        f_tail->tail = new AccessList(new F::InFrameAccess(newframe->s_offset), NULL);
        f_tail = f_tail->tail;
        v_tail = v_tail->tail;
        break;
      case 3:
        v_tail->tail = new T::StmList(new T::MoveStm(
                                          new T::MemExp(
                                              new T::BinopExp(
                                                  T::PLUS_OP, new T::TempExp(F::F_FP()), new T::ConstExp(newframe->s_offset))),
                                          new T::TempExp(F_RDX())),
                                      NULL);
        newframe->s_offset -= wordsize;
        f_tail->tail = new AccessList(new F::InFrameAccess(newframe->s_offset), NULL);
        f_tail = f_tail->tail;
        v_tail = v_tail->tail;
        break;
      case 4:
        v_tail->tail = new T::StmList(new T::MoveStm(
                                          new T::MemExp(
                                              new T::BinopExp(
                                                  T::PLUS_OP, new T::TempExp(F::F_FP()), new T::ConstExp(newframe->s_offset))),
                                          new T::TempExp(F_RCX())),
                                      NULL);
        newframe->s_offset -= wordsize;
        f_tail->tail = new AccessList(new F::InFrameAccess(newframe->s_offset), NULL);
        f_tail = f_tail->tail;
        v_tail = v_tail->tail;
        break;
      case 5:
        v_tail->tail = new T::StmList(new T::MoveStm(
                                          new T::MemExp(
                                              new T::BinopExp(
                                                  T::PLUS_OP, new T::TempExp(F::F_FP()), new T::ConstExp(newframe->s_offset))),
                                          new T::TempExp(F_R8())),
                                      NULL);
        newframe->s_offset -= wordsize;
        f_tail->tail = new AccessList(new F::InFrameAccess(newframe->s_offset), NULL);
        f_tail = f_tail->tail;
        v_tail = v_tail->tail;
        break;
      case 6:
        v_tail->tail = new T::StmList(new T::MoveStm(
                                          new T::MemExp(
                                              new T::BinopExp(
                                                  T::PLUS_OP, new T::TempExp(F::F_FP()), new T::ConstExp(newframe->s_offset))),
                                          new T::TempExp(F_R9())),
                                      NULL);
        newframe->s_offset -= wordsize;
        f_tail->tail = new AccessList(new F::InFrameAccess(newframe->s_offset), NULL);
        f_tail = f_tail->tail;
        v_tail = v_tail->tail;
        break;
      default:
      {
        f_tail->tail = new AccessList(new F::InFrameAccess(formal_off), NULL); //sequence of formals here is reversed.
        f_tail = f_tail->tail;
        formal_off += wordsize;
      }
      }
    }
    else
    {
      //allocate it on the temp
    }
  }

  newframe = new X64Frame(name, formals->tail, NULL, view_shift->tail, newframe->s_offset);
  return newframe;
}

T::Exp *InRegAccess ::ToExp(T::Exp *framePtr) const
{
  return new T::TempExp(this->reg);
}

T::Stm *F_procEntryExit1(Frame *frame, T::Stm *stm)
{
  T::StmList *iter = frame->view_shift;
  T::SeqStm *res;
  while (iter && iter->head)
  {
    res = new T::SeqStm(iter->head, res);
    iter = iter->tail;
  }
  return res;
}

AS::Proc *F_procEntryExit3(Frame *frame, AS::InstrList *inst)
{
  std::string prolog;
  std::string funcname = frame->label->Name() + ":\n";
  prolog = funcname + "# exit3\n"
                      "push %rbp\n"
                      "movq %rsp, %rbp\n"
                      "subq $" +
           std::to_string(
               abs(frame->s_offset)) +
           ", %rsp\n";
  std::string epilog = "leave\nret\n";
  return new AS::Proc(prolog, inst, epilog);
}

AS::InstrList *F_procEntryExit2(AS::InstrList *body)
{

  static TEMP::TempList *returnSink = NULL;
  if (!returnSink)
    returnSink = new TEMP::TempList(rsp, new TEMP::TempList(F::F_FP(), NULL));
  return AS::InstrList::Splice(body, new AS::InstrList(new AS::OperInstr("#exit2", NULL, returnSink, NULL), NULL));
}

F::Access *F_allocLocal(Frame *frame, bool escape)
{
  F::Access *local;
  if (escape)
  {
    local = new F::InFrameAccess(frame->s_offset);
    frame->s_offset -= wordsize;
  }
  else
  {
    local = new F::InRegAccess(TEMP::Temp::NewTemp());
  }
  return local;
}

T::CallExp *F_externalCall(std::string s, T::ExpList *args)
{
  return new T::CallExp(new T::NameExp(TEMP::NamedLabel(s)), args);
}
} // namespace F