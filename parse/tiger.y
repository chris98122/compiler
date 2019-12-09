%filenames parser
%scanner tiger/lex/scanner.h
%baseclass-preinclude tiger/absyn/absyn.h

 /*
  * Please don't modify the lines above.
  */

%union {
  int ival;
  std::string* sval;
  S::Symbol *sym;
  A::Exp *exp;
  A::ExpList *explist;
  A::Var *var;
  A::DecList *declist;
  A::Dec *dec;
  A::EFieldList *efieldlist;
  A::EField *efield;
  A::NameAndTyList *tydeclist;
  A::NameAndTy *tydec;
  A::FieldList *fieldlist;
  A::Field *field;
  A::FunDecList *fundeclist;
  A::FunDec *fundec;
  A::Ty *ty;
  }

%token<sym> ID
%token <sval> STRING
%token <ival> INT

%token 
  COMMA COLON LPAREN RPAREN RBRACK 
  LBRACE RBRACE  
  ARRAY IF THEN  WHILE FOR TO LET IN END  
  BREAK NIL
   VAR  

%type <exp>   exp expseq   
%type <explist> actuals  nonemptyactuals sequencing  sequencing_exps
%type <var>  lvalue one oneormore
%type <declist> decs decs_nonempty
%type <dec>  decs_nonempty_s vardec
%type <efieldlist> rec rec_nonempty
%type <efield> rec_one
%type <tydeclist> tydec
%type <tydec>  tydec_one
%type <fieldlist> tyfields tyfields_nonempty
%type <ty> ty
%type <fundeclist> fundec
%type <fundec> fundec_one

%start program


 /*
  * Put your codes here (lab3).
  */
    
%left SEMICOLON

%nonassoc DO
  
%nonassoc UMINUS  
%nonassoc TYPE
%nonassoc FUNCTION
%nonassoc OF 
%nonassoc  ELSE

%right ASSIGN
%left OR
%left AND

%nonassoc EQ NEQ LT LE GT GE
%left PLUS MINUS
%left TIMES DIVIDE 
%left DOT LBRACK  
%%
program:  	exp  {absyn_root = $1;}; 
exp :  INT  	{$$ =  new A::IntExp(errormsg.tokPos,$1); } 
      |STRING   {$$ = new A::StringExp(errormsg.tokPos,$1);}
      |lvalue   {$$ = new A::VarExp(errormsg.tokPos,$1);}
      |NIL      {$$ = new A::NilExp(errormsg.tokPos);}  
      |LPAREN sequencing RPAREN  {$$ =new A::SeqExp(errormsg.tokPos,$2);}
      |exp PLUS exp  {$$ =new A::OpExp(errormsg.tokPos, A::PLUS_OP,$1,$3);}
      |exp MINUS exp  {$$ =new A::OpExp(errormsg.tokPos,A::MINUS_OP,$1,$3);}
      |exp TIMES exp   {$$ =new A::OpExp(errormsg.tokPos,A::TIMES_OP,$1,$3);}
      |exp DIVIDE exp  {$$ =new A::OpExp(errormsg.tokPos,A::DIVIDE_OP,$1,$3);}
      |exp EQ exp      {$$ =new A::OpExp(errormsg.tokPos,A::EQ_OP,$1,$3);}
      |exp NEQ exp     {$$ =new A::OpExp(errormsg.tokPos,A::NEQ_OP,$1,$3);}
      |exp LT exp      {$$ =new A::OpExp(errormsg.tokPos,A::LT_OP,$1,$3);}
      |exp LE exp      {$$ =new A::OpExp(errormsg.tokPos,A::LE_OP,$1,$3);}
      |exp GT exp      {$$ =new A::OpExp(errormsg.tokPos,A::GT_OP,$1,$3);}
      |exp GE exp      {$$ =new A::OpExp(errormsg.tokPos,A::GE_OP,$1,$3);}
       |exp AND exp     {$$ =new A::IfExp(errormsg.tokPos,$1,$3,new A::IntExp(errormsg.tokPos,0));} 
      |exp OR exp      {$$ =new A::IfExp(errormsg.tokPos,$1,new A::IntExp(errormsg.tokPos,1),$3);}
       |MINUS exp %prec UMINUS  {$$ = new A::OpExp(errormsg.tokPos,A::MINUS_OP,new A::IntExp(errormsg.tokPos,0),$2);}
      |LPAREN exp RPAREN    {$$ = $2;}
      |ID LPAREN actuals RPAREN  {$$ =new A::CallExp(errormsg.tokPos, $1 ,$3);}     
      |lvalue ASSIGN exp         {$$ =new A::AssignExp(errormsg.tokPos,$1,$3);}
       |IF exp THEN exp  {$$ = new A::IfExp(errormsg.tokPos,$2,$4,NULL);}
      |IF exp THEN exp ELSE exp   {$$ =new A::IfExp(errormsg.tokPos,$2,$4,$6);} 
      |WHILE exp DO exp            {$$ =new A::WhileExp(errormsg.tokPos,$2,$4);}
      |FOR ID ASSIGN exp TO exp DO exp  {$$ = new A::ForExp(errormsg.tokPos,  $2 ,$4,$6,$8);}
      |BREAK                            {$$ = new A::BreakExp(errormsg.tokPos);}
      |LET decs IN expseq END           {$$ = new A::LetExp(errormsg.tokPos,$2,$4);}
      |ID LBRACE rec  RBRACE            {$$ = new A::RecordExp(errormsg.tokPos, $1  ,$3);}
      |ID LBRACK exp RBRACK OF exp      {$$ = new A::ArrayExp(errormsg.tokPos, $1 ,$3,$6);}
       | LPAREN RPAREN { $$ = new A::VoidExp(errormsg.tokPos ); }
        ;

 
lvalue : ID    {$$ = new A::SimpleVar(errormsg.tokPos,  $1 );}
       | oneormore  {$$ = $1;}
       ;

oneormore : one       {$$ = $1;}
           |oneormore DOT ID   {$$ = new A::FieldVar(errormsg.tokPos,$1,  $3 );}
           |oneormore LBRACK exp RBRACK   {$$ = new A::SubscriptVar(errormsg.tokPos,$1,$3);}
           ;

one : ID DOT ID  {$$ =new A::FieldVar(errormsg.tokPos,new A::SimpleVar(errormsg.tokPos, $1 ),  $3 );}
    | ID LBRACK exp RBRACK {$$ =new A::SubscriptVar(errormsg.tokPos,new A::SimpleVar(errormsg.tokPos, $1 ),$3);}
    ;



rec   :  rec_nonempty  {$$ =$1;}
        |  {$$ = NULL;} 
        ;

rec_nonempty      :    rec_one {$$ = new A::EFieldList($1,NULL);}
            	      |rec_one COMMA rec_nonempty  {$$ = new A::EFieldList($1,$3);}
                      ;
rec_one      :  ID EQ exp   {$$ = new A::EField( $1 ,$3);}
             ;


sequencing :exp          {$$ = new A::ExpList($1,NULL);}
                   | exp SEMICOLON sequencing  {$$ = new A::ExpList($1,$3);}
                   ;
          
   
actuals : nonemptyactuals   {$$ = $1;}
        |  {$$ = NULL;} 
        ;

nonemptyactuals : exp     {$$ =  new A::ExpList($1,NULL);}
                | exp COMMA nonemptyactuals {$$ =  new A::ExpList($1,$3);}
                ;

expseq  :  sequencing   {$$ =  new A::SeqExp(errormsg.tokPos,$1);} 
        ;

decs :  decs_nonempty  {$$ = $1;}
       | {$$ = NULL;}
       ;

decs_nonempty : decs_nonempty_s  {$$ = new A::DecList($1,NULL);}
              | decs_nonempty_s decs_nonempty   {$$ =new A::DecList($1,$2);}
              ;

decs_nonempty_s : tydec  {$$ =new A::TypeDec(errormsg.tokPos,$1);}
                |vardec  {$$ = $1;}
                |fundec  {$$ = new A::FunctionDec(errormsg.tokPos,$1);}
                ;


tydec   :  tydec_one   {$$ = new A::NameAndTyList ($1,NULL);}
          |tydec_one tydec      {$$ = new A::NameAndTyList ($1,$2);}
           ;      
ty    :   ID    {$$ =new A::NameTy(errormsg.tokPos, $1 );}
         |LBRACE tyfields RBRACE  {$$ = new A::RecordTy(errormsg.tokPos,$2);}
         |ARRAY OF ID {$$ = new A::ArrayTy(errormsg.tokPos, $3 );}
         ;
tydec_one : TYPE ID EQ ty    {$$ = new A::NameAndTy( $2,$4);}
          ; 


         
tyfields  : tyfields_nonempty  {$$ = $1;}
          |   {$$ = NULL;}
          ; 

tyfields_nonempty : ID COLON ID  {$$ = new A::FieldList(new A::Field(errormsg.tokPos, $1 , $3 ),NULL);}
                  | ID COLON ID COMMA tyfields_nonempty  {$$ =new A::FieldList(new A::Field(errormsg.tokPos, $1 , $3 ),$5);}
                  ;



vardec:  VAR ID ASSIGN exp  {$$ = new A::VarDec(errormsg.tokPos,$2, nullptr, $4);}
  |  VAR ID COLON ID ASSIGN exp  {$$ = new A::VarDec(errormsg.tokPos, $2, $4, $6);}
  ;



fundec    :   fundec_one    {$$ = new A::FunDecList($1,NULL);}
          |   fundec_one fundec    {$$ = new A::FunDecList($1,$2);}
          ;
          
fundec_one    :  FUNCTION ID LPAREN tyfields RPAREN COLON ID EQ exp  {$$ = new A:: FunDec(errormsg.tokPos, $2 ,$4, $7,$9);}
             |FUNCTION ID LPAREN tyfields RPAREN   EQ exp  {$$ = new A:: FunDec(errormsg.tokPos, $2 ,$4, NULL,$7);}
            ; 
 