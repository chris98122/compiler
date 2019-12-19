#include "tiger/liveness/liveness.h"
#include "tiger/frame/frame.h"

namespace LIVE
{

TEMP::TempList *UnionSets(TEMP::TempList *left, TEMP::TempList *right)
{
  TEMP::TempList *unionn = left;
  for (; right; right = right->tail)
  {
    if (!intemp(unionn, right->head))
      unionn = new TEMP::TempList(right->head, unionn);
  }
  return unionn;
}
TEMP::TempList *SubSets(TEMP::TempList *left, TEMP::TempList *right)
{
  TEMP::TempList *sub = NULL;
  for (; left; left = left->tail)
  {
    if (!intemp(right, left->head))
    {
      sub = new TEMP::TempList(left->head, sub);
    }
  }
  return sub;
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
bool tempequal(TEMP::TempList *old, TEMP::TempList *neww)
{
  TEMP::TempList *cur = old;

  for (; cur; cur = cur->tail)
  {
    if (!intemp(neww, cur->head))
    {
      return false;
    }
  }
  cur = neww;
  for (; cur; cur = cur->tail)
  {
    if (!intemp(old, cur->head))
      return false;
  }
  return true;
}
void get_in_out(G::Graph<AS::Instr> *flowgraph)
{

  in = new TAB::Table<G::Node<AS::Instr>, TEMP::TempList>();
  out = new TAB::Table<G::Node<AS::Instr>, TEMP::TempList>();

  G::NodeList<AS::Instr> *flownodes = flowgraph->mynodes;
  G::Node<AS::Instr> *flownode;
  bool fixed_point = false;

  /* calculate in out templist*/
  while (!fixed_point)
  {
    fixed_point = true;
    flownodes = flowgraph->mynodes;
    for (; flownodes; flownodes = flownodes->tail)
    {
      flownode = flownodes->head;
      TEMP::TempList *in_n_old = in->Look(flownode);
      TEMP::TempList *out_n_old = out->Look(flownode);
      TEMP::TempList *in_n = NULL, *out_n = NULL;
      /* pseudocode:
			in[n] = use[n] and (out[n] – def[n])
 			out[n] = (for s in succ[n])
			            Union(in[s])
			*/
      G::NodeList<AS::Instr> *succ = flownode->Succ();
      for (; succ; succ = succ->tail)
      {
        TEMP::TempList *in_succ = in->Look(succ->head);
        out_n = UnionSets(out_n, in_succ);
      }
      in_n = UnionSets(FG::Use(flownode),
                       SubSets(out_n, FG::Def(flownode)));

      if (!tempequal(in_n_old, in_n) || !tempequal(out_n_old, out_n))
      {
        fixed_point = false;
      }
      /* Update in_n and out_n*/
      in->Enter(flownode, in_n);
      out->Enter(flownode, out_n);
    }
  }
}

void add_hardreg(G::Graph<TEMP::Temp> *g)
{
  temptable = new TAB::Table<TEMP::Temp, G::Node<TEMP::Temp>>();
  hardreg = new TEMP::TempList(F::F_RAX(), new TEMP::TempList(F::F_RBX(), new TEMP::TempList(F::F_R12(), new TEMP::TempList(F::F_R13(), new TEMP::TempList(F::F_R14(), new TEMP::TempList(F::F_R15(),
                                                                                                                                                                                          new TEMP::TempList(F::F_RDI(),
                                                                                                                                                                                                             new TEMP::TempList(F::F_RSI(),
                                                                                                                                                                                                                                new TEMP::TempList(F::F_RDX(),
                                                                                                                                                                                                                                                   new TEMP::TempList(F::F_RCX(),
                                                                                                                                                                                                                                                                      new TEMP::TempList(F::F_R8(),
                                                                                                                                                                                                                                                                                         new TEMP::TempList(F::F_R9(),
                                                                                                                                                                                                                                                                                                            new TEMP::TempList(F::F_R10(),
                                                                                                                                                                                                                                                                                                                               new TEMP::TempList(F::F_R11(), NULL))))))))))))));
  for (TEMP::TempList *t = hardreg; t; t = t->tail)
  {
    temptable->Enter(t->head, g->NewNode(t->head));
  }
}

void edge_hardreg(G::Graph<TEMP::Temp> *g)
{
  for (TEMP::TempList *A = hardreg; A; A = A->tail)
  {
    for (TEMP::TempList *B = A->tail; B; B = B->tail)
    {
      G::Node<TEMP::Temp> *nodeA = temptable->Look(A->head);
      G::Node<TEMP::Temp> *nodeB = temptable->Look(B->head);
      g->AddEdge(nodeA, nodeB);
      g->AddEdge(nodeB, nodeA);
    }
  }
}

void add_temp(G::Graph<AS::Instr> *flowgraph, G::Graph<TEMP::Temp> *lg)
{
  G::NodeList<AS::Instr> *flownodes = flowgraph->mynodes;
  G::Node<AS::Instr> *flownode;
  for (; flownodes; flownodes = flownodes->tail)
  {
    flownode = flownodes->head;
    TEMP::TempList *outn = out->Look(flownode),
                   *def = FG::Def(flownode);
    if (!(def && def->head))
    {
      continue;
    }
    TEMP::TempList *temps = UnionSets(outn, def);
    for (; temps; temps = temps->tail)
    {
      if (temps->head == F::F_SP())
      {
        continue;
      }
      // If the temp is not in the table, add it.
      if (!temptable->Look(temps->head))
      {
        G::Node<TEMP::Temp> *tempNode = lg->NewNode(temps->head);
        temptable->Enter(temps->head, tempNode);
      }
    }
  }
}

void edge_temp(G::Graph<AS::Instr> *flowgraph, LiveGraph *lg)
{
  G::NodeList<AS::Instr> *flownodes = flowgraph->mynodes;
  G::Node<AS::Instr> *flownode;
  for (; flownodes; flownodes = flownodes->tail)
  {
    flownode = flownodes->head;
    TEMP::TempList *outn,
        *def = FG::Def(flownode);
    if (!(def && def->head))
    {
      continue;
    }
    /* Add edge to the interfere graph,firstly check if the instruction
		 of the flownode is MOVE. */
    /* If the instruction is not move, add edge directly.*/
    if (!FG::IsMove(flownode))
    {
      for (; def; def = def->tail)
      {
        if (def->head == F::F_SP())
        {
          continue;
        }
        G::Node<TEMP::Temp> *defnode = temptable->Look(def->head);

        outn = out->Look(flownode);
        for (; outn; outn = outn->tail)
        {
          if (outn->head == F::F_SP())
          {
            continue;
          }
          G::Node<TEMP::Temp> *outnode = temptable->Look(outn->head);
          /* If outnode and defnode is not linked yet.*/
          if (!outnode->Adj()->InNodeList(defnode))
          {
            lg->graph->AddEdge(defnode, outnode);
            lg->graph->AddEdge(outnode, defnode);
          }
        }
      }
    }
    /* Else,the instruction is MOVE*/
    else
    {
      for (; def; def = def->tail)
      {
        if (def->head == F::F_SP())
        {
          continue;
        }
        G::Node<TEMP::Temp> *defnode = temptable->Look(def->head);
        for (outn = out->Look(flownode); outn; outn = outn->tail)
        {
          if (outn->head == F::F_SP())
          {
            continue;
          }
          G::Node<TEMP::Temp> *outnode = temptable->Look(outn->head);

          if (!outnode->Adj()->InNodeList(defnode) && !(intemp(FG::Use(flownode), outn->head)))
          {
            lg->graph->AddEdge(outnode, defnode);
            lg->graph->AddEdge(defnode, outnode);
          }
        }

        /* Add it to the movelist. It's for coalescence in ra.*/
        for (TEMP::TempList *uses = FG::Use(flownode); uses; uses = uses->tail)
        {
          if (uses->head == F::F_SP())
          {
            continue;
          }
          G::Node<TEMP::Temp> *usenode = temptable->Look(uses->head);
          if (!isinMoveList(usenode, defnode, lg->moves))
          {
            lg->moves = new LIVE::MoveList(usenode, defnode, lg->moves);
          }
        }
      }
    }
  }
}

bool isinMoveList(G::Node<TEMP::Temp> *src, G::Node<TEMP::Temp> *dst, MoveList *tail)
{
  for (; tail; tail = tail->tail)
  {
    if (src == tail->src && dst == tail->dst)
    {
      return true;
    }
  }
  return false;
}

LiveGraph *Liveness(G::Graph<AS::Instr> *flowgraph)
{
  // TODO: Put your codes here (lab6).
  LiveGraph *lg = new LiveGraph();

  /* calculate in,out sets*/
  get_in_out(flowgraph);

  /* Construct the interfere graph */
  lg->graph = new G::Graph<TEMP::Temp>();
  lg->moves = NULL;

  /* Add nodes of hard register  into temp table*/
  add_hardreg(lg->graph);

  /* Add edge between hard register node. */
  edge_hardreg(lg->graph);

  /* Add nodes of temp register into temp table*/
  add_temp(flowgraph, lg->graph);

  /* Add edge between temp register node. */
  edge_temp(flowgraph, lg);

  return lg;
}

LIVE::MoveList *IntersectMoveList(LIVE::MoveList *left, LIVE::MoveList *right)
{
  LIVE::MoveList *cur = NULL;
  for (; left; left = left->tail)
  {
    if (isinMoveList(left->src, left->dst, right))
    {
      cur = new LIVE::MoveList(left->src, left->dst, cur);
    }
  }
  return cur;
}

LIVE::MoveList *UnionMoveList(LIVE::MoveList *left, LIVE::MoveList *right)
{
  LIVE::MoveList *cur = left;
  for (; right; right = right->tail)
  {
    if (!isinMoveList(right->src, right->dst, left))
    {
      cur = new LIVE::MoveList(right->src, right->dst, cur);
    }
  }
  return cur;
}

LIVE::MoveList *SubMoveList(LIVE::MoveList *left, LIVE::MoveList *right)
{
  LIVE::MoveList *cur = NULL;
  for (; left; left = left->tail)
  {
    if (!isinMoveList(left->src, left->dst, right))
    {
      cur = new LIVE::MoveList(left->src, left->dst, cur);
    }
  }
  return cur;
}
} // namespace LIVE