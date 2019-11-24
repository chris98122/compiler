#ifndef TIGER_FRAME_FRAME_H_
#define TIGER_FRAME_FRAME_H_

#include <string>

#include "tiger/translate/tree.h"
#include "tiger/util/util.h"

#include "tiger/codegen/assem.h"

static const int wordsize = 8;

namespace F
{
class AccessList;
class Frame
{
public:
  // Base class
  TEMP::Label *label;
  F::AccessList *formals;
  F::AccessList *locals;
  T::StmList *view_shift;
  int s_offset; //Which is commonly a minus number.
  Frame(TEMP::Label *label,
        F::AccessList *formals,
        F::AccessList *locals,
        T::StmList *view_shift, int s_offset)
      : label(label), formals(formals), locals(locals), view_shift(view_shift), s_offset(s_offset) {}
};

Frame *F_newFrame(TEMP::Label *name, U::BoolList *escape);

TEMP::Temp *F_FP(void);
TEMP::Temp *F_SP(void);
TEMP::Temp *F_ZERO(void);
TEMP::Temp *F_RA(void);
TEMP::Temp *F_RV(void);

class Access
{
public:
  enum Kind
  {
    INFRAME,
    INREG
  };

  Kind kind;

  Access(Kind kind) : kind(kind) {}

  // Hints: You may add interface like
  virtual T::Exp *ToExp(T::Exp *framePtr) const = 0;
};

class AccessList
{
public:
  Access *head;
  AccessList *tail;

  AccessList(Access *head, AccessList *tail) : head(head), tail(tail) {}
};

/*
 * Fragments
 */

class Frag
{
public:
  enum Kind
  {
    STRING,
    PROC
  };

  Kind kind;

  Frag(Kind kind) : kind(kind) {}
};

class StringFrag : public Frag
{
public:
  TEMP::Label *label;
  std::string str;

  StringFrag(TEMP::Label *label, std::string str)
      : Frag(STRING), label(label), str(str) {}
};

class ProcFrag : public Frag
{
public:
  T::Stm *body;
  Frame *frame;

  ProcFrag(T::Stm *body, Frame *frame) : Frag(PROC), body(body), frame(frame) {}
};

class FragList
{
public:
  Frag *head;
  FragList *tail;

  FragList(Frag *head, FragList *tail) : head(head), tail(tail) {}
};

T::Stm *F_procEntryExit1(Frame *frame, T::Stm *stm);

AS::Proc *F_procEntryExit3(Frame *frame, AS::InstrList *inst);
AS::InstrList *F_procEntryExit2(AS::InstrList *body);
F::Access *F_allocLocal(Frame *frame, bool escape);
} // namespace F

#endif