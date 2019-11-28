#include "tiger/regalloc/regalloc.h"

namespace RA
{

Result RegAlloc(F::Frame *f, AS::InstrList *il)
{
  // TODO: Put your codes here (lab6).
  TEMP::Map *map = new TEMP::Map( );
  TEMP::TempList *def, *use;
  long count = 0;
  AS::InstrList *i = il;
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
      std::string *reg = new std::string("%r" + std::to_string(count));
      map->Enter(def->head, reg);
      count++;
      def = def->tail;
    }
    while (use && use->head)
    {
      std::string *reg = new std::string("%r" + std::to_string(count));
      map->Enter(use->head, reg);
      count++;
      use = use->tail;
    }
    il = il->tail;
  }
  return Result(map,i);

    // int offset = 8;
    //   std::string assem = "movl " + std::to_string(offset) + "(`s0), `d0";

    //   AS::Instr *newInstr = new AS::OperInstr(assem, new TEMP::TempList(use->head, NULL), new TEMP::TempList(F::F_FP(), NULL), NULL);
    //   before = newInstr;
    //   use = use->tail;

    //   il->tail = new AS::InstrList(il->head, il->tail);
    //   il->head = before;

    //   il = il->tail;
}

} // namespace RA