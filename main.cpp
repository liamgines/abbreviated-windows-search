#include "mftreader.hpp"
#include "sqlite3.h"

#define SQLITE_MAX_SQL_LENGTH (1000000000)
#define SQL_SIZE (SQLITE_MAX_SQL_LENGTH / 1000)

uint64_t GetPathLength(File *files, uint64_t i) {
    uint64_t pathLength = 0;

    File file = files[i];
    while (file.name != NULL) {
        pathLength += strlen(file.name);    // + file name

        if (i == file.parent) {
            break;
        }

        i = file.parent;
        file = files[i];

        pathLength += 1;    // + slash
    }

    return pathLength;
}

char *GetFilePath(File *files, uint64_t i) {
    uint64_t charactersRemaining = GetPathLength(files, i) + 1;   // + NULL character

    File file = files[i];
    if (charactersRemaining < 2) {
        return nullptr;
    }

    char *filePath = (char *) malloc(charactersRemaining * sizeof(char));
    filePath[--charactersRemaining] = 0;

    while (file.name != NULL) {
        charactersRemaining -= strlen(file.name);
        memcpy(&filePath[charactersRemaining], file.name, strlen(file.name));

        if (i == file.parent) {
            break;
        }

        i = file.parent;
        file = files[i];

        filePath[--charactersRemaining] = '\\';
    }

    return filePath;
}

char *GetPath(File *files, uint64_t i) {
    File file = files[i];
    return GetFilePath(files, file.parent);
}

int GetFileValueLength(File *files, uint64_t i) {
    File file = files[i];
    if (!file.name || !file.path) {
        return 0;
    }
    return 1 + (1 + strlen(file.name) + 1) + 2 + (1 + strlen(file.path) + 1) + 1; // ("name", "path")
}

void Ok(int sqliteResultCode) {
    assert(sqliteResultCode == SQLITE_OK);
}

static int PrintSearchResult(void *NotUsed, int argc, char **argv, char **azColName) {
    for (int i = 0; i < argc; i++) {
        fprintf(stderr, "%s\n", argv[i]);
    }
    return 0;
}

int main() {
    File *files = GetFiles();

    fprintf(stderr, "\nCollecting file paths...\n");

    for (uint64_t i = 0; i < arrlen(files); i++) {
        files[i].path = GetPath(files, i);  // not a deep copy
    }

    sqlite3 *database;

    CreateDirectory("C:\\Search", NULL);
    DeleteFile("C:\\Search\\Search.db");
    Ok(sqlite3_open("C:\\Search\\Search.db", &database));
    char *errorMessage;

    char sqlCreateFiles[] = "CREATE TABLE IF NOT EXISTS FILES("
                            "NAME TEXT, "
                            "PATH TEXT"
                            ");";
    fprintf(stderr, "\n%s\n\n", sqlCreateFiles);

    if (sqlite3_exec(database, sqlCreateFiles, NULL, 0, &errorMessage) != SQLITE_OK) {
        sqlite3_free(errorMessage);
        exit(1);
    }

    char sqlInsertFilesHeader[] = "INSERT INTO FILES (NAME, PATH) "
                                  "VALUES ";

    char *sql = (char *) malloc(SQL_SIZE * sizeof(char));
    memcpy(sql, sqlInsertFilesHeader, strlen(sqlInsertFilesHeader));
    int sqlLength = strlen(sqlInsertFilesHeader);
    int valueLength;

    for (uint64_t i = 0; i < arrlen(files); i++) {
        File file = files[i];

        valueLength = GetFileValueLength(files, i) + 1;
        if (valueLength < 2) {
            continue;
        }

        if (sqlLength + valueLength + 1 > SQL_SIZE) {
            sql[--sqlLength] = 0;
            sql[--sqlLength] = ';';

            fprintf(stderr, "%s(%d characters)\n", sqlInsertFilesHeader, sqlLength);   // insert files
            if (sqlite3_exec(database, sql, NULL, 0, &errorMessage) != SQLITE_OK) {
                sqlite3_free(errorMessage);
                exit(1);
            }
            sqlLength = strlen(sqlInsertFilesHeader);
        }

        else {
            char *value = (char *) malloc(valueLength * sizeof(char));
            snprintf(value, valueLength, "(\"%s\", \"%s\")", file.name, file.path);
            memcpy(&sql[sqlLength], value, strlen(value));

            sqlLength += strlen(value);
            free(value);

            sql[sqlLength] = ',';
            sql[++sqlLength] = ' ';
            sqlLength += 1;
        }
    }

    if (sqlLength > strlen(sqlInsertFilesHeader)) {
        sql[--sqlLength] = 0;
        sql[--sqlLength] = ';';

        fprintf(stderr, "%s(%d characters)\n", sqlInsertFilesHeader, sqlLength);   // insert files
        if (sqlite3_exec(database, sql, NULL, 0, &errorMessage) != SQLITE_OK) {
            sqlite3_free(errorMessage);
            exit(1);
        }
    }

    char sqlSelectFiles[] = "SELECT PATH || \"\\\" || NAME FROM FILES WHERE NAME LIKE ";
    char searchQuery[MAX_PATH + 1];
    while (fprintf(stderr, "\nQuery: ") && fgets(searchQuery, sizeof(searchQuery), stdin) != NULL) {
        if (searchQuery[0] == '\n') {
            break;
        }
        searchQuery[strlen(searchQuery) - 1] = 0;
        snprintf(sql, strlen(sqlSelectFiles) + 2 + strlen(searchQuery) + 2 + 1 + 1, "%s\"%%%s%%\";", sqlSelectFiles, searchQuery);

        system("cls");

        if (sqlite3_exec(database, sql, PrintSearchResult, 0, &errorMessage) != SQLITE_OK) {
            sqlite3_free(errorMessage);
            exit(1);
        }
        fprintf(stderr, "\n%s\n", sql);
    }

    free(sql);

    sqlite3_close(database);

    return 0;
}
