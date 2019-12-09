%filenames = "scanner"

 /*
  * Please don't modify the lines above.
  */

 /* You can add lex definitions here. */ 

%x COMMENT
%s STR

%%

 /*
  * TODO: Put your codes here (lab2).
  *
  * Below is examples, which you can wipe out
  * and write regular expressions and actions of your own.
  *
  * All the tokens:
  *   Parser::ID
  *   Parser::STRING
  *   Parser::INT
  *   Parser::COMMA
  *   Parser::COLON
  *   Parser::SEMICOLON
  *   Parser::LPAREN
  *   Parser::RPAREN
  *   Parser::LBRACK
  *   Parser::RBRACK
  *   Parser::LBRACE
  *   Parser::RBRACE
  *   Parser::DOT
  *   Parser::PLUS
  *   Parser::MINUS
  *   Parser::TIMES
  *   Parser::DIVIDE
  *   Parser::EQ
  *   Parser::NEQ
  *   Parser::LT
  *   Parser::LE
  *   Parser::GT
  *   Parser::GE
  *   Parser::AND
  *   Parser::OR
  *   Parser::ASSIGN
  *   Parser::ARRAY
  *   Parser::IF
  *   Parser::THEN
  *   Parser::ELSE
  *   Parser::WHILE
  *   Parser::FOR
  *   Parser::TO
  *   Parser::DO
  *   Parser::LET
  *   Parser::IN
  *   Parser::END
  *   Parser::OF
  *   Parser::BREAK
  *   Parser::NIL
  *   Parser::FUNCTION
  *   Parser::VAR
  *   Parser::TYPE
  */

 /*
  * skip white space chars.
  * space, tabs and LF
  */

<INITIAL>[ \t]+ {adjust();}
<INITIAL>\n {adjust(); errormsg.Newline();}
 /* reserved words */


<INITIAL>"<=" { adjust(); return Parser::LE; }
<INITIAL>"<" { adjust(); return Parser::LT; }
<INITIAL>">=" { adjust(); return Parser::GE; }
<INITIAL>">" { adjust(); return Parser::GT; } 
<INITIAL>"<>" { adjust(); return Parser::NEQ; }
<INITIAL>"," { adjust(); return Parser::COMMA; }
<INITIAL>";" { adjust(); return Parser::SEMICOLON; }
<INITIAL>":" { adjust(); return Parser::COLON; }
<INITIAL>"." { adjust(); return Parser::DOT; }
<INITIAL>"*" { adjust(); return Parser::TIMES; }
<INITIAL>"/" { adjust(); return Parser::DIVIDE; }
<INITIAL>"+" { adjust(); return Parser::PLUS; }
<INITIAL>"-" { adjust(); return Parser::MINUS; }
<INITIAL>"{" { adjust(); return Parser::LBRACE; }
<INITIAL>"}" { adjust(); return Parser::RBRACE; }
<INITIAL>"(" { adjust(); return Parser::LPAREN; }
<INITIAL>")" { adjust(); return Parser::RPAREN; }
<INITIAL>"&" { adjust(); return Parser::AND; }
<INITIAL>"|" { adjust(); return Parser::OR; }
<INITIAL>"=" { adjust(); return Parser::EQ; }
<INITIAL>":=" { adjust(); return Parser::ASSIGN; }
<INITIAL>":" { adjust(); return Parser::COLON; }
<INITIAL>"[" { adjust(); return Parser::LBRACK; }
<INITIAL>"]" { adjust(); return Parser::RBRACK; } 

<INITIAL>if {adjust(); return Parser::IF;}
<INITIAL>let {adjust(); return Parser::LET;}
<INITIAL>type {adjust(); return Parser::TYPE;}
<INITIAL>array {adjust(); return Parser::ARRAY;} 
<INITIAL>in {adjust(); return Parser::IN;}
<INITIAL>else  {adjust(); return Parser::ELSE;}
<INITIAL>then   {adjust(); return Parser::THEN;}
<INITIAL>end {adjust(); return Parser::END;}
<INITIAL>var {adjust(); return Parser::VAR;}
<INITIAL>of {adjust();return Parser::OF;} 
<INITIAL>function {adjust(); return Parser::FUNCTION;}
<INITIAL>to { adjust(); return Parser::TO; }
<INITIAL>for { adjust(); return Parser::FOR; }
<INITIAL>do { adjust(); return Parser::DO; }
<INITIAL>while { adjust(); return Parser::WHILE; }
<INITIAL>nil { adjust(); return Parser::NIL; }
<INITIAL>break { adjust(); return Parser::BREAK; } 

<INITIAL>[_a-zA-Z][_0-9a-zA-Z]* { adjust(); return Parser::ID; }


<INITIAL>[0-9]+ { adjust();  return Parser::INT; }


<INITIAL>"/*"   { adjust(); comm_cnt++; begin(StartCondition__::COMMENT);} 
 
<INITIAL>\"  {  adjust();generate_str();begin(StartCondition__::STR);}

<STR>\" {
   charPos_ += length();
  if( strlen(str) == 0)
  {
     setMatched("");
  }
  else
    { 
      setMatched(std::string(str)); 
    }
   
  free_str();

  begin(StartCondition__::INITIAL);
  return Parser::STRING;
  }
<STR>"\\n"                           { charPos_ += length();add_char('\n'); }
<STR>"\\t"                           { charPos_ += length();add_char('\t'); }
<STR>"\\\""                          { charPos_ += length();add_char('\"'); }
<STR>"\\\\"                          { charPos_ += length();add_char('\\'); }
<STR>\\[0-9]{3}   { charPos_ += length(); int a = atoi(matched().data() +1 ); add_char((char)a);}
<STR>\\[ \n\t\f]+\\                  { charPos_ += length(); }
<STR>"\\\^A"                         { charPos_ += length();add_char((char)1); }
<STR>"\\\^B"                         { charPos_ += length();add_char((char)2); }
<STR>"\\\^C"                         { charPos_ += length();add_char((char)3); }
<STR>"\\\^D"                         { charPos_ += length();add_char((char)4); }
<STR>"\\\^E"                         { charPos_ += length();add_char((char)5); }
<STR>"\\\^F"                         { charPos_ += length();add_char((char)6); }
<STR>"\\\^G"                         { charPos_ += length();add_char((char)7); }
<STR>"\\\^H"                         { charPos_ += length();add_char((char)8); }
<STR>"\\\^I"                         { charPos_ += length();add_char((char)9); }
<STR>"\\\^J"                         { charPos_ += length();add_char((char)10); }
<STR>"\\\^K"                         { charPos_ += length();add_char((char)11); }
<STR>"\\\^L"                         { charPos_ += length();add_char((char)12); }
<STR>"\\\^M"                         { charPos_ += length();add_char((char)13); }
<STR>"\\\^N"                         { charPos_ += length();add_char((char)14); }
<STR>"\\\^O"                         { charPos_ += length();add_char((char)15); }
<STR>"\\\^P"                         { charPos_ += length();add_char((char)16); }
<STR>"\\\^Q"                         { charPos_ += length();add_char((char)17); }
<STR>"\\\^R"                         { charPos_ += length();add_char((char)18); }
<STR>"\\\^S"                         { charPos_ += length();add_char((char)19); }
<STR>"\\\^T"                         { charPos_ += length();add_char((char)20); }
<STR>"\\\^U"                         { charPos_ += length();add_char((char)21); }
<STR>"\\\^V"                         { charPos_ += length();add_char((char)22); }
<STR>"\\\^W"                         { charPos_ += length();add_char((char)23); }
<STR>"\\\^X"                         { charPos_ += length();add_char((char)24); }
<STR>"\\\^Y"                         { charPos_ += length();add_char((char)25); }
<STR>"\\\^Z"                         { charPos_ += length();add_char((char)26); }

<STR>\\\^[A-Za-z] {
  charPos_ += length();  int a = atoi(matched().data() +1 );
   add_char((char)a);}

 
<STR>.     {   charPos_ += length(); add_char(matched().data()[0]);}

<COMMENT>"*/"  {
   adjust();
   comm_cnt--;
  if( comm_cnt  == 0)
  {

    begin(StartCondition__::INITIAL);
  }
}
<COMMENT>"/*"                        { adjust();comm_cnt++; }
<COMMENT>"\n"                        { adjust();errormsg.Newline(); }
<COMMENT>.                           { adjust();} 
. {adjust(); errormsg.Error(errormsg.tokPos, "illegal token");}
