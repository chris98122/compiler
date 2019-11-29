#include "tiger/regalloc/regalloc.h"

namespace RA
{

bool isreg(TEMP::Temp *t, TEMP::Map *m)
{

  if (t == F::F_RBX())
  {
    std::string *addr = new std::string("%rbx");
    m->Enter(t, addr);
    return true;
  }
  if (t == F::F_FP() || t == F::F_RBP())
  {
    std::string *addr = new std::string("%rbp");
    m->Enter(t, addr);
    return true;
  }
  if (t == F::F_SP())
  {
    std::string *addr = new std::string("%rsp");
    m->Enter(t, addr);
    return true;
  }
  if (t == F::F_RAX())
  {
    std::string *addr = new std::string("%rax");
    m->Enter(t, addr);
    return true;
  }
  if (t == F::F_RSI())
  {
    std::string *addr = new std::string("%rsi");
    m->Enter(t, addr);
    return true;
  }
  if (t == F::F_RCX())
  {
    std::string *addr = new std::string("%rcx");
    m->Enter(t, addr);
    return true;
  }
  if (t == F::F_RDI())
  {
    std::string *addr = new std::string("%rdi");
    m->Enter(t, addr);
    return true;
  }
  if (t == F::F_R8())
  {
    std::string *addr = new std::string("%r8");
    m->Enter(t, addr);
    return true;
  }
  if (t == F::F_R9())
  {
    std::string *addr = new std::string("%r9");
    m->Enter(t, addr);
    return true;
  }
  if (t == F::F_R10())
  {
    std::string *addr = new std::string("%r10");
    m->Enter(t, addr);
    return true;
  }
  if (t == F::F_R11())
  {
    std::string *addr = new std::string("%r11");
    m->Enter(t, addr);
    return true;
  }
  if (t == F::F_R12())
  {
    std::string *addr = new std::string("%r12");
    m->Enter(t, addr);
    return true;
  }
  if (t == F::F_R13())
  {
    std::string *addr = new std::string("%r13");
    m->Enter(t, addr);
    return true;
  }
  if (t == F::F_R14())
  {
    std::string *addr = new std::string("%r14");
    m->Enter(t, addr);
    return true;
  }
  if (t == F::F_R15())
  {
    std::string *addr = new std::string("%r15");
    m->Enter(t, addr);
    return true;
  }

  return false;
}

static void RewriteProgram(F::Frame *f, AS::InstrList *pil, TEMP::Map *map);
Result RegAlloc(F::Frame *f, AS::InstrList *il)
{
  // TODO: Put your codes here (lab6).
  TEMP::Map *map = TEMP::Map::Empty();
 
  RewriteProgram(f, il, map);

  map->Enter(F::F_SP(),new std::string("%rsp"));
  return Result(map, il);
}

static TEMP::TempList *replaceTempList(TEMP::TempList *instr, TEMP::Map *map, TEMP::Temp *new_t)
{
  if (instr)
  {
    if (!isreg(instr->head, map))
    {
      return new TEMP::TempList(new_t, replaceTempList(instr->tail, map, new_t));
    }
    else
    {
      return new TEMP::TempList(instr->head, replaceTempList(instr->tail, map, new_t));
    }
  }
}
void RewriteProgram(F::Frame *f, AS::InstrList *pil, TEMP::Map *map)
{
  std::string fs = TEMP::LabelString(f->label) + "_framesize";

  AS::InstrList *il = pil,
                *instr, //every instruction in il
      *last,            // last handled instruction
      *next,            //next instruction
      *new_instr;       //new_instruction after spilling.
  int off;
  f->s_offset -= 8;
  instr = il;
  last = NULL;

  while (instr)
  {
    TEMP::Temp *t = NULL;
    next = instr->tail;
    TEMP::TempList *def = nullptr, *use= nullptr;
    switch (instr->head->kind)
    {
    case AS::Instr::MOVE:
      def = ((AS::MoveInstr *)instr->head)->dst;
      use = ((AS::MoveInstr *)instr->head)->src;

      break;
    case AS::Instr::OPER:
      def = ((AS::OperInstr *)instr->head)->dst;
      use = ((AS::OperInstr *)instr->head)->src;
      break;
    default:
      def = NULL;
      use = NULL;
    }
    if (use )
    {
      t = TEMP::Temp::NewTemp();
      map->Enter(t, new std::string("%r15"));

      //Replace spilledtemp by t.
      *use = *replaceTempList(use, map, t);
 
      std::string assem = "# Spill load\nmovq (" + fs+"-"+std::to_string(-f->s_offset) + ")(%rsp),`d0\n";

      //Add the new instruction betfore the old one.
      AS::OperInstr *os_instr = new AS::OperInstr(assem, new TEMP::TempList(t, nullptr), NULL, new AS::Targets(NULL));
      new_instr = new AS::InstrList(os_instr, instr);

      if (last)
      {
        last->tail = new_instr;
      }
      else //instr is the first instruction of il.
      {
        il = new_instr;
      }
    }
    last = instr;

    if (def )
    {
      if (!t)
      {
        t = TEMP::Temp::NewTemp();

        map->Enter(t, new std::string("%r15"));
      }

      *def = *replaceTempList(def, map, t);
      std::string assem = "# Spill store\nmovq `s0, (" + fs+"-"+std::to_string(-f->s_offset) + ")(%rsp) \n";
      //Add the new instruction betfore the old one.
      AS::OperInstr *os_instr = new AS::OperInstr(assem, NULL, new TEMP::TempList(t, nullptr), new AS::Targets(NULL));
      instr->tail = new AS::InstrList(os_instr, next);

      last = instr->tail;
    }
    instr = next;

  }
 
  pil = il;
}
} // namespace RA
