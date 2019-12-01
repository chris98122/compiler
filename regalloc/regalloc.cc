#include "tiger/regalloc/regalloc.h"

namespace RA
{

static void RewriteProgram(F::Frame *f, AS::InstrList *pil);

static void AssignColors();

static void SelectSpill();

static void MakeWorklist();
static void Build();

static void Simplify();
static TEMP::Map *AssignRegisters(LIVE::LiveGraph g);

Result RegAlloc(F::Frame *f, AS::InstrList *il)
{
  // TODO: Put your codes here (lab6).
  Result *ret = new Result(nullptr, nullptr);

  bool done = false;
  do
  {
    done = true;
    /*Liveness Analysis*/
    G::Graph<AS::Instr> *fg = FG::AssemFlowGraph(il, f);
    live = LIVE::Liveness(fg);

    Build();

    MakeWorklist();

    while (simplifyWorklist || spillWorklist)
    {

      if (simplifyWorklist)
      {
        Simplify();
      }
      if (spillWorklist)
      {
        SelectSpill();
      }
    }

    AssignColors();

    if (spilledNodes)
    {
      RewriteProgram(f, il);
      done = false;
    }

  } while (!done);

  ret->coloring = AssignRegisters(live);
  ret->il = il;

  return *ret;
}

static TEMP::Map *AssignRegisters(LIVE::LiveGraph g)
{
  TEMP::Map *res = TEMP::Map::Empty();
  G::NodeList<TEMP::Temp> *nodes = (g.graph)->Nodes();

  res->Enter(F::F_SP(), new std::string("%rsp"));
  for (; nodes; nodes = nodes->tail)
  {
    int *color = colorTab->Look(nodes->head);
    res->Enter(nodes->head->NodeInfo(), hard_reg[*color]);
  }
  return res;
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
void RewriteProgram(F::Frame *f, AS::InstrList *pil)
{
  std::string fs = TEMP::LabelString(f->label) + "_framesize";

  notSpillTemps = NULL;
  AS::InstrList *il = new AS::InstrList(NULL, NULL);
  *il = *pil;
  AS::InstrList *instr, //every instruction in il
      *last,            // last handled instruction
      *next,            //next instruction
      *new_instr;       //new_instruction after spilling.
  int off;
  int count = 0;

  while (spilledNodes)
  {
    G::Node<TEMP::Temp> *cur = spilledNodes->head;
    spilledNodes = spilledNodes->tail;
    TEMP::Temp *spilledtemp = cur->NodeInfo();
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
      if (use && intemp(use, spilledtemp))
      {

        t = TEMP::Temp::NewTemp();

        notSpillTemps = new TEMP::TempList(t, notSpillTemps);

        printf("-------====replace temp :%d=====-----\n", t->Int());

        //Replace spilledtemp by t.
        *use = *replaceTempList(use, spilledtemp, t);

        assert(!intemp(use, spilledtemp));

        std::string assem;
        std::stringstream ioss;
        ioss << "# Spill load\nmovq (" + fs + "-0x" << std::hex << -off << ")(%rsp),`d0\n";
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

      }
      last = instr;

      if (def && intemp(def, spilledtemp))
      {
        if (!t)
        {
          t = TEMP::Temp::NewTemp();

          notSpillTemps = new TEMP::TempList(t, notSpillTemps);
          printf("-------====replace temp :%d=====-----\n", t->Int());
        }

        *def = *replaceTempList(def, spilledtemp, t);

        assert(!intemp(def, spilledtemp));

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

static void MakeWorklist()
{
  G::NodeList<TEMP::Temp> *nodes = (live.graph)->Nodes();

  for (; nodes; nodes = nodes->tail)
  {
    G::Node<TEMP::Temp> *node = nodes->head;
    int degree = node->Degree();
    int *color = colorTab->Look(nodes->head);
    //Reject precolored register
    if (*color != 0)
    {
      continue;
    }
    if (degree >= K)
    {
      spillWorklist = new G::NodeList<TEMP::Temp>(node, spillWorklist);
    }
    else
    {
      simplifyWorklist = new G::NodeList<TEMP::Temp>(node, simplifyWorklist);
    }
  }
}
static void Build()
{
  simplifyWorklist = NULL;
  //degree >= K
  spillWorklist = NULL;

  spilledNodes = NULL;

  selectStack = NULL;
  colorTab = new G::Table<TEMP::Temp, int>();

  G::NodeList<TEMP::Temp> *nodes = (live.graph)->Nodes();
  for (; nodes; nodes = nodes->tail)
  {
    TEMP::Temp *temp = nodes->head->NodeInfo();
    int *color;
    if (temp == F::F_RAX())
      *color = 1;
    else if (temp == F::F_RBX())
      *color = 2;
    else if (temp == F::F_RCX())
      *color = 3;
    else if (temp == F::F_RDX())
      *color = 4;
    else if (temp == F::F_RSI())
      *color = 5;
    else if (temp == F::F_RDI())
      *color = 6;
    else if (temp == F::F_RBP())
      *color = 7;
    else if (temp == F::F_R8())
      *color = 8;
    else if (temp == F::F_R9())
      *color = 9;
    else if (temp == F::F_R10())
      *color = 10;
    else if (temp == F::F_R11())
      *color = 11;
    else if (temp == F::F_R12())
      *color = 12;
    else if (temp == F::F_R13())
      *color = 13;
    else if (temp == F::F_R14())
      *color = 14;
    else if (temp == F::F_R15())
      *color = 15;
    else
      *color = 0; //Temp register
    colorTab->Enter(nodes->head, color);
  }
}

static void AssignColors()
{
  bool okColors[K + 1];
  spilledNodes = NULL;
  while (selectStack)
  {
    okColors[0] = false;
    for (int i = 1; i < K + 1; i++)
    {
      okColors[i] = true;
    }

    G::Node<TEMP::Temp> *n = selectStack->head;
    selectStack = selectStack->tail;

    for (G::NodeList<TEMP::Temp> *succs = n->Succ(); succs; succs = succs->tail)
    {
      int *color = colorTab->Look(succs->head);
      okColors[*color] = false;
    }

    int i;
    bool realSpill = true;
    for (i = 1; i < K + 1; i++)
    {
      if (okColors[i])
      {
        realSpill = false;
        break;
      }
    }
    if (realSpill)
    {
      spilledNodes = new G::NodeList<TEMP::Temp>(n, spilledNodes);
    }
    else
    {
      int *color = colorTab->Look(n);
      *color = i;
    }
  }
}

static void SelectSpill()
{
}

static void Simplify()
{

  G::Node<TEMP::Temp> *node = simplifyWorklist->head;
  simplifyWorklist = simplifyWorklist->tail;
  //push n to stack
  selectStack = new G::NodeList<TEMP::Temp>(node, selectStack);
  // G_nodeList adj = Adjacent(node);
  // for (; adj; adj = adj->tail)
  // {
  //   DecrementDegree(adj->head);
  // }
}
} // namespace RA