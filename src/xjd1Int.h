/*
** Copyright (c) 2011 D. Richard Hipp
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the Simplified BSD License (also
** known as the "2-Clause License" or "FreeBSD License".)
**
** This program is distributed in the hope that it will be useful,
** but without any warranty; without even the implied warranty of
** merchantability or fitness for a particular purpose.
**
** Author contact information:
**   drh@hwaci.com
**   http://www.hwaci.com/drh/
**
*************************************************************************
** Internal definitions for XJD1
*/
#ifndef _XJD1INT_H
#define _XJD1INT_H

#include "xjd1.h"
#include "parse.h"
#include "sqlite3.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

/* Marker for routines not intended for external use */
#define PRIVATE

/* Additional tokens above and beyond those generated by the parser and
** found in parse.h 
*/
#define TK_NOT_LIKEOP        (TK_LIKEOP+128)
#define TK_NOT_IS            (TK_IS+128)
#define TK_FUNCTION          100
#define TK_SPACE             101
#define TK_ILLEGAL           102
#define TK_CREATECOLLECTION  103
#define TK_DROPCOLLECTION    104
#define TK_ARRAY             105
#define TK_STRUCT            106
#define TK_JVALUE            107

typedef unsigned char u8;
typedef unsigned short int u16;
typedef struct Command Command;
typedef struct DataSrc DataSrc;
typedef struct Expr Expr;
typedef struct ExprItem ExprItem;
typedef struct ExprList ExprList;
typedef struct JsonNode JsonNode;
typedef struct JsonStructElem JsonStructElem;
typedef struct Parse Parse;
typedef struct PoolChunk PoolChunk;
typedef struct Pool Pool;
typedef struct Query Query;
typedef struct String String;
typedef struct Token Token;

/* A single allocation from the Pool allocator */
struct PoolChunk {
  PoolChunk *pNext;                 /* Next chunk on list of them all */
};

/* A memory allocation pool */
struct Pool {
  PoolChunk *pChunk;                /* List of all memory allocations */
  char *pSpace;                     /* Space available for allocation */
  int nSpace;                       /* Bytes available in pSpace */
};

/* A variable length string */
struct String {
  Pool *pPool;                      /* Memory allocation pool or NULL */
  char *zBuf;                       /* String content */
  int nUsed;                        /* Slots used.  Not counting final 0 */
  int nAlloc;                       /* Space allocated */
};

/* Execution context */
struct xjd1_context {
  int nRef;                         /* Reference count */
  u8 isDying;                       /* True if has been deleted */
  int (*xLog)(const char*,void*);   /* Error logging function */
  void *pLogArg;                    /* 2nd argument to xLog() */
};

/* An open database connection */
struct xjd1 {
  xjd1_context *pContext;           /* Execution context */
  int nRef;                         /* Reference count */
  u8 isDying;                       /* True if has been closed */
  u8 parserTrace;                   /* True to enable parser tracing */
  u8 appendErr;                     /* append errMsg rather than overwrite */
  xjd1_stmt *pStmt;                 /* list of all prepared statements */
  sqlite3 *db;                      /* Storage engine */
  int errCode;                      /* Latest non-zero error code */
  String errMsg;                    /* Latest error message */
};

/* A prepared statement */
struct xjd1_stmt {
  xjd1 *pConn;                      /* Database connection */
  xjd1_stmt *pNext, *pPrev;         /* List of all statements */
  Pool sPool;                       /* Memory pool used for parsing */
  int nRef;                         /* Reference count */
  u8 isDying;                       /* True if has been closed */
  char *zCode;                      /* Text of the query */
  Command *pCmd;                    /* Parsed command */
  int okValue;                      /* True if retValue is valid */
  String retValue;                  /* String rendering of return value */
  char *zErrMsg;                    /* Error message */
};

/* A token into to the parser */
struct Token {
  const char *z;                    /* Text of the token */
  int n;                            /* Number of characters */
};

/* A single element of an expression list */
struct ExprItem {
  char *zAs;                /* AS value, or DESCENDING, or ASCENDING */
  Expr *pExpr;              /* The expression */
};

/* A list of expressions */
struct ExprList {
  int nEItem;               /* Number of items on the expression list */
  int nEAlloc;              /* Slots allocated in apEItem[] */
  ExprItem *apEItem;        /* The expression in the list */
};

/* A node of an expression */
struct Expr {
  u16 eType;                /* Expression node type */
  u16 eClass;               /* Expression class */
  xjd1_stmt *pStmt;         /* Statement this expression belongs to */
  union {
    struct {                /* Binary or unary operator. eClass==XJD1_EXPR_BI */
      Expr *pLeft;             /* Left operand.  Only operand for unary ops */
      Expr *pRight;            /* Right operand.  NULL for unary ops */
    } bi;
    struct {                /* Substructure nam.  eClass==EXPR_LVALUE */
      Expr *pLeft;             /* Lvalue or id to the left */
      char *zId;               /* ID to the right */
    } lvalue;
    struct {                /* Identifiers */
      char *zId;               /* token value.  eClass=EXPR_TK */
    } id;
    struct {                /* Function calls.  eClass=EXPR_FUNC */
      char *zFName;            /* Name of the function */
      ExprList *args;          /* List of argumnts */
    } func;
    struct {                /* Subqueries.  eClass=EXPR_Q */
      Query *p;                /* The subquery */
    } subq;
    struct {                /* Literal value.  eClass=EXPR_JSON */
      JsonNode *p;             /* The value */
    } json;
    ExprList *ar;           /* Array literal.  eClass=EXPR_ARRAY */
    ExprList *st;           /* Struct literal.  eClass=EXPR_STRUCT */
  } u;
};
#define XJD1_EXPR_BI      1
#define XJD1_EXPR_TK      2
#define XJD1_EXPR_FUNC    3
#define XJD1_EXPR_Q       4
#define XJD1_EXPR_JSON    5
#define XJD1_EXPR_ARRAY   6
#define XJD1_EXPR_STRUCT  7
#define XJD1_EXPR_LVALUE  8

/* A single element of a JSON structure */
struct JsonStructElem {
  char *zLabel;             /* Label on this element */
  JsonStructElem *pNext;    /* Next element of the structure */
  JsonNode *pValue;         /* Value of this element */
};

/* A single element of a JSON value */
struct JsonNode {
  int eJType;               /* Element type */
  int nRef;                 /* Number of references */
  union {
    int b;                  /* Boolean value */
    double r;               /* Real value */
    char *z;                /* String value */
    struct {                /* Array value */
      int nElem;               /* Number of elements */
      JsonNode **apElem;       /* Value of each element */
    } ar;
    struct {                /* Struct value */
      JsonStructElem *pFirst;  /* List of structure elements */
      JsonStructElem *pLast;   /* Last element of the list */
    } st;
  } u;
};

/* Values for eJType */
#define XJD1_FALSE     0
#define XJD1_TRUE      1
#define XJD1_REAL      2
#define XJD1_NULL      3
#define XJD1_STRING    4
#define XJD1_ARRAY     5
#define XJD1_STRUCT    6

/* Parsing context */
struct Parse {
  xjd1 *pConn;                    /* Connect for recording errors */
  Pool *pPool;                    /* Memory allocation pool */
  Command *pCmd;                  /* Results */
  Token sTok;                     /* Last token seen */
  int errCode;                    /* Error code */
  String errMsg;                  /* Error message string */
};

/* A query statement */
struct Query {
  int eQType;                   /* Query type */
  xjd1_stmt *pStmt;             /* Statement this query is part of */
  Query *pOuter;                /* Next outer query for a subquery */
  union {
    struct {                    /* For compound queries */
      Query *pLeft;               /* Left subquery */
      Query *pRight;              /* Righ subquery */
      int doneLeft;               /* True if left is run to completion */
    } compound;
    struct {                    /* For simple queries */
      Expr *pRes;                 /* Result JSON string */
      DataSrc *pFrom;             /* The FROM clause */
      Expr *pWhere;               /* The WHERE clause */
      ExprList *pGroupBy;         /* The GROUP BY clause */
      Expr *pHaving;              /* The HAVING clause */
      ExprList *pOrderBy;         /* The ORDER BY clause */
      Expr *pLimit;               /* The LIMIT clause */
      Expr *pOffset;              /* The OFFSET clause */
    } simple;
  } u;
};

/* A Data Source is a representation of a term out of the FROM clause. */
struct DataSrc {
  int eDSType;              /* Source type */
  char *zAs;                /* The identifier after the AS keyword */
  Query *pQuery;            /* Query this data source services */
  JsonNode *pValue;         /* Current value for this data source */
  int isOwner;              /* True if this DataSrc owns the pOut line */
  union {
    struct {                /* For a join.  eDSType==TK_COMMA */
      DataSrc *pLeft;          /* Data source on the left */
      DataSrc *pRight;         /* Data source on the right */
    } join;
    struct {                /* For a named collection.  eDSType==TK_ID */
      char *zName;             /* The collection name */
      sqlite3_stmt *pStmt;     /* Cursor for reading content */
      int eofSeen;             /* True if at EOF */
    } tab;
    struct {                /* EACH() or FLATTEN().  eDSType==TK_FLATTENOP */
      DataSrc *pNext;          /* Data source to the left */
      char cOpName;            /* E or F for "EACH" or "FLATTEN" */
      ExprList *pList;         /* List of arguments */
    } flatten;
    struct {                /* A subquery.  eDSType==TK_SELECT */
      Query *q;                /* The subquery */
    } subq;
  } u;
};

/* Any command, including but not limited to a query */
struct Command {
  int eCmdType;             /* Type of command */
  union {
    struct {                /* Transaction control operations */
      char *zTransId;          /* Transaction name */
    } trans;
    struct {                /* Create or drop table */
      int ifExists;            /* IF [NOT] EXISTS clause */
      char *zName;             /* Name of table */
    } crtab;
    struct {                /* Query statement */
      Query *pQuery;           /* The query */
    } q;
    struct {                /* Insert */
      char *zName;             /* Table to insert into */
      Expr *pValue;            /* Value to be inserted */
      Query *pQuery;           /* Query to insert from */
    } ins;
    struct {                /* Delete */
      char *zName;             /* Table to delete */
      Expr *pWhere;            /* WHERE clause */
    } del;
    struct {                /* Update */
      char *zName;             /* Table to modify */
      Expr *pWhere;            /* WHERE clause */
      ExprList *pChng;         /* Alternating lvalve and new value */
    } update;
    struct {                /* Pragma */
      char *zName;             /* Pragma name */
      Expr *pValue;            /* Argument or empty string */
    } prag;
  } u;
};

/******************************** context.c **********************************/
void xjd1ContextUnref(xjd1_context*);

/******************************** conn.c *************************************/
void xjd1Unref(xjd1*);
void xjd1Error(xjd1*,int,const char*,...);

/******************************** datasrc.c **********************************/
int xjd1DataSrcInit(DataSrc*,Query*);
int xjd1DataSrcRewind(DataSrc*);
int xjd1DataSrcStep(DataSrc*);
int xjd1DataSrcEOF(DataSrc*);
int xjd1DataSrcClose(DataSrc*);
JsonNode *xjd1DataSrcDoc(DataSrc*, const char*);

/******************************** expr.c *************************************/
int xjd1ExprInit(Expr*, xjd1_stmt*, Query*);
int xjd1ExprListInit(ExprList*, xjd1_stmt*, Query*);
JsonNode *xjd1ExprEval(Expr*);
int xjd1ExprTrue(Expr*);
int xjd1ExprClose(Expr*);
int xjd1ExprListClose(ExprList*);

/******************************** json.c *************************************/
JsonNode *xjd1JsonParse(const char *zIn, int mxIn);
JsonNode *xjd1JsonRef(JsonNode*);
void xjd1JsonRender(String*, JsonNode*);
JsonNode *xjd1JsonNew(Pool*);
JsonNode *xjd1JsonEdit(JsonNode*);
void xjd1JsonFree(JsonNode*);
void xjd1DequoteString(char*,int);

/******************************** malloc.c ***********************************/
Pool *xjd1PoolNew(void);
void xjd1PoolClear(Pool*);
void xjd1PoolDelete(Pool*);
void *xjd1PoolMalloc(Pool*, int);
void *xjd1PoolMallocZero(Pool*, int);
char *xjd1PoolDup(Pool*, const char *, int);

/******************************** pragma.c ***********************************/
int xjd1PragmaStep(xjd1_stmt*);

/******************************** query.c ************************************/
int xjd1QueryInit(Query*,xjd1_stmt*,Query*);
int xjd1QueryRewind(Query*);
int xjd1QueryStep(Query*);
int xjd1QueryEOF(Query*);
int xjd1QueryClose(Query*);
JsonNode *xjd1QueryDoc(Query*, const char*);

/******************************** stmt.c *************************************/
JsonNode *xjd1StmtDoc(xjd1_stmt*, const char*);

/******************************** string.c ***********************************/
int xjd1Strlen30(const char *);
void xjd1StringInit(String*, Pool*, int);
String *xjd1StringNew(Pool*, int);
int xjd1StringAppend(String*, const char*, int);
#define xjd1StringText(S)      ((S)->zBuf)
#define xjd1StringLen(S)       ((S)->nUsed)
void xjd1StringTruncate(String*);
void xjd1StringClear(String*);
void xjd1StringDelete(String*);
void xjd1StringRemovePrefix(String*,int);
int xjd1StringVAppendF(String*, const char*, va_list);
int xjd1StringAppendF(String*, const char*, ...);


/******************************** tokenize.c *********************************/
extern const unsigned char xjd1CtypeMap[];
#define xjd1Isspace(x)   (xjd1CtypeMap[(unsigned char)(x)]&0x01)
#define xjd1Isalnum(x)   (xjd1CtypeMap[(unsigned char)(x)]&0x06)
#define xjd1Isalpha(x)   (xjd1CtypeMap[(unsigned char)(x)]&0x02)
#define xjd1Isdigit(x)   (xjd1CtypeMap[(unsigned char)(x)]&0x04)
#define xjd1Isxdigit(x)  (xjd1CtypeMap[(unsigned char)(x)]&0x08)
#define xjd1Isident(x)   (xjd1CtypeMap[(unsigned char)(x)]&0x46)
int xjd1RunParser(xjd1*, xjd1_stmt*, const char*, int*);

/******************************** trace.c ************************************/
const char *xjd1TokenName(int);
void xjd1TraceCommand(String*,int,const Command*);
void xjd1TraceQuery(String*,int,const Query*);
void xjd1TraceDataSrc(String*,int,const DataSrc*);
void xjd1TraceExpr(String*,const Expr*);
void xjd1TraceExprList(String*,int, const ExprList*);

#endif /* _XJD1INT_H */
