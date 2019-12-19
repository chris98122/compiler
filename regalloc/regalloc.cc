#include "tiger/regalloc/regalloc.h"

namespace RA
{
static bool precolor(G::Node<TEMP::Temp> *n)
{
  return *(colorTab->Look(n));
}

static int degree(G::Node<TEMP::Temp> *n)
{
  return *(degreeTab->Look(n));
}
static LIVE::MoveList *movelist(G::Node<TEMP::Temp> *n)
{
  return moveListTab->Look(n);
}

//Use George to check if v can be combined by u
static bool OK(G::Node<TEMP::Temp> *v, G::Node<TEMP::Temp> *u)
{
  bool ok = true;
  for (G::NodeList<TEMP::Temp> *adj = v->Adj(); adj; adj = adj->tail)
  {
    G::Node<TEMP::Temp> *t = adj->head;
    if (!(t->Degree() < K || precolor(t) || u->Adj()->InNodeList(t)))
    {
      ok = false;
      break;
    }
  }
  return ok;
}
/* Use Briggs. Check if the high degree nodes in the adjacent nodes 
of the combined node(uv) are less than K */
static bool Conservative(G::NodeList<TEMP::Temp> *nodes)
{
  int count = 0;
  for (; nodes; nodes = nodes->tail)
  {
    if (nodes->head->Degree() >= K)
    {
      count = count + 1;
    }
  }
  return count < K;
}

static LIVE::MoveList *NodeMoves(G::Node<TEMP::Temp> *n)
{

  // If n is combined now(Alias(n) != n), it doesn't matter.Because n's movelist was
  // modified when combining.
  LIVE::MoveList *movelist = moveListTab->Look(n);
  return LIVE::IntersectMoveList(movelist, LIVE::UnionMoveList(activeMoves, worklistMoves));
}

static bool MoveRelated(G::Node<TEMP::Temp> *n)
{
  return NodeMoves(n) != NULL;
}

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

    while (simplifyWorklist || worklistMoves || freezeWorklist || spillWorklist)
    {

      if (simplifyWorklist)
      {
        Simplify();
      }
      else if (worklistMoves)
      {
        Coalesce();
      }
      else if (freezeWorklist)
      {
        Freeze();
      }
      else if (spillWorklist)
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

static TEMP::Map *AssignRegisters(LIVE::LiveGraph *g)
{
  TEMP::Map *res = TEMP::Map::Empty();
  G::NodeList<TEMP::Temp> *nodes = (g->graph)->Nodes();

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
  G::NodeList<TEMP::Temp> *nodes = (live->graph)->Nodes();

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
    else if (MoveRelated(node))
    {
      freezeWorklist = new G::NodeList<TEMP::Temp>(node, freezeWorklist);
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

  freezeWorklist = NULL;
  spillWorklist = NULL;

  spilledNodes = NULL;
  coalescedNodes = NULL;
  coloredNodes = NULL;

  selectStack = NULL;

  /****************** Move Sets ****************/
  //All move instructions is in one of the sets below.
  //Already coalesced moves
  coalescedMoves = NULL;
  //Moves whose src and dst interfere.
  constrainedMoves = NULL;
  //Moves don't need to coalesce.
  frozenMoves = NULL;
  //Moves likely to be coalesced later.
  worklistMoves = live->moves;
  //Moves not ready to be coalesced (Do not satisfy Briggs or George)
  activeMoves = NULL;

  degreeTab = new G::Table<TEMP::Temp, int>();
  colorTab = new G::Table<TEMP::Temp, int>();
  moveListTab = new G::Table<TEMP::Temp, LIVE::MoveList>();
  aliasTab = new G::Table<TEMP::Temp, G::Node<TEMP::Temp>>();

  G::NodeList<TEMP::Temp> *nodes = (live->graph)->Nodes();
  for (; nodes; nodes = nodes->tail)
  {
    TEMP::Temp *temp = nodes->head->NodeInfo();
    int *color = new int;
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

    int *degree = new int;
    *degree = nodes->head->Degree();
    degreeTab->Enter(nodes->head, degree);
    /* Initial alias table */
    G::Node<TEMP::Temp> *alias = nodes->head;
    aliasTab->Enter(nodes->head, alias);

    /* Initial movelist table*/
    LIVE::MoveList *list = worklistMoves;
    LIVE::MoveList *movelist = NULL;
    for (; list; list = list->tail)
    {
      if (list->src == nodes->head || list->dst == nodes->head)
      {
        movelist = new LIVE::MoveList(list->src, list->dst, movelist);
      }
    }
    moveListTab->Enter(nodes->head, movelist);
  }
}
static void EnableMoves(G::NodeList<TEMP::Temp> *nodes)
{
  for (; nodes; nodes = nodes->tail)
  {
    for (LIVE::MoveList *m = NodeMoves(nodes->head); m; m = m->tail)
    {
      if (LIVE::isinMoveList(m->src, m->dst, activeMoves))
      {
        activeMoves = LIVE::SubMoveList(activeMoves, new LIVE::MoveList(m->src, m->dst, NULL));
        worklistMoves = new LIVE::MoveList(m->src, m->dst, worklistMoves);
      }
    }
  }
}
static G::Node<TEMP::Temp> *alias(G::Node<TEMP::Temp> *n)
{
  return aliasTab->Look(n);
}

static G::Node<TEMP::Temp> *GetAlias(G::Node<TEMP::Temp> *t)
{
  if (coalescedNodes->InNodeList(t))
  {
    return GetAlias(alias(t));
  }
  else
  {
    return t;
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
      printf("-------====spilledNodes temp :%d=====-----\n", n->NodeInfo()->Int());
    }
    else
    {
      int *color = colorTab->Look(n);
      *color = i;
      printf("-------====assign color temp :%d=====-----\n", n->NodeInfo()->Int());
    }
  }
  for (G::NodeList<TEMP::Temp> *p = live->graph->mynodes; p; p = p->tail)
  {
    colorTab->Enter(p->head, colorTab->Look(GetAlias(p->head)));
  }
}

static void SelectSpill()
{

  G::Node<TEMP::Temp> *m = NULL;
  //calculate spilling weight
  int max = 0;

  for (G::NodeList<TEMP::Temp> *spill = spillWorklist; spill; spill = spill->tail)
  {
    TEMP::Temp *t = spill->head->NodeInfo();
    if (intemp(notSpillTemps, t))
    {
      continue;
    }
    if (spill->head->Degree() > max)
    {
      m = spill->head;
      max = m->Degree();
    }
  }
  if (m)
  {
    spillWorklist = G::NodeList<TEMP::Temp>::SubList(spillWorklist, new G::NodeList<TEMP::Temp>(m, NULL));
    simplifyWorklist = new G::NodeList<TEMP::Temp>(m, simplifyWorklist);
    FreezeMoves(m);
  }
  else
  {
    m = spillWorklist->head;
    spillWorklist = spillWorklist->tail;
    simplifyWorklist = new G::NodeList<TEMP::Temp>(m, simplifyWorklist);
    FreezeMoves(m);
  }
}

static void DecrementDegree(G::Node<TEMP::Temp> *n)
{

  int *degree = degreeTab->Look(n);
  int d = *degree; // Optimization
  *degree = *degree - 1;
  int *color = colorTab->Look(n);
  if (d == K && *color == 0)
  {
    //If n and its adjacent nodes are in activeMoves,
    EnableMoves(new G::NodeList<TEMP::Temp>(n, n->Adj()));
    spillWorklist = G::NodeList<TEMP::Temp>::SubList(spillWorklist, new G::NodeList<TEMP::Temp>(n, NULL));
    if (MoveRelated(n))
    {
      freezeWorklist = new G::NodeList<TEMP::Temp>(n, freezeWorklist);
    }
    else
    {
      simplifyWorklist = new G::NodeList<TEMP::Temp>(n, simplifyWorklist);
    }
  }
}
static void Simplify()
{

  G::Node<TEMP::Temp> *node = simplifyWorklist->head;
  simplifyWorklist = simplifyWorklist->tail;
  //push n to stack
  selectStack = new G::NodeList<TEMP::Temp>(node, selectStack);
  G::NodeList<TEMP::Temp> *adj = node->Adj();
  for (; adj; adj = adj->tail)
  {
    DecrementDegree(adj->head);
  }
  printf("-------====Simplify temp :%d=====-----\n", node->NodeInfo()->Int());
}

static void Combine(G::Node<TEMP::Temp> *u, G::Node<TEMP::Temp> *v)
{
  if (freezeWorklist->InNodeList(v))
  {
    freezeWorklist = G::NodeList<TEMP::Temp>::SubList(freezeWorklist, new G::NodeList<TEMP::Temp>(v, NULL));
  }
  else
  {
    spillWorklist = G::NodeList<TEMP::Temp>::SubList(spillWorklist, new G::NodeList<TEMP::Temp>(v, NULL));
  }
  coalescedNodes = new G::NodeList<TEMP::Temp>(v, coalescedNodes);
  aliasTab->Enter(v, u);

  //Combine v's movelist by u's
  moveListTab->Enter(u, LIVE::UnionMoveList(movelist(u), movelist(v)));

  //If it's necessary?
  //EnableMoves(G_NodeList(v,NULL));

  for (G::NodeList<TEMP::Temp> *adj = v->Adj(); adj; adj = adj->tail)
  {
    G::Node<TEMP::Temp> *t = adj->head;
    G::Graph<TEMP::Temp>::AddEdge(t, u);
    DecrementDegree(t);
  }
  if (degree(u) >= K && freezeWorklist->InNodeList(u))
  {
    freezeWorklist = G::NodeList<TEMP::Temp>::SubList(freezeWorklist, new G::NodeList<TEMP::Temp>(u, NULL));
    spillWorklist = new G::NodeList<TEMP::Temp>(u, spillWorklist);
  }
}
static void AddWorkList(G::Node<TEMP::Temp> *n)
{
  if (!precolor(n) && !MoveRelated(n) && degree(n) < K)
  {
    freezeWorklist = G::NodeList<TEMP::Temp>::SubList(freezeWorklist, new G::NodeList<TEMP::Temp>(n, NULL));
    simplifyWorklist = new G::NodeList<TEMP::Temp>(n, simplifyWorklist);
  }
}
static void Coalesce()
{

  printf("-------====Coalesce  =====-----\n");

  G::Node<TEMP::Temp> *x, *y, *u, *v;
  x = worklistMoves->src;
  y = worklistMoves->dst;

  //If only 1 precolored in x,y, then it must be u.
  if (precolor(GetAlias(y)))
  {
    u = GetAlias(y);
    v = GetAlias(x);
  }
  else
  {
    u = GetAlias(x);
    v = GetAlias(y);
  }
  worklistMoves = worklistMoves->tail;

  if (u == v)
  {
    coalescedMoves = new LIVE::MoveList(x, y, coalescedMoves);
    AddWorkList(u);
  }
  //If u and v are both precolored or u and v is adjacent,Cannot combine them directly.
  else if (precolor(v) || v->Adj()->InNodeList(u))
  {
    constrainedMoves = new LIVE::MoveList(x, y, constrainedMoves);
    AddWorkList(u);
    AddWorkList(v);
  }
  //If u and v satisfy the demands of Briggs,then combine them.
  //Conservative uses Briggs
  //OK uses George
  else if ((precolor(u) && OK(v, u)) || (!precolor(u) && Conservative(G::NodeList<TEMP::Temp>::UnionNodeList(u->Adj(), v->Adj()))))
  {
    coalescedMoves = new LIVE::MoveList(x, y, coalescedMoves);
    Combine(u, v);
    AddWorkList(u);
  }
  //Can't coalesce now
  else
  {
    activeMoves = new LIVE::MoveList(x, y, activeMoves);
  }
}
static void FreezeMoves(G::Node<TEMP::Temp> *u)
{
  for (LIVE::MoveList *m = NodeMoves(u); m; m = m->tail)
  {
    G::Node<TEMP::Temp> *x = m->src;
    G::Node<TEMP::Temp> *y = m->dst;

    //v is the Move neighbor of u.
    G::Node<TEMP::Temp> *v;
    if (GetAlias(y) == GetAlias(u))
    {
      v = GetAlias(x);
    }
    else
    {
      v = GetAlias(y);
    }
    activeMoves = LIVE::SubMoveList(activeMoves, new LIVE::MoveList(x, y, NULL));
    frozenMoves = new LIVE::MoveList(x, y, frozenMoves);
    if (NodeMoves(v) == NULL && degree(v) < K)
    {
      freezeWorklist = G::NodeList<TEMP::Temp>::SubList(freezeWorklist, new G::NodeList<TEMP::Temp>(v, NULL));
      simplifyWorklist = new G::NodeList<TEMP::Temp>(v, simplifyWorklist);
    }
  }
}
static void Freeze()
{

  printf("-------====Freeze=====-----\n");
  G::Node<TEMP::Temp> *u = freezeWorklist->head;
  freezeWorklist = freezeWorklist->tail;
  simplifyWorklist = new G::NodeList<TEMP::Temp>(u, simplifyWorklist);
  FreezeMoves(u);
}
} // namespace RA