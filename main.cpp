#include <iostream>

extern "C" {
#include "sqlite3.h"
#include "xjd1.h"
}

using namespace std;

#define IS_BOOL 1
#define IS_NUMBER 2
#define IS_TEXT 3

#define IS_ALPHA(c) (((c) >= 'A' && (c) <= 'Z') || ((c) >= 'a' && (c) <= 'z'))
#define TO_UPPER(c) ((c) & 0xDF)

const char * strstri (const char * str1, const char * str2)
{
    const char *cp = str1;
    const char *s1, *s2;

    if ( !*str2 )
        return(str1);

    while (*cp)
    {
        s1 = cp;
        s2 = str2;

        while ( *s1 && *s2 && (IS_ALPHA(*s1) && IS_ALPHA(*s2))?!(TO_UPPER(*s1) - TO_UPPER(*s2)):!(*s1-*s2))
            ++s1, ++s2;

        if (!*s2)
            return(cp);

        ++cp;
    }
    return(NULL);
}

int mysqlite3_exec(sqlite3 *db, string sql)
{
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare(db, sql.c_str(), sql.size(), &stmt, 0);
    if(rc == SQLITE_OK)
    {
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
    return rc;
}

int myxjd1_exec(xjd1 *db, string sql)
{
    xjd1_stmt *jstmt;
    int i_sql_end = 0;
    int rc = xjd1_stmt_new(db, sql.c_str(), &jstmt, &i_sql_end);
    if(rc == XJD1_OK)
    {
        xjd1_stmt_step(jstmt);
        xjd1_stmt_delete(jstmt);
    }
    return rc;
}

int main()
{
    string dbfn = "freeagent.db";
    sqlite3 *db;
    xjd1 *jdb;
    int rc;

    rc = sqlite3_open(dbfn.c_str(), &db);
    if( rc != SQLITE_OK )
    {
        cerr << "sqlite3 cannot open " << dbfn << endl;
    }
    else
    {
        rc = xjd1_open_with_db(0, db, &jdb);
        if( rc != XJD1_OK )
        {
            cerr << "xdj1 cannot open " << dbfn << endl;
        }
        else
        {
            string sql = "BEGIN;";
            rc = mysqlite3_exec(db, sql);
            if(rc == SQLITE_OK)
            {
                sql = "DROP COLLECTION myfields_metadata;";
                rc = myxjd1_exec(jdb, sql);

                sql = "CREATE COLLECTION myfields_metadata;";
                rc = myxjd1_exec(jdb, sql);

                sql = "INSERT INTO myfields_metadata VALUE {red: \"le \\\"quote\\\"\"};";
                rc = myxjd1_exec(jdb, sql);

                sqlite3_stmt *stmt;
                sql = "select * from fields_metadata;";
                sqlite3_prepare_v2(db, sql.c_str(), sql.size(), &stmt, 0);
                while(sqlite3_step(stmt) == SQLITE_ROW)
                {
                    string json = "{";
                    for(int i=0, col_count = sqlite3_column_count(stmt); i < col_count; ++i)
                    {
                        const char *declared_type = sqlite3_column_decltype(stmt, i);
                        int col_type = strstri("boolean", declared_type) ? IS_BOOL : IS_TEXT;
                        if(col_type == IS_TEXT) col_type = strstri("integer", declared_type) ? IS_NUMBER : IS_TEXT;
                        if(col_type == IS_TEXT) col_type = strstri("numeric", declared_type) ? IS_NUMBER : IS_TEXT;
                        if(col_type == IS_TEXT) col_type = strstri("float", declared_type) ? IS_NUMBER : IS_TEXT;
                        bool need_quote = col_type == IS_TEXT;

                        if(i > 0)
                        {
                            json += ",";
                        }
                        json += (const char*)sqlite3_column_name(stmt, i);
                        json += need_quote ? ":\"" : ":";
                        const char *value = (const char*)sqlite3_column_text(stmt, i);
                        json += value ? value : (need_quote ? "" : "0");
                        if(need_quote) json += "\"";
                    }
                    json += "}";
                    //cout << json << endl;

                    sql = "INSERT INTO myfields_metadata VALUE " + json;
                    rc = myxjd1_exec(jdb, sql.c_str());
                }
                sqlite3_finalize(stmt);
                sql = "COMMIT;";
                rc = mysqlite3_exec(db, sql);
            }
            xjd1_close(jdb);
        }
        sqlite3_close(db);
    }

    cout << "Hello world!" << endl;
    return 0;
}
