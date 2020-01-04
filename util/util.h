#ifndef TIGER_UTIL_UTIL_H_
#define TIGER_UTIL_UTIL_H_

namespace U
{

class BoolList
{
public:
  bool head;
  BoolList *tail;

  BoolList(bool head, BoolList *tail) : head(head), tail(tail) {}
  void Print(FILE *out)
  {
    fprintf(out, head ? "TRUE,"
            : "FALSE,");
    if (tail == NULL)
      return;
    tail->Print(out);
  }
};

} // namespace U

#endif // TIGER_UTIL_UTIL_H_