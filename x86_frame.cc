#include "tiger/frame/frame.h"

#include <string>

namespace F
{

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

Frame *F_newFrame(TEMP::Label *name, U::BoolList *escape)
{
  X64Frame *x64frame = new X64Frame(name, nullptr, nullptr, nullptr, 0);
  return x64frame;
}

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

T::Exp *InRegAccess ::ToExp(T::Exp *framePtr) const
{
  return new T::TempExp(this->reg);
}

T::Stm *F_procEntryExit1(Frame *frame, T::Stm *stm)
{
  return nullptr;
}

AS::Proc *F_procEntryExit3(Frame *frame, AS::InstrList *inst)
{
  return nullptr;
}

AS::InstrList *F_procEntryExit2(AS::InstrList *body)
{

  return nullptr;
}

static TEMP::Temp *rbp = NULL;
static TEMP::Temp *rax = NULL;
static TEMP::Temp *rdi = NULL;
static TEMP::Temp *rsi = NULL;
static TEMP::Temp *rdx = NULL;
static TEMP::Temp *rcx = NULL;
static TEMP::Temp *r8 = NULL;
static TEMP::Temp *r9 = NULL;

TEMP::Temp *F_FP(void)
{
  if (!rbp)
    rbp = TEMP::Temp::NewTemp();
  return rbp;
}
TEMP::Temp *F_SP(void)
{
}
TEMP::Temp *F_ZERO(void)
{
}
TEMP::Temp *F_RA(void)
{
}
TEMP::Temp *F_RV(void)
{
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
} // namespace F