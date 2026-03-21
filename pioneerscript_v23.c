#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <math.h>

// Windows Specific Headers
#include <windows.h>
#include <direct.h>

#define MAX_VARS 200
#define STACK_SIZE 512
#define BUF_SIZE 2048

// --- DATA STRUCTURES ---
typedef struct { 
    char name[64]; 
    int value; 
    int defined; 
    int is_temp; 
} Variable;

typedef struct { 
    int data[STACK_SIZE]; 
    int top; 
} Stack;

// --- GLOBAL STATE ---
Variable symbol_table[MAX_VARS];
int var_count = 0;
char admin_pin[16] = "";
int admin_active = 0;

// --- STACK ENGINE ---
void push(Stack *s, int val) { 
    if (s->top < STACK_SIZE - 1) s->data[++(s->top)] = val; 
    else printf(">> [ERR] Stack Overflow\n");
}

int pop(Stack *s, int *err) { 
    if (s->top < 0) { *err = 1; return 0; } 
    *err = 0; 
    return s->data[(s->top)--]; 
}

// --- INTERPRETER CORE ---
void interpret(char *code) {
    static Stack s = { .top = -1 }; // Static keeps stack alive between lines
    char code_copy[BUF_SIZE]; 
    strcpy(code_copy, code);
    char *token = strtok(code_copy, " \n\r");
    int st = 0;

    while (token != NULL) {
        // --- 1. BASIC MATH (%) (^)(x)(!)(mod)(**) ---
        if (strcmp(token, "%") == 0) push(&s, pop(&s, &st) + pop(&s, &st));
        else if (strcmp(token, "^") == 0) { int b = pop(&s, &st); push(&s, pop(&s, &st) - b); }
        else if (strcmp(token, "x") == 0) push(&s, pop(&s, &st) * pop(&s, &st));
        else if (strcmp(token, "!") == 0) { int b = pop(&s, &st); push(&s, pop(&s, &st) / b); }
        else if (strcmp(token, "mod") == 0) { int b = pop(&s, &st); push(&s, pop(&s, &st) % b); }
        else if (strcmp(token, "**") == 0) { int b = pop(&s, &st); push(&s, (int)pow(pop(&s, &st), b)); }

        // --- 2. BOOLEAN & COMPARISON (==)(>)(<)(and)(or)(not) ---
        else if (strcmp(token, "==") == 0) push(&s, (pop(&s, &st) == pop(&s, &st)));
        else if (strcmp(token, ">") == 0) { int b = pop(&s, &st); push(&s, (pop(&s, &st) > b)); }
        else if (strcmp(token, "<") == 0) { int b = pop(&s, &st); push(&s, (pop(&s, &st) < b)); }
        else if (strcmp(token, "and") == 0) push(&s, (pop(&s, &st) && pop(&s, &st)));
        else if (strcmp(token, "or") == 0)  push(&s, (pop(&s, &st) || pop(&s, &st)));
        else if (strcmp(token, "not") == 0) push(&s, !pop(&s, &st));

        // --- 3. STACK SURGERY (dup)(swap)(drop)(clear) ---
        else if (strcmp(token, "dup") == 0) { int v = pop(&s, &st); push(&s, v); push(&s, v); }
        else if (strcmp(token, "swap") == 0) { int a = pop(&s, &st); int b = pop(&s, &st); push(&s, a); push(&s, b); }
        else if (strcmp(token, "drop") == 0) { pop(&s, &st); }
        else if (strcmp(token, "clear") == 0) { s.top = -1; }

        // --- 4. CONDITIONALS (?) ---
        else if (strcmp(token, "?") == 0) { 
            int f = pop(&s, &st); int t = pop(&s, &st); int cond = pop(&s, &st);
            push(&s, cond ? t : f);
        }

        // --- 5. VARIABLE ASSIGNMENT ---
        else if (strcmp(token, "ass") == 0 || strcmp(token, "tmp") == 0) {
            int is_t = (strcmp(token, "tmp") == 0);
            char *name = strtok(NULL, " "); strtok(NULL, " "); // skip '='
            char *val_str = strtok(NULL, " ");
            if(name && val_str) {
                int found = 0;
                for(int i=0; i<var_count; i++) {
                    if(strcmp(symbol_table[i].name, name) == 0) {
                        symbol_table[i].value = atoi(val_str); found = 1; break;
                    }
                }
                if(!found) {
                    strcpy(symbol_table[var_count].name, name);
                    symbol_table[var_count].value = atoi(val_str);
                    symbol_table[var_count].is_temp = is_t;
                    symbol_table[var_count++].defined = 1;
                }
            }
        }
        else if (strcmp(token, "dec") == 0) {
            char *n = strtok(NULL, " ");
            for(int i=0; i<var_count; i++) {
                if(strcmp(symbol_table[i].name, n) == 0 && symbol_table[i].defined) {
                    push(&s, symbol_table[i].value);
                    if(symbol_table[i].is_temp) symbol_table[i].defined = 0; // Burn temp
                    break;
                }
            }
        }

        // --- 6. SYSTEM COMMANDS ---
        else if (strcmp(token, "show") == 0) {
            int val = pop(&s, &st);
            if(st == 0) printf("pioneer output: %d\n", val);
            else printf(">> [ERR] Stack empty.\n");
            fflush(stdout);
        }
        else if (strcmp(token, "time") == 0) push(&s, (int)time(NULL));
        else if (isdigit(token[0]) || (token[0] == '-' && isdigit(token[1]))) push(&s, atoi(token));
        
        token = strtok(NULL, " \n\r");
    }
}

int main() {
    SetConsoleTitle("PioneerScript v26 MASTER");
    printf("PIONEERSCRIPT v26 MASTER: Pioneer doesn't know to rest.\n");
    printf("Set Admin PIN: "); fflush(stdout);
    scanf("%15s", admin_pin);
    getchar(); // Clear newline

    char cmd[BUF_SIZE];
    while(1) { 
        printf("pioneer> "); fflush(stdout);
        if (fgets(cmd, BUF_SIZE, stdin) == NULL) break;
        if (strcmp(cmd, "exit\n") == 0) break;
        interpret(cmd); 
    }
    return 0;
}
