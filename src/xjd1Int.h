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
#include "xjd1.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

/* Marker for routines not intended for external use */
#define PRIVATE

/* Additional tokens above and beyond those generated by the parser and
** found in parse.h 
*/
#define TK_NOT_LIKEOP   (TK_LIKEOP+128)
#define TK_NOT_IS       (TK_IS+128)
#define TK_FUNCTION     100
#define TK_SPACE        101
#define TK_ILLEGAL      102
#define TK_CREATETABLE  103
#define TK_DROPTABLE    104

typedef unsigned char u8;
typedef struct Command Command;
typedef struct DataSrc DataSrc;
typedef struct Expr Expr;
typedef struct ExprItem ExprItem;
typedef struct ExprList ExprList;
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
  xjd1_stmt *pStmt;                 /* list of all prepared statements */
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
  char *zErrMsg;                    /* Error message */
};

/* A variable length string */
struct String {
  Pool *pPool;                      /* Memory allocation pool or NULL */
  char *zBuf;                       /* String content */
  int nUsed;                        /* Slots used.  Not counting final 0 */
  int nAlloc;                       /* Space allocated */
};

/* A token into to the parser */
struct Token {
  const char *z;                    /* Text of the token */
  int n;                            /* Number of characters */
};

/* A single element of an expression list */
struct ExprItem {
  Token tkAs;               /* AS value, or DESCENDING, or ASCENDING */
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
  int eType;                /* Expression node type */
  union {
    struct {                /* Binary and unary operators */
      Expr *pLeft;             /* Left operand.  Only operand for unary ops */
      Expr *pRight;            /* Right operand.  NULL for unary ops */
    } bi;
    Token tk;               /* Token values */
    struct {                /* Function calls */
      Token name;              /* Name of the function */
      ExprList *args;          /* List of argumnts */
    } func;
    Query *q;               /* Subqueries */
  } u;
};

/* Parsing context */
struct Parse {
  Pool *pPool;                    /* Memory allocation pool */
  Command *pCmd;                  /* Results */
  Token sTok;                     /* Last token seen */
  int errCode;                    /* Error code */
  String errMsg;                  /* Error message string */
};

/* A query statement */
struct Query {
  int eQType;                   /* Query type */
  union {
    struct {                    /* For compound queries */
      Query *pLeft;               /* Left subquery */
      Query *pRight;              /* Righ subquery */
    } compound;
    struct {                    /* For simple queries */
      ExprList *pCol;             /* List of result columns */
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
  Token asId;               /* The identifier after the AS keyword */
  union {
    struct {                /* For a join.  eDSType==TK_COMMA */
      DataSrc *pLeft;          /* Data source on the left */
      DataSrc *pRight;         /* Data source on the right */
    } join;
    struct {                /* For a named table.  eDSType==TK_ID */
      Token name;              /* The table name */
    } tab;
    struct {                /* EACH() or FLATTEN().  eDSType==TK_FLATTENOP */
      DataSrc *pNext;          /* Data source to the left */
      Token opName;            /* "EACH" or "FLATTEN" */
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
      Token id;                /* Transaction name */
    } trans;
    struct {                /* Create or drop table */
      int ifExists;            /* IF [NOT] EXISTS clause */
      Token name;              /* Name of table */
    } crtab;
    struct {                /* Query statement */
      Query *pQuery;           /* The query */
    } q;
    struct {                /* Insert */
      Token name;              /* Table to insert into */
      Token jvalue;            /* Value to be inserted */
      Query *pQuery;           /* Query to insert from */
    } ins;
    struct {                /* Delete */
      Token name;              /* Table to delete */
      Expr *pWhere;            /* WHERE clause */
    } del;
    struct {                /* Update */
      Token name;              /* Table to modify */
      Expr *pWhere;            /* WHERE clause */
      ExprList *pChng;         /* Alternating lvalve and new value */
    } update;
  } u;
};



/******************************** context.c **********************************/
void xjd1ContextUnref(xjd1_context*);

/******************************** conn.c *************************************/
void xjd1Unref(xjd1*);

/******************************** malloc.c ***********************************/
Pool *xjd1PoolNew(void);
void xjd1PoolClear(Pool*);
void xjd1PoolDelete(Pool*);
void *xjd1PoolMalloc(Pool*, int);
void *xjd1PoolMallocZero(Pool*, int);
char *xjd1PoolDup(Pool*, const char *, int);

/******************************** string.c ***********************************/
int xjd1Strlen30(const char *);
void xjd1StringInit(String*, Pool*, int);
String *xjd1StringNew(Pool*, int);
int xjd1StringAppend(String*, const char*, int);
#define xjd1StringText(S)      ((S)->zBuf)
#define xjd1StringLen(S)       ((S)->nUsed)
#define xjd1StringTruncate(S)  ((S)->nUsed=0)
void xjd1StringClear(String*);
void xjd1StringDelete(String*);
int xjd1StringVAppendF(String*, const char*, va_list);
int xjd1StringAppendF(String*, const char*, ...);


/******************************** tokenize.c *********************************/
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
