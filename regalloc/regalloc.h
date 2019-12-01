#ifndef TIGER_REGALLOC_REGALLOC_H_
#define TIGER_REGALLOC_REGALLOC_H_

#include "tiger/codegen/assem.h"
#include "tiger/frame/frame.h"
#include "tiger/frame/temp.h"

#include "tiger/liveness/liveness.h"

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

Result RegAlloc(F::Frame *f, AS::InstrList *il);

static G::NodeList<TEMP::Temp >* spilledNodes; 

static G::NodeList<TEMP::Temp >* spillWorklist;
static G::NodeList<TEMP::Temp >* simplifyWorklist;

static TEMP::TempList* notSpillTemps;


static G::NodeList<TEMP::Temp >*  selectStack;

static LIVE::LiveGraph live;

static G::Table<TEMP::Temp , int> * colorTab; //binding points to an int (color).
static std::string *hard_reg[16] =
    {new std::string("none"), new std::string("%rax"), new std::string("%rbx"),
     new std::string("%rcx"), new std::string("%rdx"), new std::string("%rsi"),
     new std::string("%rdi"), new std::string("%rbp"), new std::string("%r8"),
     new std::string("%r9"), new std::string("%r10"), new std::string("%r11"),
     new std::string("%r12"), new std::string("%r13"), new std::string("%r14"),
     new std::string("%r15")};

} // namespace RA

#endif