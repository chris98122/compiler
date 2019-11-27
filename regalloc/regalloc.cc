#include "tiger/regalloc/regalloc.h"

namespace RA
{

Result RegAlloc(F::Frame *f, AS::InstrList *il)
{
  // TODO: Put your codes here (lab6).
  TEMP::Map *map = new TEMP::Map();
  TEMP::TempList *def, *use;
  int count = 0;
  while (il && il->head)
  {
    AS::Instr *instr = il->head;
    switch (instr->kind)
    {
    case AS::Instr::MOVE:
      def = ((AS::MoveInstr *)instr)->dst;
      use = ((AS::MoveInstr *)instr)->src;

      break;
    case AS::Instr::OPER:
      def = ((AS::OperInstr *)instr)->dst;
      use = ((AS::OperInstr *)instr)->src;
      break;
    default:
      def = NULL;
      use = NULL;
      break;
    }
    while (def && def->head)
    {
      std::string reg = "r" + std::to_string(count);
      map->Enter(def->head, &reg);
      count++;
      def = def->tail;
    }
    while (use && use->head)
    {
      std::string reg = "r" + std::to_string(count);
      map->Enter(use->head, &reg);
      count++;
      use = use->tail;
    }

    il = il->tail;
  }
  return Result(map, il);
}

} // namespace RA