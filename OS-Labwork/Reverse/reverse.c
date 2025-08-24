#define _GNU_SOURCE  // 启用getline和basename

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>  // 用于basename

typedef struct Node {
    char *line;        // 存储一行文本
    struct Node *next; // 指向下一节点
} Node;

Node* createNode(char *line) {
    Node *newNode = malloc(sizeof(Node));
    if (!newNode) {
        fprintf(stderr, "malloc failed\n");
        exit(1);
    }
    newNode->line = strdup(line);
    if (!newNode->line) {
        fprintf(stderr, "malloc failed\n");
        free(newNode);
        exit(1);
    }
    newNode->next = NULL;
    return newNode;
}

void insertNode(Node **head, char *line) {
    Node *newNode = createNode(line);
    newNode->next = *head;
    *head = newNode;
}

void printAndFreeList(Node *head, FILE *output) {
    Node *current = head;
    while (current) {
        fprintf(output, "%s\n", current->line);
        Node *temp = current;
        current = current->next;
        free(temp->line);
        free(temp);
    }
}

int main(int argc, char *argv[]) {
    FILE *input = NULL, *output = NULL;
    Node *head = NULL;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    if (argc > 3) {
        fprintf(stderr, "usage: reverse <input> <output>\n");
        return 1;
    }

    if (argc == 1) {
        input = stdin;
        output = stdout;
    } else if (argc == 2) {
        if (!(input = fopen(argv[1], "r"))) {
            fprintf(stderr, "reverse: cannot open file '%s'\n", argv[1]);
            return 1;
        }
        output = stdout;
    } else {
        // 复制字符串以避免basename修改原参数
        char *input_path = strdup(argv[1]);
        char *output_path = strdup(argv[2]);
        
        if (strcmp(basename(input_path), basename(output_path)) == 0) {
            fprintf(stderr, "reverse: input and output file must differ\n");
            free(input_path);
            free(output_path);
            return 1;
        }
        free(input_path);
        free(output_path);

        if (!(input = fopen(argv[1], "r"))) {
            fprintf(stderr, "reverse: cannot open file '%s'\n", argv[1]);
            return 1;
        }
        if (!(output = fopen(argv[2], "w"))) {
            fprintf(stderr, "reverse: cannot open file '%s'\n", argv[2]);
            fclose(input);
            return 1;
        }
    }

    while ((read = getline(&line, &len, input)) != -1) {
        // 移除换行符（与原始行为一致）
        if (read > 0 && line[read-1] == '\n') {
            line[read-1] = '\0';
        }
        insertNode(&head, line);
    }

    printAndFreeList(head, output);

    free(line);
    if (input != stdin) fclose(input);
    if (output != stdout) fclose(output);

    return 0;
}