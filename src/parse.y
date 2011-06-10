/*
** 2011 May 26
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
** This file contains grammar for XJD1 query language
** using the lemon parser generator to generate C code that runs
** the parser.  Lemon will also generate a header file containing
** numeric codes for all of the tokens.
*/

// All token codes are small integers with #defines that begin with "TK_"
%token_prefix TK_

// The type of the data attached to each token is Token.  This is also the
// default type for non-terminals.
//
%token_type {Token}
%default_type {Token}

// The generated parser function takes a 4th argument as follows:
%extra_argument {Parse *p}

// The name of the generated procedure that implements the parser
// is as follows:
%name xjd1Parser

// The following text is included near the beginning of the C source
// code file that implements the parser.
//
%include {
#include "xjd1Int.h"

/*
** Disable all error recovery processing in the parser push-down
** automaton.
*/
#define YYNOERRORRECOVERY 1

/*
** Make yytestcase() is not used
*/
#define yytestcase(X)

} // end %include

// Input is a single XJD1 command
input ::= cmd SEMI.

/////////////////////////// Expression Processing /////////////////////////////
//

%left OR.
%left AND.
%right NOT.
%left IS LIKEOP BETWEEN IN NE EQ.
%left GT LE LT GE.
%right ESCAPE.
%left BITAND BITOR LSHIFT RSHIFT.
%left PLUS MINUS.
%left STAR SLASH REM.
%left CONCAT.
%left COLLATE.
%right BITNOT.

%include {
  /* Generate an Expr object from a token */
  static Expr *tokExpr(Parse *p, int eType, Token *pTok){
    Expr *pNew = xjd1PoolMallocZero(p->pPool, sizeof(*pNew));
    if( pNew ){
      pNew->eType = eType; 
      pNew->u.tk = *pTok;
    }
    return pNew;
  }

  /* Generate an Expr object that is a binary operator on two
  ** other Expr objects. */
  static Expr *biExpr(Parse *p, Expr *pLeft, int eOp, Expr *pRight){
    Expr *pNew = xjd1PoolMallocZero(p->pPool, sizeof(*pNew));
    if( pNew ){
      pNew->eType = eOp; 
      pNew->u.bi.pLeft = pLeft;
      pNew->u.bi.pRight = pRight;
    }
    return pNew;
  }

  /* Generate an Expr object that is a function call. */
  static Expr *funcExpr(Parse *p, Token *pFName, ExprList *pArgs){
    Expr *pNew = xjd1PoolMallocZero(p->pPool, sizeof(*pNew));
    if( pNew ){
      pNew->eType = TK_FUNCTION; 
      pNew->u.func.name = *pFName;
      pNew->u.func.args = pArgs;
    }
    return pNew;
  }

  /* Generate an Expr object that is a subquery. */
  static Expr *subqExpr(Parse *p, Query *pQuery){
    Expr *pNew = xjd1PoolMallocZero(p->pPool, sizeof(*pNew));
    if( pNew ){
      pNew->eType = TK_SELECT;
      pNew->u.q = pQuery;
    }
    return pNew;
  }

  /* Append a new expression to an expression list.  Allocate the
  ** expression list object if necessary. */
  static ExprList *apndExpr(Parse *p, ExprList *pList, Expr *pExpr, Token *pT){
    ExprItem *pItem;
    if( pList==0 ){
      pList = xjd1PoolMallocZero(p->pPool, sizeof(*pList)+4*sizeof(ExprItem));
      if( pList==0 ) return 0;
      pList->nEAlloc = 4;
    }
    if( pList->nEAlloc<=pList->nEItem ){
      ExprItem **pNew;
      pNew = xjd1PoolMalloc(p->pPool, sizeof(ExprItem)*pList->nEAlloc*2);
      if( pNew==0 ) return pList;
      memcpy(pNew, pList->apEItem, pList->nEItem*sizeof(ExprItem));
      pList->nEAlloc *= 2;
      pList->apEItem = pNew;
    }
    pItem = &pList->apEItem[pList->nEItem++];
    pItem->tkAs = *pT;
    pItem->pExpr = pExpr;
    return pList;
  }
}

jvalue(A) ::= INTEGER(X).  {A = X;}
jvalue(A) ::= FLOAT(X).    {A = X;}
jvalue(A) ::= STRING(X).   {A = X;}
jvalue(A) ::= TRUE(X).     {A = X;}
jvalue(A) ::= FALSE(X).    {A = X;}
jvalue(A) ::= NULL(X).     {A = X;}
jvalue(A) ::= JVALUE(X).   {A = X;}

%type jexpr {Expr*}
jexpr(A) ::= expr(X).      {A = X;}
jexpr(A) ::= JVALUE(X).    {A = tokExpr(p,@X,&X);}

%type lvalue {Expr*}
lvalue(A) ::= ID(X).                  {A = tokExpr(p,@X,&X);}
lvalue(A) ::= lvalue(X) DOT ID(Y).    {A = biExpr(p,X,TK_DOT,tokExpr(p,@Y,&Y));}
lvalue(A) ::= lvalue(X) LB expr(Y) RB.{A = biExpr(p,X,TK_LB,Y);}

%type expr {Expr*}
expr(A) ::= lvalue(X).    {A = X;}
expr(A) ::= INTEGER(X).   {A = tokExpr(p,@X,&X);}
expr(A) ::= FLOAT(X).     {A = tokExpr(p,@X,&X);}
expr(A) ::= STRING(X).    {A = tokExpr(p,@X,&X);}
expr(A) ::= TRUE(X).      {A = tokExpr(p,@X,&X);}
expr(A) ::= FALSE(X).     {A = tokExpr(p,@X,&X);}
expr(A) ::= NULL(X).      {A = tokExpr(p,@X,&X);}
expr(A) ::= ID(X) LP exprlist(Y) RP.  {A = funcExpr(p,&X,Y);}
expr(A) ::= expr(X) AND(OP) expr(Y).  {A = biExpr(p,X,@OP,Y);}
expr(A) ::= expr(X) OR(OP) expr(Y).              {A = biExpr(p,X,@OP,Y);}
expr(A) ::= expr(X) LT|GT|GE|LE(OP) expr(Y).     {A = biExpr(p,X,@OP,Y);}
expr(A) ::= expr(X) EQ|NE(OP) expr(Y).           {A = biExpr(p,X,@OP,Y);}
expr(A) ::= expr(X) BITAND|BITOR|LSHIFT|RSHIFT(OP) expr(Y).
                                                 {A = biExpr(p,X,@OP,Y);}
expr(A) ::= expr(X) PLUS|MINUS(OP) expr(Y).      {A = biExpr(p,X,@OP,Y);}
expr(A) ::= expr(X) STAR|SLASH|REM(OP) expr(Y).  {A = biExpr(p,X,@OP,Y);}
expr(A) ::= expr(X) CONCAT(OP) expr(Y).          {A = biExpr(p,X,@OP,Y);}
%type likeop {int}
likeop(A) ::= LIKEOP(OP).                        {A = @OP;}
likeop(A) ::= NOT LIKEOP(OP).                    {A = 128+@OP;}
expr(A) ::= expr(X) likeop(OP) expr(Y). [LIKEOP] {A = biExpr(p,X,OP,Y);}
expr(A) ::= expr(X) IS(OP) expr(Y).              {A = biExpr(p,X,@OP,Y);}
expr(A) ::= expr(X) IS NOT expr(Y).              {A = biExpr(p,X,TK_IS+128,Y);}
expr(A) ::= NOT(OP) expr(X).                     {A = biExpr(p,X,@OP,0);}
expr(A) ::= BITNOT(OP) expr(X).                  {A = biExpr(p,X,@OP,0);}
expr(A) ::= MINUS(OP) expr(X). [BITNOT]          {A = biExpr(p,X,@OP,0);}
expr(A) ::= PLUS(OP) expr(X). [BITNOT]           {A = biExpr(p,X,@OP,0);}
expr(A) ::= LP select(X) RP.                     {A = subqExpr(p,X);}
expr(A) ::= LP expr(X) RP.                       {A = X;}

%type exprlist {ExprList*}
%type nexprlist {ExprList*}
exprlist(A) ::= nexprlist(X).     {A = X;}
exprlist(A) ::= .                 {A = 0;}
nexprlist(A) ::= expr(X).                     {A = apndExpr(p,0,X,0);}
nexprlist(A) ::= nexprlist(X) COMMA expr(Y).  {A = apndExpr(p,X,Y,0);}

//////////////////////// The SELECT statement /////////////////////////////////
//
cmd ::= select.

%left UNION EXCEPT.
%left INTERSECT.
%type select {Query*}
%type eselect {Query*}
%type oneselect {Query*}

%include {
  /* The value of a LIMIT ... OFFSET ... clause. */
  typedef struct LimitOffset {
    Expr *pLimit;
    Expr *pOffset;
  } LimitOffset;

  /* The value of a GROUP BY ... HAVING ... clause */
  typedef struct GroupByHaving {
    ExprList *pGroupBy;
    Expr *pHaving;
  } GroupByHaving;

  /* Construct a simple query object */
  static Query *simpleQuery(
    Parse *p,
    ExprList *pCol,
    DataSrc *pFrom,
    Expr *pWhere,
    GroupByHaving *pGroupBy,
    ExprList *pOrderBy,
    LimitOffset *pLimit
  ){
    Query *pNew = xjd1PoolMallocZero(p->pPool, sizeof(*pNew));
    if( pNew ){
      pNew->eQType = TK_SELECT;
      pNew->u.simple.pCol = pCol;
      pNew->u.simple.pFrom = pFrom;
      pNew->u.simple.pWhere = pWhere;
      pNew->u.simple.pGroupBy = pGroupBy ? pGroupBy->pGroupBy : 0;
      pNew->u.simple.pHaving = pGroupBy ? pGroupBy->pHaving : 0;
      pNew->u.simple.pOrderBy = pOrderBy;
      pNew->u.simple.pLimit = pLimit ? pLimit->pLimit : 0;
      pNew->u.simple.pOffset = pLimit ? pLimit->pOffset : 0;
    }
    return pNew;
  }

  /* Construct a compound query object */
  static Query *compoundQuery(
    Parse *p,
    Query *pLeft,
    int eOp,
    Query *pRight
  ){
    Query *pNew = xjd1PoolMallocZero(p->pPool, sizeof(*pNew));
    if( pNew ){
      pNew->eQType = eOp;
      pNew->u.compound.pLeft = pLeft;
      pNew->u.compound.pRight = pRight;
    }
    return pNew;
  }
}


select(A) ::= oneselect(X).                        {A = X;}
select(A) ::= eselect(X) UNION(OP) eselect(Y).     {A=compoundQuery(p,X,@OP,Y);}
select(A) ::= eselect(X) UNION ALL(OP) eselect(Y). {A=compoundQuery(p,X,@OP,Y);}
select(A) ::= eselect(X) EXCEPT(OP) eselect(Y).    {A=compoundQuery(p,X,@OP,Y);}
select(A) ::= eselect(X) INTERSECT(OP) eselect(Y). {A=compoundQuery(p,X,@OP,Y);}
eselect(A) ::= select(X).                          {A = X;}
eselect(A) ::= expr(X).       // must be (SELECT...)
  {A = X->u.q;}

oneselect(A) ::= SELECT selcollist_opt(S) from(F) where_opt(W)
                    groupby_opt(G) orderby_opt(O) limit_opt(L).
  {A = simpleQuery(p,S,F,W,&G,O,&L);}


// selcollist is a list of expressions that are to become the return
// values of the SELECT statement.
//
%type selcollist_opt {ExprList*}
selcollist_opt(A) ::= .                 {A = 0;}
selcollist_opt(A) ::= selcollist.       {A = 0;}
selcollist ::= selcol.
selcollist ::= selcollist COMMA selcol.
selcol ::= expr.
selcol ::= expr AS ID.

// A complete FROM clause.
//
%type from {DataSrc*}
from(A) ::= .                  {A = 0;}
from(A) ::= FROM fromlist.     {A = 0;}
fromlist ::= fromitem.
fromlist ::= fromlist COMMA fromitem.
fromitem ::= ID.
fromitem ::= ID AS ID.
fromitem ::= LP select RP.
fromitem ::= fromitem FLATTENOP LP eachexpr_list RP.

eachexpr ::= expr.
eachexpr ::= expr AS ID.
eachexpr_list ::= eachexpr.
eachexpr_list ::= eachexpr_list COMMA eachexpr.

%type groupby_opt {GroupByHaving}
groupby_opt ::= .
groupby_opt ::= GROUP BY exprlist.
groupby_opt ::= GROUP BY exprlist HAVING expr.

%type orderby_opt {ExprList*}
orderby_opt ::= .
orderby_opt ::= ORDER BY sortlist.
sortlist ::= sortlist COMMA sortitem sortorder.
sortlist ::= sortitem sortorder.
sortitem ::= expr.
sortorder ::= ASCENDING.
sortorder ::= DESCENDING.
sortorder ::= .

%type limit_opt {LimitOffset}
limit_opt ::= .
limit_opt ::= LIMIT expr.
limit_opt ::= LIMIT expr OFFSET expr. 

%type where_opt {Expr*}
where_opt ::= .
where_opt ::= WHERE expr.

///////////////////// TRANSACTIONS ////////////////////////////
//
cmd ::= BEGIN ID.
cmd ::= ROLLBACK ID.
cmd ::= COMMIT ID.

///////////////////// The CREATE TABLE statement ////////////////////////////
//
cmd ::= CREATE TABLE ifnotexists tabname.
ifnotexists ::= .
ifnotexists ::= IF NOT EXISTS.
tabname ::= ID.
tabname ::= ID DOT ID.

////////////////////////// The DROP TABLE /////////////////////////////////////
//
cmd ::= DROP TABLE ifexists tabname.
ifexists ::= IF EXISTS.
ifexists ::= .


/////////////////////////// The DELETE statement /////////////////////////////
//
cmd ::= DELETE FROM tabname where_opt.

////////////////////////// The UPDATE command ////////////////////////////////
//
cmd ::= UPDATE tabname SET setlist where_opt.

setlist ::= setlist COMMA setitem.
setlist ::= setitem.
setitem ::= lvalue EQ jexpr.

////////////////////////// The INSERT command /////////////////////////////////
//
cmd ::= INSERT INTO tabname VALUE jvalue.
cmd ::= INSERT INTO tabname select.
