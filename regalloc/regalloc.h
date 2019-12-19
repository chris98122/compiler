#ifndef TIGER_REGALLOC_REGALLOC_H_
#define TIGER_REGALLOC_REGALLOC_H_

#include "tiger/codegen/assem.h"
#include "tiger/frame/frame.h"
#include "tiger/frame/temp.h"

#include "tiger/liveness/liveness.h"
#include "tiger/util/graph.h"
#include "tiger/liveness/flowgraph.h"

#include <iostream>
#include <sstream>
#include <set>

#define K 15
namespace RA
{

class Result
{
public:
  TEMP::Map *coloring;
  AS::InstrList *il;
  Result(TEMP::Map *coloring, AS::InstrList *il) : coloring(coloring), il(il) {}
};


static LIVE::MoveList * movelist(G::Node<TEMP::Temp> *n);
static void Combine(G::Node<TEMP::Temp> * u,G::Node<TEMP::Temp> * v);
static void AddWorkList(G::Node<TEMP::Temp> *n);
static void DecrementDegree(G::Node<TEMP::Temp> *n);
static bool Conservative(G::NodeList<TEMP::Temp> *nodes);
static bool precolor(G::Node<TEMP::Temp> *n);
static bool OK(G::Node<TEMP::Temp> *v, G::Node<TEMP::Temp> *u);

static LIVE::MoveList *NodeMoves(G::Node<TEMP::Temp> *n);
static bool MoveRelated(G::Node<TEMP::Temp> *n);

static void RewriteProgram(F::Frame *f, AS::InstrList *pil);
static void AssignColors();
static void SelectSpill();
static void MakeWorklist();
static void Build();
static void Simplify();
static void Coalesce();

static void FreezeMoves(G::Node<TEMP::Temp> *u);
static void Freeze();

static TEMP::Map *AssignRegisters(LIVE::LiveGraph *g);


static int degree(G::Node<TEMP::Temp> * n);
static void EnableMoves(G::NodeList<TEMP::Temp> * nodes);
static G::Node<TEMP::Temp> *GetAlias(G::Node<TEMP::Temp> *t);
static G::Node<TEMP::Temp> *alias(G::Node<TEMP::Temp> *n);
Result RegAlloc(F::Frame *f, AS::InstrList *il);

static G::NodeList<TEMP::Temp> *simplifyWorklist;
static G::NodeList<TEMP::Temp> *spillWorklist;
static G::NodeList<TEMP::Temp> *freezeWorklist;

static G::NodeList<TEMP::Temp> *coloredNodes;
static G::NodeList<TEMP::Temp> *spilledNodes;
static G::NodeList<TEMP::Temp> *coalescedNodes;

static LIVE::MoveList *coalescedMoves;
static LIVE::MoveList *constrainedMoves;
static LIVE::MoveList *frozenMoves;
static LIVE::MoveList *worklistMoves; //All move instructions.
static LIVE::MoveList *activeMoves;

static TEMP::TempList *notSpillTemps;

static G::NodeList<TEMP::Temp> *selectStack;

static LIVE::LiveGraph *live;

static G::Table<TEMP::Temp, int> *colorTab; //binding points to an int (color).
static G::Table<TEMP::Temp, LIVE::MoveList> *moveListTab;
static G::Table<TEMP::Temp, G::Node<TEMP::Temp> > *aliasTab;
static G::Table<TEMP::Temp, int> *degreeTab;

static std::string *hard_reg[16] =
    {new std::string("none"), new std::string("%rax"), new std::string("%rbx"),
     new std::string("%rcx"), new std::string("%rdx"), new std::string("%rsi"),
     new std::string("%rdi"), new std::string("%rbp"), new std::string("%r8"),
     new std::string("%r9"), new std::string("%r10"), new std::string("%r11"),
     new std::string("%r12"), new std::string("%r13"), new std::string("%r14"),
     new std::string("%r15")};

} // namespace RA

#endif