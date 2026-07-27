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
    if (!filePath) {
        fprintf(stderr, "\nWARNING: Ran out of memory when trying to allocate space for a file path. Database will be incomplete.");
        return nullptr;
    }

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

class Args {
public:
    char searchTerm[MAX_PATH];

    Args(char *searchTerm) {
		if (!searchTerm) {
			this->searchTerm[0] = 0;
			return;
		}
        memcpy(this->searchTerm, searchTerm, MAX_PATH);
		this->searchTerm[MAX_PATH - 1] = 0;
    }

	Args() : Args(NULL) {}
};

int main(int argc, char **argv) {
	Args args = (argc > 1) ? Args(argv[argc - 1]) : Args();

	char sqlSelectFiles[] = "SELECT PATH || \"\\\" || NAME FROM FILES WHERE NAME LIKE ";
    char sqlCreateFiles[] = "CREATE TABLE IF NOT EXISTS FILES("
                            "NAME TEXT, "
                            "PATH TEXT"
                            ");";
    char sqlInsertFilesHeader[] = "INSERT INTO FILES (NAME, PATH) "
                                  "VALUES ";

	sqlite3 *database;
    char *errorMessage;
	char *sql = (char *) malloc(SQL_SIZE * sizeof(char));
    if (!sql) {
        fprintf(stderr, "ERROR: Ran out of memory when trying to allocate space for SQL statements.\n");
        return 1;
    }

	if (strlen(args.searchTerm) > 0) {
	    Ok(sqlite3_open("C:\\Search\\Search.db", &database));
		snprintf(sql, strlen(sqlSelectFiles) + 2 + strlen(args.searchTerm) + 2 + 1 + 1, "%s\"%%%s%%\";", sqlSelectFiles, args.searchTerm);
		fprintf(stderr, "\n");
		if (sqlite3_exec(database, sql, PrintSearchResult, 0, &errorMessage) != SQLITE_OK) {
			fprintf(stderr, "DATABASE ERROR: %s\n", errorMessage);
			fprintf(stderr, "Run the exe without a search term to build the database first.\n");
			free(sql);
			sqlite3_free(errorMessage);
			exit(1);
		}
		free(sql);
		sqlite3_close(database);
		return 0;
	}

    File *files = GetFiles();
    if (!files) {
        fprintf(stderr, "\nRequesting admin privileges...\n");
        ShellExecute(NULL, "runas", argv[0], NULL, NULL, SW_SHOWNORMAL);
        return 1;
    }

    CreateDirectory("C:\\Search", NULL);
    DeleteFile("C:\\Search\\Search.db");
    Ok(sqlite3_open("C:\\Search\\Search.db", &database));

    fprintf(stderr, "\nCollecting file paths...\n");
    for (uint64_t i = 0; i < arrlen(files); i++) {
        files[i].path = GetPath(files, i);  // not a deep copy
    }

    fprintf(stderr, "\n%s\n\n", sqlCreateFiles);
    if (sqlite3_exec(database, sqlCreateFiles, NULL, 0, &errorMessage) != SQLITE_OK) {
        free(sql);
        sqlite3_free(errorMessage);
        exit(1);
    }

    for (uint64_t i = 0; i < arrlen(files); i++) {
        snprintf(sql, SQL_SIZE, "%s", sqlInsertFilesHeader);

        int j = i;
        for (j; j < arrlen(files); j++) {
            File file = files[j];
            int valueLength = GetFileValueLength(files, j) + 1;
            if (valueLength < 2) {
                continue;
            }

            // https://cplusplus.com/reference/cstdio/snprintf/
            char *writeStart = &sql[strlen(sql)];
            int charactersRemaining = SQL_SIZE - strlen(sql);
            int returnValue = snprintf(writeStart, charactersRemaining, "(\"%s\", \"%s\"), ", file.name, file.path);
            if ((returnValue < 0 || returnValue >= charactersRemaining)) {
                *(writeStart - 1) = 0;
                *(writeStart - 2) = ';';
                break;
            }
            else if (j == arrlen(files) - 1) {
                sql[strlen(sql) - 1] = 0;
                sql[strlen(sql) - 1] = ';';
            }
        }

        if (strlen(sql) > strlen(sqlInsertFilesHeader) && sqlite3_exec(database, sql, NULL, 0, &errorMessage) != SQLITE_OK) {
            fprintf(stderr, "DATABASE ERROR: %s\n", errorMessage);
            free(sql);
            sqlite3_free(errorMessage);
            exit(1);
        }
        fprintf(stderr, "%s(%zu characters)\n", sqlInsertFilesHeader, strlen(sql) - strlen(sqlInsertFilesHeader));   // insert files

        if (i >= j) {
            break;
        }
        i = j - 1;
    }

    free(sql);
    sqlite3_close(database);
    return 0;
}