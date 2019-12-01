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

bool isreg(TEMP::Temp *t)
{

  if (t == F::F_RBX())
  {
    std::string *addr = new std::string("%rbx");

    return true;
  }
  if (t == F::F_FP() || t == F::F_RBP())
  {
    std::string *addr = new std::string("%rbp");

    return true;
  }
  if (t == F::F_SP())
  {
    std::string *addr = new std::string("%rsp");

    return true;
  }
  if (t == F::F_RAX())
  {
    std::string *addr = new std::string("%rax");

    return true;
  }
  if (t == F::F_RSI())
  {
    std::string *addr = new std::string("%rsi");

    return true;
  }
  if (t == F::F_RCX())
  {
    std::string *addr = new std::string("%rcx");

    return true;
  }
  if (t == F::F_RDI())
  {
    std::string *addr = new std::string("%rdi");

    return true;
  }
  if (t == F::F_R8())
  {
    std::string *addr = new std::string("%r8");

    return true;
  }
  if (t == F::F_R9())
  {
    std::string *addr = new std::string("%r9");

    return true;
  }
  if (t == F::F_R10())
  {
    std::string *addr = new std::string("%r10");

    return true;
  }
  if (t == F::F_R11())
  {
    std::string *addr = new std::string("%r11");

    return true;
  }
  if (t == F::F_R12())
  {
    std::string *addr = new std::string("%r12");

    return true;
  }
  if (t == F::F_R13())
  {
    std::string *addr = new std::string("%r13");

    return true;
  }
  if (t == F::F_R14())
  {
    std::string *addr = new std::string("%r14");

    return true;
  }
  if (t == F::F_R15())
  {
    std::string *addr = new std::string("%r15");

    return true;
  }

  return false;
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
      if (!isreg(def->head))
      {
        spilledNodes.insert(def->head);
      }
      def = def->tail;
    }
    while (use)
    {

      if (!isreg(use->head))
      {
        spilledNodes.insert(use->head);
      }
      use = use->tail;
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

bool intemp(TEMP::TempList *list, TEMP::Temp *temp)
{
  for (; list; list = list->tail)
  {
    if (list->head == temp)
      return true;
  }
  return false;
}
void RewriteProgram(F::Frame *f, AS::InstrList *pil, TEMP::Map *map)
{
  std::string fs = TEMP::LabelString(f->label) + "_framesize";

  AS::InstrList *il = new AS::InstrList(NULL, NULL);
  *il = *pil;
  AS::InstrList *instr, //every instruction in il
      *last,            // last handled instruction
      *next,            //next instruction
      *new_instr;       //new_instruction after spilling.
  int off;
  int count = 0;

  while (!spilledNodes.empty())
  {
    printf("-------====tempcount:%d=====-----\n", count);
    TEMP::Temp *spilltemp = *(spilledNodes.begin());

    printf("-------====spilltemp :%d=====-----\n", spilltemp->Int());
    spilledNodes.erase(spilledNodes.begin());
    off = f->s_offset;
    f->s_offset -= 8;
    instr = il;
    last = NULL;
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
      if (use && intemp(use, spilltemp))
      {

        t = TEMP::Temp::NewTemp();
        if (count % 2)
          map->Enter(t, new std::string("%r15"));
        else
        { 
          map->Enter(t, new std::string("%r13")); 
        }

        printf("-------====replace temp :%d=====-----\n", t->Int());

        //Replace spilledtemp by t.
        *use = *replaceTempList(use, spilltemp, t);

        assert(!intemp(use, spilltemp));

        std::string assem;
        std::stringstream ioss;
        ioss << "#  use  Spill load\nmovq (" + fs + "-0x" << std::hex << -off << ")(%rsp),`d0\n";

        assem = ioss.str();

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

        std::string assem_store;
        std::stringstream ioss_store;
        ioss_store << "# use Spill store\nmovq `s0, (" + fs + "-0x" << std::hex << -off << ")(%rsp) \n";

        assem_store = ioss_store.str();

        //Add the new instruction betfore the old one.
        AS::OperInstr *os_instr_store = new AS::OperInstr(assem_store, NULL, new TEMP::TempList(t, nullptr), new AS::Targets(NULL));
        instr->tail = new AS::InstrList(os_instr_store, next);

        last = instr->tail;
      }
      else
      {

        last = instr;
      }

      if (def && intemp(def, spilltemp))
      {
        if (!t)
        {
          t = TEMP::Temp::NewTemp();
          map->Enter(t, new std::string("%r14"));

          printf("-------====replace temp :%d=====-----\n", t->Int());
        }

        *def = *replaceTempList(def, spilltemp, t);

        assert(!intemp(def, spilltemp));

        std::string assem;
        std::stringstream ioss;
        ioss << "# Spill store\nmovq `s0, (" + fs + "-0x" << std::hex << -off << ")(%rsp) \n";

        assem = ioss.str();

        //Add the new instruction betfore the old one.
        AS::OperInstr *os_instr = new AS::OperInstr(assem, NULL, new TEMP::TempList(t, nullptr), new AS::Targets(NULL));
        instr->tail = new AS::InstrList(os_instr, next);

        last = instr->tail;
      }
      instr = next;
    }
  }

  f->s_offset += 8;
  *pil = *il;
}

} // namespace RA
