#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mapreduce.h"

void Map(char* file_name) {
    FILE* fp = fopen(file_name, "r");
    if (!fp) {
        fprintf(stderr, "Failed to open %s\n", file_name);
        return;
    }

    char* line = NULL;
    size_t size = 0;
    while (getline(&line, &size, fp) != -1) {
        char* token, *dummy = line;
        while ((token = strsep(&dummy, " \t\n\r")) != NULL) {
            if (*token != '\0') {
                MR_Emit(token, "1");
            }
        }
    }
    free(line);
    fclose(fp);
}

void Reduce(char* key, Getter get_next, int partition_number) {
    int count = 0;
    char* value;
    while ((value = get_next(key, partition_number)) != NULL) {
        count += atoi(value);
    }
    printf("%s %d\n", key, count);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s file1 [file2 ...]\n", argv[0]);
        return 1;
    }

    MR_Run(argc, argv, Map, 10, Reduce, 10, MR_DefaultHashPartition);
    return 0;
}