#include "mftreader.h"

void OutputFilePath(File *files, uint64_t i) {
    File file = files[i];
	if (!file.name) return;
    if (i != file.parent) OutputFilePath(files, file.parent);

	fprintf(stderr, "%s\\", file.name);
}

int main() {
	File* files = GetFiles();
	for (uint64_t i = 0; i < arrlen(files); i++) {
		OutputFilePath(files, i);
		fprintf(stderr, "\n");
	}
	return 0;
}