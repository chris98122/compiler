#include "tiger/frame/frame.h"

#include <string>

namespace F {

class X64Frame : public Frame {
  // TODO: Put your codes here (lab6).
};

class InFrameAccess : public Access {
 public:
  int offset;

  InFrameAccess(int offset) : Access(INFRAME), offset(offset) {}
};

class InRegAccess : public Access {
 public:
  TEMP::Temp* reg;

  InRegAccess(TEMP::Temp* reg) : Access(INREG), reg(reg) {}
};

T::Stm *F_procEntryExit1(Frame *frame, T::Stm *stm) 
{
  return nullptr;
}

AS::Proc *F_procEntryExit3(Frame *frame,AS::InstrList* inst ) 
{
  return nullptr;
}


AS::InstrList *F_procEntryExit2(AS::InstrList* body)
{

  return nullptr;
} 
TEMP::Temp *F_FP(void)
{

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

}
}  // namespace F