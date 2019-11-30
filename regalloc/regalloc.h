#ifndef TIGER_REGALLOC_REGALLOC_H_
#define TIGER_REGALLOC_REGALLOC_H_

#include "tiger/codegen/assem.h"
#include "tiger/frame/frame.h"
#include "tiger/frame/temp.h"

#include "tiger/util/graph.h"

#include <iostream>
#include <sstream>
#include <set>
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
 
static std::set <TEMP::Temp *>spilledNodes;

} // namespace RA

#endif