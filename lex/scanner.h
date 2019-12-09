#ifndef TIGER_LEX_SCANNER_H_
#define TIGER_LEX_SCANNER_H_

#include <algorithm>
#include <string>
#include <cstring>

#include "scannerbase.h"
#include "tiger/errormsg/errormsg.h"
#include "tiger/parse/parserbase.h"

/* @function: generate_str
 * @input: NULL
 * @output: An empty string with a '\0'; 
 */

extern EM::ErrorMsg errormsg;

class Scanner : public ScannerBase
{
public:
  explicit Scanner(std::istream &in = std::cin, std::ostream &out = std::cout);

  Scanner(std::string const &infile, std::string const &outfile);

  int lex();

private:
  int lex__();
  int executeAction__(size_t ruleNr);

  void print();
  void preCode();
  void postCode(PostEnum__ type);
  void adjust();
  void adjustStr();

  char *generate_str();
  char *add_char(char c);
  void free_str();
 
  std::string stringBuf_;
  int charPos_; 
  int len = 0;
  char *str = NULL;
  int comm_cnt = 0;
};

inline Scanner::Scanner(std::istream &in, std::ostream &out)
    : ScannerBase(in, out), charPos_(1) {}

inline Scanner::Scanner(std::string const &infile, std::string const &outfile)
    : ScannerBase(infile, outfile), charPos_(1) {}

inline int Scanner::lex() { return lex__(); }

inline void Scanner::preCode()
{
  // optionally replace by your own code
}

/* @function: generate_str
 * @input: NULL
 * @output: An empty string with a '\0'; 
 */
inline char* Scanner::generate_str()
{
  len=0;
  str=(char *)malloc(len+1);
  str[0]='\0';
 
  return str;
}

/* @function: add_char
 * @input: a single char
 * @output: The string value,whose tail was added the char
 */
inline char * Scanner::add_char(char c)
{
  len++;
  str=(char *)realloc(str,len+1);
  str[len-1]=c;
  str[len]='\0';
 
  return str;
}

/* @function: free_char
 * @input: null
 * @output: Free the memory of str and reset 
 * the length of it.
 */
inline void Scanner::free_str()
{
  free(str);
  str=NULL;
  len=0;
}

inline void Scanner::postCode(PostEnum__ type)
{
  // optionally replace by your own code
  
}

inline void Scanner::print() { print__(); }

inline void Scanner::adjust()
{
  errormsg.tokPos = charPos_;
  charPos_ += length();
}

inline void Scanner::adjustStr() { charPos_ += length(); }

#endif // TIGER_LEX_SCANNER_H_
