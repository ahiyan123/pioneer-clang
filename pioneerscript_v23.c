#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#ifdef _WIN32
    #include <windows.h>
    #define sleep(x) Sleep((x) * 1000) // Windows Sleep is in milliseconds
#else
    #include <unistd.h>
#endif

#include <curl/curl.h>
#include <openssl/sha.h>

#define MAX_VARS 100
#define STACK_SIZE 256
#define BUF_SIZE 1024

// --- STRUCTURES ---
typedef struct { char name[64]; int value; int defined; int is_temp; } Variable;
typedef struct {
    char filename[64]; char pin[16]; char recovery_key[40];
    int attempts; int is_hard_locked;
} Vault;
typedef struct { int data[STACK_SIZE]; int top; } Stack;

// --- GLOBAL STATE ---
Variable symbol_table[MAX_VARS];
Vault secure_vault[20];
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

// --- LIBCURL & OPENSSL ---
size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) { return fwrite(ptr, size, nmemb, stream); }

void get_web_file(const char *url, const char *out_name) {
    CURL *curl = curl_easy_init();
    if (curl) {
        FILE *fp = fopen(out_name, "wb");
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        if(curl_easy_perform(curl) != CURLE_OK) printf(">> [ERR] Network fail.\n");
        fclose(fp); curl_easy_cleanup(curl);
        printf(">> %s scouted.\n", url);
    }
}

void scan_file_identity(const char *path) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    FILE *f = fopen(path, "rb");
    if (!f) return;
    SHA256_CTX sha256; SHA256_Init(&sha256);
    char buffer[4096]; int bytesRead = 0;
    while ((bytesRead = fread(buffer, 1, 4096, f)) != 0) SHA256_Update(&sha256, buffer, bytesRead);
    SHA256_Final(hash, &sha256); fclose(f);
    printf(">> IDENTITY: ");
    for(int i=0; i<SHA256_DIGEST_LENGTH; i++) printf("%02x", hash[i]);
    printf("\n");
}

// --- ENGINE ---
void interpret(char *code) {
    Stack s = { .top = -1 };
    char code_copy[BUF_SIZE]; strcpy(code_copy, code);
    char *token = strtok(code_copy, " ");
    int st = 0;

    while (token != NULL) {
        // --- ADMIN & SECURITY ---
        if (strcmp(token, "sys-admin") == 0) {
            char in[16]; printf(">> PIN: "); scanf("%s", in);
            if (strcmp(admin_pin, in) == 0) admin_active = 1;
        }
        else if (strcmp(token, "lock-f-att-t") == 0) {
            char *fn = strtok(NULL, " ");
            if(fn) {
                strcpy(secure_vault[vault_count].filename, fn);
                strcpy(secure_vault[vault_count].pin, "1111");
                gen_recovery(secure_vault[vault_count].recovery_key);
                vault_count++;
            }
        }
        else if (strcmp(token, "scan-f") == 0 || strcmp(token, "scan-w") == 0) {
            char *path = strtok(NULL, " "); if(path) scan_file_identity(path);
        }

        // --- MATH OPS (THE CORE) ---
        else if (strcmp(token, "%") == 0) push(&s, pop(&s, &st) + pop(&s, &st));
        else if (strcmp(token, "^") == 0) { int b = pop(&s, &st); push(&s, pop(&s, &st) - b); }
        else if (strcmp(token, "x") == 0) push(&s, pop(&s, &st) * pop(&s, &st));
        else if (strcmp(token, "!") == 0) { int b = pop(&s, &st); push(&s, pop(&s, &st) / b); }
        else if (strcmp(token, "==") == 0) push(&s, (pop(&s, &st) == pop(&s, &st)));
        
        // --- VAR OPS ---
        else if (strcmp(token, "ass") == 0 || strcmp(token, "tmp") == 0) {
            int is_t = (strcmp(token, "tmp") == 0);
            char *n = strtok(NULL, " "); strtok(NULL, " ");
            strcpy(symbol_table[var_count].name, n);
            symbol_table[var_count].value = atoi(strtok(NULL, " "));
            symbol_table[var_count].is_temp = is_t;
            symbol_table[var_count++].defined = 1;
        }
        else if (strcmp(token, "dec") == 0) {
            char *n = strtok(NULL, " ");
            for(int i=0; i<var_count; i++) {
                if(strcmp(symbol_table[i].name, n) == 0) {
                    push(&s, symbol_table[i].value);
                    if(symbol_table[i].is_temp) symbol_table[i].defined = 0; // Burn temp
                    break;
                }
            }
        }

        // --- SYS & UTIL ---
        else if (strcmp(token, "help") == 0) printf(">> ass, tmp, dec, %%, ^, x, !, ==, get-wf, scan-f, sys-admin, sys, status, show, ign\n");
        else if (strcmp(token, "get-wf") == 0) { char *u = strtok(NULL, " "); if(u) get_web_file(u, "out.pnr"); }
        else if (strcmp(token, "show") == 0) printf("pioneer output: %d\n", pop(&s, &st));
        else if (strcmp(token, "sys") == 0) { char *c = strtok(NULL, "\""); if(admin_active) system(c); }
        else if (strcmp(token, "ign") == 0) return;
        else if (isdigit(token[0])) push(&s, atoi(token));

        token = strtok(NULL, " ");
    }
}

int main() {
    curl_global_init(CURL_GLOBAL_DEFAULT); srand(time(NULL));
    printf("PIONEERSCRIPT v23 MASTER: Pioneer doesn't know to rest.\nSet Admin PIN: ");
    scanf("%s", admin_pin);
    char cmd[BUF_SIZE];
    while(1) { printf("pioneer> "); fgets(cmd, BUF_SIZE, stdin); interpret(cmd); }
    curl_global_cleanup(); return 0;
}
