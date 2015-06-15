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
*****************************************************************************
** Compile and run this standalone program in order to generate code that
** implements a function that will translate alphabetic identifiers into
** parser token codes.
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

/*
** A header comment placed at the beginning of generated code.
*/
static const char zHdr[] = 
  "/**********************************************************************\n"
  "** The following code is automatically generated\n"
  "** by ../tool/mkkeywordhash.c\n"
  "*/\n"
;

/*
** All the keywords of the SQL language are stored in a hash
** table composed of instances of the following structure.
*/
typedef struct Keyword Keyword;
struct Keyword {
  char *zName;         /* The keyword name */
  char *zTokenType;    /* Token value for this keyword */
  int id;              /* Unique ID for this record */
  int hash;            /* Hash on the keyword */
  int offset;          /* Offset to start of name string */
  int len;             /* Length of this keyword, not counting final \000 */
  int prefix;          /* Number of characters in prefix */
  int longestSuffix;   /* Longest suffix that is a prefix on another word */
  int iNext;           /* Index in aKeywordTable[] of next with same hash */
  int substrId;        /* Id to another keyword this keyword is embedded in */
  int substrOffset;    /* Offset into substrId for start of this keyword */
  char zOrigName[20];  /* Original keyword name before processing */
};
 
/*
** These are the keywords
*/
static Keyword aKeywordTable[] = {
  { "ALL",          "TK_ALL",        },
  { "ASCENDING",    "TK_ASCENDING",  },
  { "ASC",          "TK_ASCENDING",  },
  { "AS",           "TK_AS",         },
  { "ASYNCHRONOUS", "TK_ASYNC",      },
  { "ASYNC",        "TK_ASYNC",      },
  { "BEGIN",        "TK_BEGIN",      },
  { "BY",           "TK_BY",         },
  { "COLLATE",      "TK_COLLATE",    },
  { "COLLECTION",   "TK_COLLECTION", },
  { "COMMIT",       "TK_COMMIT",     },
  { "CREATE",       "TK_CREATE",     },
  { "DISTINCT",     "TK_DISTINCT",   },
  { "DELETE",       "TK_DELETE",     },
  { "DESCENDING",   "TK_DESCENDING", },
  { "DESC",         "TK_DESCENDING", },
  { "DROP",         "TK_DROP",       },
  { "EACH",         "TK_FLATTENOP",  },
  { "ELSE",         "TK_ELSE",       },
  { "EXCEPT",       "TK_EXCEPT",     },
  { "EXISTS",       "TK_EXISTS",     },
  { "false",        "TK_FALSE",      },
  { "FLATTEN",      "TK_FLATTENOP",  },
  { "FROM",         "TK_FROM",       },
  { "GLOB",         "TK_LIKEOP",     },
  { "GROUP",        "TK_GROUP",      },
  { "HAVING",       "TK_HAVING",     },
  { "IF",           "TK_IF",         },
  { "INSERT",       "TK_INSERT",     },
  { "INTERSECT",    "TK_INTERSECT",  },
  { "in",           "TK_IN",         },
  { "INTO",         "TK_INTO",       },
  { "ILIKE",         "TK_ILIKEOP",     },
  { "LIKE",         "TK_LIKEOP",     },
  { "LIMIT",        "TK_LIMIT",      },
  { "NOT",          "TK_NOT",        },
  { "NULL",         "TK_NULL",       },
  { "null",         "TK_NULL",       },
  { "OFFSET",       "TK_OFFSET",     },
  { "ORDER",        "TK_ORDER",      },
  { "PRAGMA",       "TK_PRAGMA",     },
  { "ROLLBACK",     "TK_ROLLBACK",   },
  { "SELECT",       "TK_SELECT",     },
  { "SET",          "TK_SET",        },
  { "SYNC",         "TK_SYNC",       },
  { "SYNCHRONOUS",  "TK_SYNC",       },
  { "true",         "TK_TRUE",       },
  { "UNION",        "TK_UNION",      },
  { "UPDATE",       "TK_UPDATE",     },
  { "VALUE",        "TK_VALUE",      },
  { "WHERE",        "TK_WHERE",      },
  { "WITHIN",       "TK_WITHIN",     },
};

/* Number of keywords */
static int nKeyword = (sizeof(aKeywordTable)/sizeof(aKeywordTable[0]));

/*
** Comparision function for two Keyword records
*/
static int keywordCompare1(const void *a, const void *b){
  const Keyword *pA = (Keyword*)a;
  const Keyword *pB = (Keyword*)b;
  int n = pA->len - pB->len;
  if( n==0 ){
    n = strcmp(pA->zName, pB->zName);
  }
  assert( n!=0 );
  return n;
}
static int keywordCompare2(const void *a, const void *b){
  const Keyword *pA = (Keyword*)a;
  const Keyword *pB = (Keyword*)b;
  int n = pB->longestSuffix - pA->longestSuffix;
  if( n==0 ){
    n = strcmp(pA->zName, pB->zName);
  }
  assert( n!=0 );
  return n;
}
static int keywordCompare3(const void *a, const void *b){
  const Keyword *pA = (Keyword*)a;
  const Keyword *pB = (Keyword*)b;
  int n = pA->offset - pB->offset;
  if( n==0 ) n = pB->id - pA->id;
  assert( n!=0 );
  return n;
}

/*
** Return a KeywordTable entry with the given id
*/
static Keyword *findById(int id){
  int i;
  for(i=0; i<nKeyword; i++){
    if( aKeywordTable[i].id==id ) break;
  }
  return &aKeywordTable[i];
}

/*
** This routine does the work.  The generated code is printed on standard
** output.
*/
int main(int argc, char **argv){
  int i, j, k, h;
  int bestSize, bestCount;
  int count;
  int nChar;
  int totalLen = 0;
  int aHash[1000];  /* 1000 is much bigger than nKeyword */
  char zText[2000];

  /* Fill in the lengths of strings and hashes for all entries. */
  for(i=0; i<nKeyword; i++){
    Keyword *p = &aKeywordTable[i];
    p->len = strlen(p->zName);
    assert( p->len<sizeof(p->zOrigName) );
    strcpy(p->zOrigName, p->zName);
    totalLen += p->len;
    p->hash = (p->zName[0]*4) ^ p->zName[p->len-1]*3 ^ p->len;
    p->id = i+1;
  }

  /* Sort the table from shortest to longest keyword */
  qsort(aKeywordTable, nKeyword, sizeof(aKeywordTable[0]), keywordCompare1);

  /* Look for short keywords embedded in longer keywords */
  for(i=nKeyword-2; i>=0; i--){
    Keyword *p = &aKeywordTable[i];
    for(j=nKeyword-1; j>i && p->substrId==0; j--){
      Keyword *pOther = &aKeywordTable[j];
      if( pOther->substrId ) continue;
      if( pOther->len<=p->len ) continue;
      for(k=0; k<=pOther->len-p->len; k++){
        if( memcmp(p->zName, &pOther->zName[k], p->len)==0 ){
          p->substrId = pOther->id;
          p->substrOffset = k;
          break;
        }
      }
    }
  }

  /* Compute the longestSuffix value for every word */
  for(i=0; i<nKeyword; i++){
    Keyword *p = &aKeywordTable[i];
    if( p->substrId ) continue;
    for(j=0; j<nKeyword; j++){
      Keyword *pOther;
      if( j==i ) continue;
      pOther = &aKeywordTable[j];
      if( pOther->substrId ) continue;
      for(k=p->longestSuffix+1; k<p->len && k<pOther->len; k++){
        if( memcmp(&p->zName[p->len-k], pOther->zName, k)==0 ){
          p->longestSuffix = k;
        }
      }
    }
  }

  /* Sort the table into reverse order by length */
  qsort(aKeywordTable, nKeyword, sizeof(aKeywordTable[0]), keywordCompare2);

  /* Fill in the offset for all entries */
  nChar = 0;
  for(i=0; i<nKeyword; i++){
    Keyword *p = &aKeywordTable[i];
    if( p->offset>0 || p->substrId ) continue;
    p->offset = nChar;
    nChar += p->len;
    for(k=p->len-1; k>=1; k--){
      for(j=i+1; j<nKeyword; j++){
        Keyword *pOther = &aKeywordTable[j];
        if( pOther->offset>0 || pOther->substrId ) continue;
        if( pOther->len<=k ) continue;
        if( memcmp(&p->zName[p->len-k], pOther->zName, k)==0 ){
          p = pOther;
          p->offset = nChar - k;
          nChar = p->offset + p->len;
          p->zName += k;
          p->len -= k;
          p->prefix = k;
          j = i;
          k = p->len;
        }
      }
    }
  }
  for(i=0; i<nKeyword; i++){
    Keyword *p = &aKeywordTable[i];
    if( p->substrId ){
      p->offset = findById(p->substrId)->offset + p->substrOffset;
    }
  }

  /* Sort the table by offset */
  qsort(aKeywordTable, nKeyword, sizeof(aKeywordTable[0]), keywordCompare3);

  /* Figure out how big to make the hash table in order to minimize the
  ** number of collisions */
  bestSize = nKeyword;
  bestCount = nKeyword*nKeyword;
  for(i=nKeyword/2; i<=2*nKeyword; i++){
    for(j=0; j<i; j++) aHash[j] = 0;
    for(j=0; j<nKeyword; j++){
      h = aKeywordTable[j].hash % i;
      aHash[h] *= 2;
      aHash[h]++;
    }
    for(j=count=0; j<i; j++) count += aHash[j];
    if( count<bestCount ){
      bestCount = count;
      bestSize = i;
    }
  }

  /* Compute the hash */
  for(i=0; i<bestSize; i++) aHash[i] = 0;
  for(i=0; i<nKeyword; i++){
    h = aKeywordTable[i].hash % bestSize;
    aKeywordTable[i].iNext = aHash[h];
    aHash[h] = i+1;
  }

  /* Begin generating code */
  printf("%s", zHdr);
  printf("/* Hash score: %d */\n", bestCount);
  printf("static int keywordCode(const char *z, int n){\n");
  printf("  /* zText[] encodes %d bytes of keywords in %d bytes */\n",
          totalLen + nKeyword, nChar+1 );
  for(i=j=k=0; i<nKeyword; i++){
    Keyword *p = &aKeywordTable[i];
    if( p->substrId ) continue;
    memcpy(&zText[k], p->zName, p->len);
    k += p->len;
    if( j+p->len>70 ){
      printf("%*s */\n", 74-j, "");
      j = 0;
    }
    if( j==0 ){
      printf("  /*   ");
      j = 8;
    }
    printf("%s", p->zName);
    j += p->len;
  }
  if( j>0 ){
    printf("%*s */\n", 74-j, "");
  }
  printf("  static const char zText[%d] = {\n", nChar);
  zText[nChar] = 0;
  for(i=j=0; i<k; i++){
    if( j==0 ){
      printf("    ");
    }
    if( zText[i]==0 ){
      printf("0");
    }else{
      printf("'%c',", zText[i]);
    }
    j += 4;
    if( j>68 ){
      printf("\n");
      j = 0;
    }
  }
  if( j>0 ) printf("\n");
  printf("  };\n");

  printf("  static const unsigned char aHash[%d] = {\n", bestSize);
  for(i=j=0; i<bestSize; i++){
    if( j==0 ) printf("    ");
    printf(" %3d,", aHash[i]);
    j++;
    if( j>12 ){
      printf("\n");
      j = 0;
    }
  }
  printf("%s  };\n", j==0 ? "" : "\n");    

  printf("  static const unsigned char aNext[%d] = {\n", nKeyword);
  for(i=j=0; i<nKeyword; i++){
    if( j==0 ) printf("    ");
    printf(" %3d,", aKeywordTable[i].iNext);
    j++;
    if( j>12 ){
      printf("\n");
      j = 0;
    }
  }
  printf("%s  };\n", j==0 ? "" : "\n");    

  printf("  static const unsigned char aLen[%d] = {\n", nKeyword);
  for(i=j=0; i<nKeyword; i++){
    if( j==0 ) printf("    ");
    printf(" %3d,", aKeywordTable[i].len+aKeywordTable[i].prefix);
    j++;
    if( j>12 ){
      printf("\n");
      j = 0;
    }
  }
  printf("%s  };\n", j==0 ? "" : "\n");    

  printf("  static const unsigned short int aOffset[%d] = {\n", nKeyword);
  for(i=j=0; i<nKeyword; i++){
    if( j==0 ) printf("    ");
    printf(" %3d,", aKeywordTable[i].offset);
    j++;
    if( j>12 ){
      printf("\n");
      j = 0;
    }
  }
  printf("%s  };\n", j==0 ? "" : "\n");

  printf("  static const unsigned char aCode[%d] = {\n", nKeyword);
  for(i=j=0; i<nKeyword; i++){
    char *zToken = aKeywordTable[i].zTokenType;
    if( j==0 ) printf("    ");
    printf("%s,%*s", zToken, (int)(14-strlen(zToken)), "");
    j++;
    if( j>=5 ){
      printf("\n");
      j = 0;
    }
  }
  printf("%s  };\n", j==0 ? "" : "\n");

  printf("  int h, i;\n");
  printf("  if( n<2 ) return TK_ID;\n");
  printf("  h = (z[0]*4 ^ z[n-1]*3 ^ n) %% %d;\n", bestSize);
  printf("  for(i=((int)aHash[h])-1; i>=0; i=((int)aNext[i])-1){\n");
  printf("    if( aLen[i]==n && memcmp(&zText[aOffset[i]],z,n)==0 ){\n");
  printf("      return aCode[i];\n");
  printf("    }\n");
  printf("  }\n");
  printf("  return TK_ID;\n");
  printf("}\n");
  printf("#define XJD1_N_KEYWORD %d\n", nKeyword);
  printf(
    "\n"
    "/* End of the automatically generated hash code\n"
    "*********************************************************************/\n");

  return 0;
}
