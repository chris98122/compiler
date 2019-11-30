#include "tiger/regalloc/regalloc.h"

namespace RA
{

void addreg(TEMP::Map *m)
{

  std::string *addr = new std::string("%rbx");
  m->Enter(F::F_RBX(), addr);

  addr = new std::string("%rbp");
  m->Enter(F::F_FP(), addr);
  m->Enter(F::F_RBP(), addr);

  addr = new std::string("%rsp");
  m->Enter(F::F_SP(), addr);
  addr = new std::string("%rax");
  m->Enter(F::F_RAX(), addr);

  addr = new std::string("%rsi");
  m->Enter(F::F_RSI(), addr);
  addr = new std::string("%rcx");
  m->Enter(F::F_RCX(), addr);

  addr = new std::string("%rdi");
  m->Enter(F::F_RDI(), addr);
  addr = new std::string("%r8");
  m->Enter(F::F_R8(), addr);
  addr = new std::string("%r9");
  m->Enter(F::F_R9(), addr);
  addr = new std::string("%r10");
  m->Enter(F::F_R10(), addr);
  addr = new std::string("%r11");
  m->Enter(F::F_R11(), addr);
  addr = new std::string("%r12");
  m->Enter(F::F_R12(), addr);
  addr = new std::string("%r13");
  m->Enter(F::F_R13(), addr);
  addr = new std::string("%r14");
  m->Enter(F::F_R14(), addr);
  addr = new std::string("%r15");
  m->Enter(F::F_R15(), addr);
}

static void add_temp(AS::InstrList *instr)
{

  while (instr)
  {

    TEMP::TempList *def = nullptr, *use = nullptr;
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
    }
    while (def)
    {
      spilledNodes.insert(def->head);
      def = def->tail;
    }
    while (use)
    {
      spilledNodes.insert(use->head);
      use =  use ->tail;
    }
    instr = instr->tail;
  }
}

static void RewriteProgram(F::Frame *f, AS::InstrList *pil, TEMP::Map *map);
Result RegAlloc(F::Frame *f, AS::InstrList *il)
{
  // TODO: Put your codes here (lab6).
  TEMP::Map *map = TEMP::Map::Empty();
  addreg(map);
  add_temp(il);

  RewriteProgram(f, il, map);

  map->Enter(F::F_SP(), new std::string("%rsp"));
  return Result(map, il);
}

static TEMP::TempList *replaceTempList(TEMP::TempList *instr, TEMP::Temp *old, TEMP::Temp *new_t)
{
  if (instr)
  {
    if (instr->head == old)
    {
      return new TEMP::TempList(new_t, replaceTempList(instr->tail, old, new_t));
    }
    else
    {
      return new TEMP::TempList(instr->head, replaceTempList(instr->tail, old, new_t));
    }
  }
  else
  {
    return nullptr;
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
  instr = il;
  last = NULL;
  int count = 0;

  while (!spilledNodes.empty())
  {
    TEMP::Temp *spilltemp = *(spilledNodes.begin());
    spilledNodes.erase(spilledNodes.begin());
    off = f->s_offset;
    f->s_offset -= 8;
    count++;
    while (instr)
    {
      TEMP::Temp *t = NULL;
      next = instr->tail;
      TEMP::TempList *def = nullptr, *use = nullptr;
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
      if (use)
      {
        t = TEMP::Temp::NewTemp();
        switch (count)
        {
        case 1:
          map->Enter(t, new std::string("%r15"));
          break;
        case 2:
          map->Enter(t, new std::string("%r14"));
          break;
        case 3:
          map->Enter(t, new std::string("%r13"));
          break;

        case 4:
          map->Enter(t, new std::string("%r12"));
          break;
        default:
          assert(0);
        }

        //Replace spilledtemp by t.
        *use = *replaceTempList(use, spilltemp, t);

        std::string assem = "# Spill load\nmovq (" + fs + "-" + std::to_string(-off) + ")(%rsp),`d0\n";

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

      if (def)
      {
        if (!t)
        {
          t = TEMP::Temp::NewTemp();

          switch (count)
          {
          case 1:
            map->Enter(t, new std::string("%r15"));
            break;
          case 2:
            map->Enter(t, new std::string("%r14"));
            break;
          case 3:
            map->Enter(t, new std::string("%r13"));
            break;

          case 4:
            map->Enter(t, new std::string("%r12"));
            break;
          default:
            assert(0);
          }
        }

        *def = *replaceTempList(def, spilltemp, t);
        std::string assem = "# Spill store\nmovq `s0, (" + fs + "-" + std::to_string(-off) + ")(%rsp) \n";
        //Add the new instruction betfore the old one.
        AS::OperInstr *os_instr = new AS::OperInstr(assem, NULL, new TEMP::TempList(t, nullptr), new AS::Targets(NULL));
        instr->tail = new AS::InstrList(os_instr, next);

        last = instr->tail;
      }

      instr = next;
    }
    f->s_offset += 8;
  }
  pil = il;
}
} // namespace RA
