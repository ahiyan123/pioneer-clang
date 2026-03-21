#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#ifdef _WIN32
    #include <windows.h>
    #define sleep(x) Sleep((x) * 1000)
#else
    #include <unistd.h>
#endif

#include <curl/curl.h>
#include <openssl/sha.h>

#define MAX_VARS 100
#define STACK_SIZE 256
#define BUF_SIZE 1024
#define MAX_VAULTS 20

// --- STRUCTURES ---
typedef struct { char name[64]; int value; int defined; int is_temp; } Variable;
typedef struct {
    char filename[64]; char pin[16]; char recovery_key[40];
    int attempts; int is_hard_locked; int active;
} Vault;
typedef struct { int data[STACK_SIZE]; int top; } Stack;

// --- GLOBAL STATE ---
Variable symbol_table[MAX_VARS];
Vault secure_vault[MAX_VAULTS];
int var_count = 0, vault_count = 0;
char admin_pin[16] = "";
int admin_active = 0;

// --- CORE UTILS ---
void push(Stack *s, int val) { if (s->top < STACK_SIZE - 1) s->data[++(s->top)] = val; }
int pop(Stack *s, int *err) { if (s->top < 0) { *err = 2; return 0; } *err = 0; return s->data[(s->top)--]; }

void gen_recovery(char *buf) {
    const char *hex = "0123456789ABCDEF";
    for (int i = 0; i < 32; i++) buf[i] = hex[rand() % 16];
    buf[32] = '\0';
}

void interpret(char *code); // Forward declaration for recursion (run-pnr)

// --- ENGINE ---
void interpret(char *code) {
    Stack s = { .top = -1 };
    char code_copy[BUF_SIZE]; strcpy(code_copy, code);
    char *token = strtok(code_copy, " \n\r");
    int st = 0;

    while (token != NULL) {
        // --- 1. ADMIN & SECURITY ---
        if (strcmp(token, "sys-admin") == 0) {
            char in[16]; printf(">> PIN: "); fflush(stdout); scanf("%s", in);
            if (strcmp(admin_pin, in) == 0) { admin_active = 1; printf(">> ADMIN ACTIVE\n"); }
            else { printf(">> ACCESS DENIED\n"); }
            fflush(stdout);
        }
        else if (strcmp(token, "lock-f") == 0) {
            char *fn = strtok(NULL, " ");
            if(fn && vault_count < MAX_VAULTS) {
                strcpy(secure_vault[vault_count].filename, fn);
                strcpy(secure_vault[vault_count].pin, "1111"); // Default
                gen_recovery(secure_vault[vault_count].recovery_key);
                secure_vault[vault_count].active = 1;
                printf(">> %s LOCKED. Recovery: %s\n", fn, secure_vault[vault_count].recovery_key);
                vault_count++;
            }
            fflush(stdout);
        }

        // --- 2. MATH OPS ---
        else if (strcmp(token, "%") == 0) push(&s, pop(&s, &st) + pop(&s, &st));
        else if (strcmp(token, "^") == 0) { int b = pop(&s, &st); push(&s, pop(&s, &st) - b); }
        else if (strcmp(token, "x") == 0) push(&s, pop(&s, &st) * pop(&s, &st));
        else if (strcmp(token, "!") == 0) { int b = pop(&s, &st); push(&s, pop(&s, &st) / b); }
        
        // --- 3. STACK MANIPULATION (NEW) ---
        else if (strcmp(token, "dup") == 0) { int v = pop(&s, &st); push(&s, v); push(&s, v); }
        else if (strcmp(token, "swap") == 0) { int a = pop(&s, &st); int b = pop(&s, &st); push(&s, a); push(&s, b); }
        else if (strcmp(token, "drop") == 0) { pop(&s, &st); }

        // --- 4. VARIABLE OPS ---
        else if (strcmp(token, "ass") == 0 || strcmp(token, "tmp") == 0) {
            int is_t = (strcmp(token, "tmp") == 0);
            char *n = strtok(NULL, " "); strtok(NULL, " "); // skip '='
            char *v = strtok(NULL, " ");
            if(n && v) {
                int found = 0;
                for(int i=0; i<var_count; i++) {
                    if(strcmp(symbol_table[i].name, n) == 0) {
                        symbol_table[i].value = atoi(v); found = 1; break;
                    }
                }
                if(!found) {
                    strcpy(symbol_table[var_count].name, n);
                    symbol_table[var_count].value = atoi(v);
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
                    if(symbol_table[i].is_temp) symbol_table[i].defined = 0;
                    break;
                }
            }
        }

        // --- 5. AUTOMATION (NEW) ---
        else if (strcmp(token, "run-pnr") == 0) {
            char *path = strtok(NULL, " ");
            if(path) {
                FILE *fp = fopen(path, "r");
                if(fp) {
                    char line[BUF_SIZE];
                    while(fgets(line, BUF_SIZE, fp)) interpret(line);
                    fclose(fp);
                }
            }
        }

        // --- 6. SYS & SHOW ---
        else if (strcmp(token, "show") == 0) {
            int val = pop(&s, &st);
            if(st == 0) printf("pioneer output: %d\n", val);
            else printf(">> [ERR] Stack empty.\n");
            fflush(stdout);
        }
        else if (strcmp(token, "help") == 0) {
            printf(">> ass, tmp, dec, %%, ^, x, !, lock-f, dup, swap, drop, run-pnr, sys-admin, sys, show, ign\n");
            fflush(stdout);
        }
        else if (isdigit(token[0])) push(&s, atoi(token));
        else if (strcmp(token, "ign") == 0) return;

        token = strtok(NULL, " \n\r");
    }
}

int main() {
    srand(time(NULL));
    printf("PIONEERSCRIPT v25 MASTER: Pioneer doesn't know to rest.\nSet Admin PIN: ");
    fflush(stdout);
    scanf("%15s", admin_pin);
    int c; while ((c = getchar()) != '\n' && c != EOF); // Clear buffer

    char cmd[BUF_SIZE];
    while(1) { 
        printf("pioneer> "); fflush(stdout);
        if (fgets(cmd, BUF_SIZE, stdin) == NULL) break;
        interpret(cmd); 
    }
    return 0;
}
