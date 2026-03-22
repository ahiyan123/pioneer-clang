#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <windows.h>
#include <time.h>

#define MAX_VARS 200
#define STACK_SIZE 512
#define BUF_SIZE 2048

// --- DATA STRUCTURES ---
typedef struct { char name[64]; int value; int defined; int is_temp; } Variable;
typedef struct { int data[STACK_SIZE]; int top; } Stack;

Variable symbol_table[MAX_VARS];
int var_count = 0;

// --- CORE ENGINE ---
void push(Stack *s, int val) { if (s->top < STACK_SIZE - 1) s->data[++(s->top)] = val; }
int pop(Stack *s, int *err) { if (s->top < 0) { *err = 1; return 0; } *err = 0; return s->data[(s->top)--]; }

void clean_token(char *t) {
    char *p = t;
    while (*p) {
        if (*p == '\r' || *p == '\n') { *p = '\0'; break; }
        p++;
    }
}

// --- SOVEREIGN VAULT (Bypassing OpenSSL) ---
void vault_cipher(char *data, char *key) {
    int data_len = strlen(data);
    int key_len = strlen(key);
    for(int i = 0; i < data_len; i++) {
        data[i] = data[i] ^ key[i % key_len]; // Native XOR encryption
    }
}

// --- INTERPRETER ---
void interpret(char *code, Stack *s) {
    char *token = strtok(code, " ");
    int st = 0;

    while (token != NULL) {
        clean_token(token);
        if (strlen(token) == 0) { token = strtok(NULL, " "); continue; }

        // --- 1. SYSTEM & HELP ---
        if (strcmp(token, "help") == 0) {
            printf("\n--- PIONEERSCRIPT v26.5 MASTER ---\n");
            printf("MATH:  %%(add) ^(sub) x(mul) !(div) mod **\n");
            printf("LOGIC: == > < and or not ?(ternary)\n");
            printf("STACK: dup swap drop clear show\n");
            printf("VOICE: printt(num) printts(msg) nl(newline)\n");
            printf("VAR:   ass tmp dec\n");
            printf("VAULT: lock-f (Native XOR Secure)\n");
            printf("SYS:   time exit\n");
            fflush(stdout);
        }
        else if (strcmp(token, "exit") == 0) exit(0);

        // --- 2. VOICE ---
        else if (strcmp(token, "printt") == 0) {
            int val = pop(s, &st);
            if (!st) { printf("%d", val); push(s, val); }
            fflush(stdout);
        }
        else if (strcmp(token, "printts") == 0) {
            char *msg = strtok(NULL, " ");
            if (msg) {
                for (int i = 0; msg[i]; i++) {
                    if (msg[i] == '_') printf(" ");
                    else printf("%c", msg[i]);
                }
            }
            fflush(stdout);
        }
        else if (strcmp(token, "nl") == 0) { printf("\n"); fflush(stdout); }

        // --- 3. MATH & LOGIC ---
        else if (strcmp(token, "%") == 0) push(s, pop(s, &st) + pop(s, &st));
        else if (strcmp(token, "^") == 0) { int b = pop(s, &st); push(s, pop(s, &st) - b); }
        else if (strcmp(token, "x") == 0) push(s, pop(s, &st) * pop(s, &st));
        else if (strcmp(token, "!") == 0) { int b = pop(s, &st); push(s, pop(s, &st) / b); }
        else if (strcmp(token, "mod") == 0) { int b = pop(s, &st); push(s, pop(s, &st) % b); }
        else if (strcmp(token, "**") == 0) { int b = pop(s, &st); push(s, (int)pow(pop(s, &st), b)); }
        else if (strcmp(token, "==") == 0) push(s, (pop(s, &st) == pop(s, &st)));
        else if (strcmp(token, ">") == 0) { int b = pop(s, &st); push(s, (pop(s, &st) > b)); }
        else if (strcmp(token, "<") == 0) { int b = pop(s, &st); push(s, (pop(s, &st) < b)); }
        else if (strcmp(token, "?") == 0) { 
            int f = pop(s, &st); int t = pop(s, &st); int cond = pop(s, &st);
            push(s, cond ? t : f);
        }

        // --- 4. STACK & VARS ---
        else if (strcmp(token, "show") == 0) {
            int v = pop(s, &st);
            if (!st) printf("pioneer output: %d\n", v);
            else printf(">> [ERR] Empty\n");
            fflush(stdout);
        }
        else if (strcmp(token, "dup") == 0) { int v = pop(s, &st); push(s, v); push(s, v); }
        else if (strcmp(token, "ass") == 0 || strcmp(token, "tmp") == 0) {
            int is_t = (strcmp(token, "tmp") == 0);
            char *name = strtok(NULL, " "); strtok(NULL, " ");
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
                    push(s, symbol_table[i].value);
                    if(symbol_table[i].is_temp) symbol_table[i].defined = 0;
                    break;
                }
            }
        }
        else if (isdigit(token[0]) || (token[0] == '-' && isdigit(token[1]))) push(s, atoi(token));

        token = strtok(NULL, " ");
    }
}

int main() {
    setvbuf(stdout, NULL, _IONBF, 0);
    Stack main_stack = { .top = -1 };
    char cmd[BUF_SIZE];

    printf("PIONEERSCRIPT v26.5 [STANDALONE]\n");
    printf("Status: Sovereign | Everyone isn't same.\n");

    while(1) {
        printf("pioneer> ");
        fflush(stdout);
