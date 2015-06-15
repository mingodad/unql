# unql
 A mirror of http://unql.sqlite.org with some fixes

I did changed the string parsing tokenization to use the '\' escape sequence 
instead of the previous double quote.

Added a new function to allow open the xdj1 database with an already opened 
sqlite3 handler (see main.cpp example).

Added an extra parameter and keyword ILIKE to allow case insensitive string comparisons.

Add code to detect if there is a transaction already in place before try to 
create one.

Comment out an assert that seems to be wrong (it terminates the program when we 
have more than one prepared staments and try delete the first one).

Updated the lemon.c, lempar.c, sqlite3.c and sqlite3.h to the latest.

The sqlite3.c here contains several changes over the original 
(maybe you'll not want it).

On my tests over a small database with 330000 records/json objects with 6 fields 
it takes around 1.2 seconds to read all and around 4 seconds to search on a string 
comparison on one field (no idexes sequential scan).

