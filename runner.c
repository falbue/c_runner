#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_CODE_SIZE 65536
#define MAX_OUTPUT_SIZE 131072

int has_main_function(const char *code) {
    const char *patterns[] = {
        "int main(",
        "void main(",
        "int main(void",
        "void main(void",
        "int main (",
        "void main ("
    };
    
    size_t patterns_count = sizeof(patterns) / sizeof(patterns[0]);
    
    for (size_t i = 0; i < patterns_count; i++) {
        if (strstr(code, patterns[i]) != NULL) {
            return 1;
        }
    }
    return 0;
}

void safe_add_main_wrapper(char *code, size_t max_size) {
    const char *wrapper = 
        "#include <stdio.h>\n\n"
        "int main() {\n"
        "    // User code execution\n"
        "    %s\n"
        "    return 0;\n"
        "}\n";
    
    char new_code[MAX_CODE_SIZE];
    int written = snprintf(new_code, max_size, wrapper, code);
    
    if (written < 0 || written >= max_size) {
        // Если не удалось добавить обертку, оставляем исходный код
        return;
    }
    
    strncpy(code, new_code, max_size - 1);
    code[max_size - 1] = '\0';
}

int main() {
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    
    printf("=== C CODE RUNNER STARTED ===\n");
    
    char code[MAX_CODE_SIZE] = {0};
    size_t total_read = 0;
    char buffer[1024];
    
    // Читаем код из stdin
    while (fgets(buffer, sizeof(buffer), stdin)) {
        size_t len = strlen(buffer);
        if (total_read + len < MAX_CODE_SIZE) {
            strcat(code, buffer);
            total_read += len;
        } else {
            fprintf(stderr, "WARNING: Code truncated due to size limits\n");
            break;
        }
    }
    
    if (total_read == 0) {
        fprintf(stderr, "ERROR: No code provided\n");
        return 1;
    }
    
    if (!has_main_function(code)) {
        printf("INFO: Adding main() wrapper\n");
        safe_add_main_wrapper(code, MAX_CODE_SIZE);
    }
    
    FILE *fp = fopen("user_code.c", "w");
    if (!fp) {
        fprintf(stderr, "ERROR: Cannot create file\n");
        return 1;
    }
    fputs(code, fp);
    fclose(fp);
    
    printf("COMPILING...\n");
    
    // Компилируем с дополнительными флагами безопасности
    FILE *compile_pipe = popen("gcc -Wall -Wextra -Werror -fstack-protector user_code.c -o user_program 2>&1", "r");
    if (!compile_pipe) {
        fprintf(stderr, "ERROR: Compilation failed\n");
        return 1;
    }
    
    char compile_output[MAX_OUTPUT_SIZE] = {0};
    size_t output_size = 0;
    char comp_buffer[1024];
    
    while (fgets(comp_buffer, sizeof(comp_buffer), compile_pipe)) {
        size_t len = strlen(comp_buffer);
        if (output_size + len < MAX_OUTPUT_SIZE) {
            strcat(compile_output, comp_buffer);
            output_size += len;
        }
    }
    
    int compile_status = pclose(compile_pipe);
    
    if (compile_status != 0) {
        printf("%s", compile_output);
        fprintf(stderr, "\nCOMPILATION FAILED\n");
        return 1;
    }
    
    printf("EXECUTING...\n");
    printf("=== PROGRAM OUTPUT ===\n");
    
    // Запускаем программу с ограничениями
    system("timeout 10s ./user_program");
    
    printf("\n=== EXECUTION FINISHED ===\n");
    return 0;
}