#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <windows.h>

#define STACK_SIZE 512
#define BUF_SIZE 2048

typedef struct { int data[STACK_SIZE]; int top; } Stack;

// --- MANDATORY WINDOWS FIXES ---
void push(Stack *s, int val) {
    if (s->top < STACK_SIZE - 1) s->data[++(s->top)] = val;
}

int pop(Stack *s, int *err) {
    if (s->top < 0) { *err = 1; return 0; }
    *err = 0;
    return s->data[(s->top)--];
}

void interpret(char *code, Stack *s) {
    char *token = strtok(code, " \n\r");
    int st = 0;

    while (token != NULL) {
        // Numbers
        if (isdigit(token[0]) || (token[0] == '-' && isdigit(token[1]))) {
            push(s, atoi(token));
        }
        // Operators
        else if (strcmp(token, "%") == 0) push(s, pop(s, &st) + pop(s, &st));
        else if (strcmp(token, "x") == 0) push(s, pop(s, &st) * pop(s, &st));
        else if (strcmp(token, "show") == 0) {
            int val = pop(s, &st);
            if (st == 0) {
                printf("pioneer output: %d\n", val);
            } else {
                printf(">> [ERR] Stack Empty\n");
            }
            fflush(stdout); // FORCE WINDOWS TO SHOW TEXT
        }
        // Add your other ops (==, >, <, ?, dup) here...
        
        token = strtok(NULL, " \n\r");
    }
}

int main() {
    // 1. Prepare Windows Console
    SetConsoleTitle("PioneerScript v26 MASTER");
    system("cls"); // Clear screen for a fresh start
    
    Stack main_stack = { .top = -1 };
    char admin_pin[16];
    char cmd[BUF_SIZE];

    printf("PIONEERSCRIPT v26\n----------------\n");
    printf("Set Admin PIN: ");
    fflush(stdout);
    
    // Use fgets for PIN to avoid leaving \n in the buffer
    fgets(admin_pin, 16, stdin);
    admin_pin[strcspn(admin_pin, "\n")] = 0; 

    printf("Engine Ready.\n");
    fflush(stdout);

    while(1) { 
        printf("pioneer> ");
        fflush(stdout); // Essential for Windows CMD
        
        if (fgets(cmd, BUF_SIZE, stdin) == NULL) break;
        if (strcmp(cmd, "exit\n") == 0) break;
        
        interpret(cmd, &main_stack);
        fflush(stdout); 
    }
    return 0;
}
