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
      in_n = UnionSets(FG::Def(flownode),
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
  TEMP::TempList *hardreg = new TEMP::TempList(F::F_RAX(), new TEMP::TempList(F::F_RBX(), new TEMP::TempList(F::F_R12(), new TEMP::TempList(F::F_R13(), new TEMP::TempList(F::F_R14(), new TEMP::TempList(F::F_R15(),
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
    temptable->Enter( t->head ,g->NewNode(t->head));
  }
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

  /* Add hardregister to the graph */
  add_hardreg(lg->graph);

  /* Add nodes of hard register into temp table*/
  /* Add edge between hard register node. */
  /* Add nodes of temp register into temp table*/
  /* Add edge between temp register node. */

  return lg;
}

} // namespace LIVE