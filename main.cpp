#include "mftreader.h"

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

    if (charactersRemaining < 2) {
        return nullptr;
    }

    char *filePath = (char *) malloc(charactersRemaining * sizeof(char));
    filePath[--charactersRemaining] = 0;
    
    File file = files[i];
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

int main() {
	File* files = GetFiles();
	for (uint64_t i = 0; i < arrlen(files); i++) {
        File file = files[i];
        file.path = GetFilePath(files, i);
        if (file.path) {
            fprintf(stderr, "%s\n", file.path);
        }
    }
	return 0;
}