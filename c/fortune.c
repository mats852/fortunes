#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

// Prototypes
int nrand(int n);

char fortunes[] = "fortunes";
char idx[] = "fortunes.index";

_Bool isIndexValid(struct stat* fortunesFileStat, struct stat* indexFileStat);
void chooseFortune(FILE* fortunesFile, FILE* indexFile, char* choice, int choicelen);
void chooseFortuneFromIndex(FILE* fortunesFile, FILE* indexFile, char* choice, int choicelen);
size_t calculateRandomOffset(size_t length, size_t offs);

int main(int argc, char* argv[])
{
	FILE *fortunesFile, *indexFile;

	fortunesFile = fopen(fortunes, "r+");
	if (fortunesFile == NULL) {
		perror("misfortune");

		return 1;
	}

	char* indexFileMode = "r";

	indexFile = fopen(idx, indexFileMode);
	if (indexFile == NULL) {
		indexFileMode = "w";
		indexFile = fopen(idx, indexFileMode);
		if (indexFile == NULL) {
			perror("misfortune");

			goto _cleanup;
		}
	}

	struct stat fortunesFileStat;

	if (stat(fortunes, &fortunesFileStat) != 0) {
		perror("misfortune");

		goto _cleanup;
	}

	struct stat indexFileStat;

	if (stat(idx, &indexFileStat) != 0) {
		perror("misfortune");

		goto _cleanup;
	}

	char choice[2048];
	int choicelen = sizeof(choice) / sizeof(char);

	if (isIndexValid(&fortunesFileStat, &indexFileStat)) {
		chooseFortuneFromIndex(fortunesFile, indexFile, choice, choicelen);
	} else {
		if (!strcmp(indexFileMode, "w")) {
			indexFile = freopen(idx, "w", indexFile);
			if (indexFile == NULL) {
				perror("misfortune");

				goto _cleanup;
			}
		}

		chooseFortune(fortunesFile, indexFile, choice, choicelen);

		fflush(indexFile);
		fclose(indexFile);
	}

	puts(choice);

_cleanup:

	if (fortunesFile != NULL)
		fclose(fortunesFile);

	if (indexFile != NULL)
		fclose(indexFile);
}

_Bool isIndexValid(struct stat* fortunesFileStat, struct stat* indexFileStat)
{
	if (indexFileStat->st_size == 0) {
		return false;
	}

	return (fortunesFileStat->st_mtime < indexFileStat->st_mtime);
}

void chooseFortune(FILE* fortunesFile, FILE* indexFile, char* choice, int choicelen)
{
	unsigned char off[4];

	char* line = NULL;
	size_t len = 0;

	for (int i = 0; getline(&line, &len, fortunesFile) != -1; i++) {
		long offset = ftell(fortunesFile);

		off[0] = offset;
		off[1] = offset >> 8;
		off[2] = offset >> 16;
		off[3] = offset >> 24;

		fwrite(off, sizeof(unsigned char), sizeof(off), indexFile);

		if (nrand(i) == 0) {
			strncpy(choice, line, choicelen);
		}
	}

	free(line);
}

void chooseFortuneFromIndex(
	FILE* fortunesFile,
	FILE* indexFile,
	char* choice,
	int choicelen)
{
	unsigned char off[4];

	struct stat indexFileStat;

	if (stat(idx, &indexFileStat) != 0) {
		perror("misfortune");

		return;
	}

	int r = fseek(indexFile, calculateRandomOffset(indexFileStat.st_size, sizeof(off)), 0);
	if (r == -1) {
		strcpy(choice, "misfortune");
		return;
	}

	r = fread(off, sizeof(off), 1, indexFile);
	if (r == -1) {
		strcpy(choice, "misfortune");
		return;
	}

	r = fseek(fortunesFile, off[0] | (off[1] << 8) | (off[2] << 16) | (off[3] << 24), 0);
	if (r == -1) {
		strcpy(choice, "misfortune");
		return;
	}

	int ch;
	for (int i = 0; (ch = fgetc(fortunesFile)) != '\n' && ch != EOF && i < choicelen; i++) {
		choice[i] = ch;
	}
}

int nrand(int n)
{
	if (n <= 0)
		return 0;

	return arc4random() % n;
}

size_t calculateRandomOffset(size_t length, size_t offs)
{
	return (arc4random() % (length / offs)) * offs;
}
