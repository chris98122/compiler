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

TEMP::Temp *F_RDI()
{
  if (!rdi)
    rdi = TEMP::Temp::NewTemp();
  return rdi;
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
  int num = 1;               //Marks the sequence of the formals.
                             /*If the formal is escape, then allocate it on the frame.
	  Else,allocate it on the temp.*/
  X64Frame *newframe = new X64Frame(name, NULL, NULL, NULL, 0);

  for (; escapes; escapes = escapes->tail, num++)
  {
    escape = escapes->head;
    if (escape)
    {
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