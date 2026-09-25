#include <stdio.h>
#include <string.h>
#include <sqlite3.h>

int main(void)
{
    sqlite3 *db;
    sqlite3_stmt *stmt;
    int ok;

    if (strcmp(sqlite3_libversion(), SQLITE_VERSION) != 0) {
        fprintf(stderr, "header %s != library %s\n", SQLITE_VERSION, sqlite3_libversion());
        return 1;
    }

    /* ceil() comes from libm, so this also exercises the math functions */
    if (sqlite3_open(":memory:", &db) != SQLITE_OK
        || sqlite3_prepare_v2(db, "SELECT ceil(1.5)", -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "%s\n", sqlite3_errmsg(db));
        return 1;
    }

    ok = sqlite3_step(stmt) == SQLITE_ROW && sqlite3_column_int(stmt, 0) == 2;
    sqlite3_finalize(stmt);
    sqlite3_close(db);

    printf("SQLite %s, threadsafe=%d: %s\n", sqlite3_libversion(), sqlite3_threadsafe(), ok ? "ok" : "FAILED");
    return ok ? 0 : 1;
}
