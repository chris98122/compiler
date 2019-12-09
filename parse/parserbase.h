// Generated by Bisonc++ V6.01.00 on Thu, 05 Dec 2019 07:03:10 -0800

// hdr/includes
#ifndef ParserBase_h_included
#define ParserBase_h_included

#include <exception>
#include <vector>
#include <iostream>
// $insert preincludes
#include "tiger/absyn/absyn.h"

// hdr/baseclass

namespace // anonymous
{
    struct PI__;
}



class ParserBase
{
    public:
        enum DebugMode__
        {
            OFF           = 0,
            ON            = 1 << 0,
            ACTIONCASES   = 1 << 1
        };

// $insert tokens

    // Symbolic tokens:
    enum Tokens__
    {
        ID = 257,
        STRING,
        INT,
        COMMA,
        COLON,
        LPAREN,
        RPAREN,
        RBRACK,
        LBRACE,
        RBRACE,
        ARRAY,
        IF,
        THEN,
        WHILE,
        FOR,
        TO,
        LET,
        IN,
        END,
        BREAK,
        NIL,
        VAR,
        SEMICOLON = 302,
        DO,
        UMINUS,
        TYPE,
        FUNCTION,
        OF,
        ELSE,
        ASSIGN,
        OR,
        AND,
        EQ,
        NEQ,
        LT,
        LE,
        GT,
        GE,
        PLUS,
        MINUS,
        TIMES,
        DIVIDE,
        DOT,
        LBRACK,
    };

// $insert STYPE
union STYPE__
{
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
 };


    private:
                        // state  semval
        typedef std::pair<size_t, STYPE__> StatePair;
                       // token   semval
        typedef std::pair<int,    STYPE__> TokenPair;

        int d_stackIdx = -1;
        std::vector<StatePair> d_stateStack;
        StatePair  *d_vsp = 0;       // points to the topmost value stack
        size_t      d_state = 0;

        TokenPair   d_next;
        int         d_token;

        bool        d_terminalToken = false;
        bool        d_recovery = false;


    protected:
        enum Return__
        {
            PARSE_ACCEPT__ = 0,   // values used as parse()'s return values
            PARSE_ABORT__  = 1
        };
        enum ErrorRecovery__
        {
            UNEXPECTED_TOKEN__,
        };

        bool        d_actionCases__ = false;    // set by options/directives
        bool        d_debug__ = true;
        size_t      d_requiredTokens__;
        size_t      d_nErrors__;                // initialized by clearin()
        size_t      d_acceptedTokens__;
        STYPE__     d_val__;


        ParserBase();

        void ABORT() const;
        void ACCEPT() const;
        void ERROR() const;

        STYPE__ &vs__(int idx);             // value stack element idx 
        int  lookup__() const;
        int  savedToken__() const;
        int  token__() const;
        size_t stackSize__() const;
        size_t state__() const;
        size_t top__() const;
        void clearin__();
        void errorVerbose__();
        void lex__(int token);
        void popToken__();
        void pop__(size_t count = 1);
        void pushToken__(int token);
        void push__(size_t nextState);
        void redoToken__();
        bool recovery__() const;
        void reduce__(int rule);
        void shift__(int state);
        void startRecovery__();

    public:
        void setDebug(bool mode);
        void setDebug(DebugMode__ mode);
}; 

// hdr/abort
inline void ParserBase::ABORT() const
{
    throw PARSE_ABORT__;
}

// hdr/accept
inline void ParserBase::ACCEPT() const
{
    throw PARSE_ACCEPT__;
}


// hdr/error
inline void ParserBase::ERROR() const
{
    throw UNEXPECTED_TOKEN__;
}

// hdr/savedtoken
inline int ParserBase::savedToken__() const
{
    return d_next.first;
}

// hdr/opbitand
inline ParserBase::DebugMode__ operator&(ParserBase::DebugMode__ lhs,
                                     ParserBase::DebugMode__ rhs)
{
    return static_cast<ParserBase::DebugMode__>(
            static_cast<int>(lhs) & rhs);
}

// hdr/opbitor
inline ParserBase::DebugMode__ operator|(ParserBase::DebugMode__ lhs, 
                                     ParserBase::DebugMode__ rhs)
{
    return static_cast<ParserBase::DebugMode__>(static_cast<int>(lhs) | rhs);
};

// hdr/recovery
inline bool ParserBase::recovery__() const
{
    return d_recovery;
}

// hdr/stacksize
inline size_t ParserBase::stackSize__() const
{
    return d_stackIdx + 1;
}

// hdr/state
inline size_t ParserBase::state__() const
{
    return d_state;
}

// hdr/token
inline int ParserBase::token__() const
{
    return d_token;
}

// hdr/vs
inline ParserBase::STYPE__ &ParserBase::vs__(int idx) 
{
    return (d_vsp + idx)->second;
}

// hdr/tail
// For convenience, when including ParserBase.h its symbols are available as
// symbols in the class Parser, too.
#define Parser ParserBase


#endif



