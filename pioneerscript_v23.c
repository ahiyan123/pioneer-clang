#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <windows.h> // Essential for Windows Terminal control

#define BUF_SIZE 1024
#define STACK_SIZE 256

// --- WINDOWS FORCED OUTPUT ---
void init_windows_console() {
    // Attach to the parent console if we don't have one
    if (!AttachConsole(ATTACH_PARENT_PROCESS)) {
        AllocConsole();
    }
    // Force STDOUT to be unbuffered so "show" works immediately
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stdin, NULL, _IONBF, 0);
}

typedef struct { int data[STACK_SIZE]; int top; } Stack;
void push(Stack *s, int v) { if(s->top < STACK_SIZE-1) s->data[++s->top] = v; }
int pop(Stack *s, int *e) { if(s->top < 0) { *e=1; return 0; } *e=0; return s->data[s->top--]; }

void interpret(char *input, Stack *s) {
    char *token = strtok(input, " \n\r");
    int err = 0;
    while(token) {
        if(isdigit(token[0])) push(s, atoi(token));
        else if(strcmp(token, "%") == 0) push(s, pop(s, &err) + pop(s, &err));
        else if(strcmp(token, "show") == 0) {
            int v = pop(s, &err);
            if(!err) printf("pioneer output: %d\n", v);
            else printf(">> [ERR] Empty\n");
        }
        token = strtok(NULL, " \n\r");
    }
}

int main() {
    init_windows_console();
    Stack ms = {.top = -1};
    char cmd[BUF_SIZE];

    printf("PIONEERSCRIPT v26.1 [WINDOWS FIX]\n");
    printf("Motto: Pioneer doesn't know to rest.\n");

    while(1) {
        printf("pioneer> ");
        if(!fgets(cmd, BUF_SIZE, stdin)) break;
        interpret(cmd, &ms);
    }
    return 0;
}
