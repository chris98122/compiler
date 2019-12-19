#ifndef TIGER_LIVENESS_LIVENESS_H_
#define TIGER_LIVENESS_LIVENESS_H_

#include "tiger/codegen/assem.h"
#include "tiger/frame/frame.h"
#include "tiger/frame/temp.h"
#include "tiger/liveness/flowgraph.h"
#include "tiger/util/graph.h"

namespace LIVE
{

class MoveList
{
public:
  G::Node<TEMP::Temp> *src, *dst;
  MoveList *tail;

  MoveList(G::Node<TEMP::Temp> *src, G::Node<TEMP::Temp> *dst, MoveList *tail)
      : src(src), dst(dst), tail(tail) {}
};

class LiveGraph
{
public:
  G::Graph<TEMP::Temp> *graph;
  MoveList *moves;
};

static TAB::Table<G::Node<AS::Instr>, TEMP::TempList> *in;
static TAB::Table<G::Node<AS::Instr>, TEMP::TempList> *out;
static TAB::Table<TEMP::Temp, G::Node<TEMP::Temp>> *temptable;
static TEMP::TempList *hardreg;

LIVE::MoveList *UnionMoveList(LIVE::MoveList *left, LIVE::MoveList *right);
LIVE::MoveList *IntersectMoveList(LIVE::MoveList *left, LIVE::MoveList *right);
LIVE::MoveList * SubMoveList(LIVE::MoveList *left,LIVE::MoveList * right);
bool isinMoveList(G::Node<TEMP::Temp> *src, G::Node<TEMP::Temp> *dst, MoveList *tail);
void edge_temp(G::Graph<AS::Instr> *flowgraph, G::Graph<TEMP::Temp> *lg);
void add_temp(G::Graph<AS::Instr> *flowgraph, G::Graph<TEMP::Temp> *lg);
void edge_hardreg();
void add_hardreg(G::Graph<TEMP::Temp> *g);
void get_in_out(G::Graph<AS::Instr> *flowgraph);
bool intemp(TEMP::TempList *list, TEMP::Temp *temp);
bool tempequal(TEMP::TempList *old, TEMP::TempList *neww);
TEMP::TempList *UnionSets(TEMP::TempList *left, TEMP::TempList *right);
LiveGraph *Liveness(G::Graph<AS::Instr> *flowgraph);
TEMP::TempList *SubSets(TEMP::TempList *left, TEMP::TempList *right);

} // namespace LIVE

#endif