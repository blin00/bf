/*
    special opcodes ([n] = value of raw byte):
    z:      zero memory cell
    i [n]:  increment cell by n
    l [n]:  shift ptr left n positions
    r [n]:  shift ptr right n positions
*/

#include <stdio.h>
#include <stdlib.h>

// e.g. 0, (-1), (tape[ptr])
#define BF_EOF 0
#define TAPE_LENGTH 30000

unsigned char tape_raw[TAPE_LENGTH] = { 0 };
unsigned char* tape = &tape_raw[0];

typedef struct Code Code;

typedef Code* (*bf_func)(Code* self);

struct Code {
    bf_func func;
    Code *next;
    Code *branch;
    unsigned char arg;
};

Code* bf_plus(Code* self) {
    (*tape)++;
    return self->next;
}

Code* bf_minus(Code* self) {
    (*tape)--;
    return self->next;
}

Code* bf_left(Code* self) {
    tape--;
    return self->next;
}

Code* bf_right(Code* self) {
    tape++;
    return self->next;
}

Code* bf_lb(Code* self) {
    if (*tape) return self->next;
    else return self->branch;
}

Code* bf_rb(Code* self) {
    if (*tape) return self->branch;
    else return self->next;
}

Code* bf_getc(Code* self) {
    int c = getchar();
    *tape = (unsigned char) (c == EOF ? BF_EOF : c);
    return self->next;
}

Code* bf_putc(Code* self) {
    putchar(*tape);
    return self->next;
}

Code* bf_zero(Code* self) {
    *tape = 0;
    return self->next;
}

Code* bf_inc(Code* self) {
    *tape += self->arg;
    return self->next;
}

void* realloc_failsoft(void* ptr, size_t size) {
    void* mem = realloc(ptr, size);
    if (!mem) {
        fprintf(stderr, "warn: realloc shrink failed - continuing\n");
        return ptr;
    }
    return mem;
}

void print_opt(unsigned char* opt, size_t len) {
    size_t i;
    for (i = 0; i < len; i++) {
        if (opt[i] == 'i' || opt[i] == 'l' || opt[i] == 'r') {
            fprintf(stderr, "%c(%d)", opt[i], (signed char) opt[i + 1]);
            i++;
        } else {
            fputc(opt[i], stderr);
        }
    }
    fputc('\n', stderr);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s file\n", argv[0]);
        return 1;
    }
    FILE *f = fopen(argv[1], "r");
    if (!f) {
        fprintf(stderr, "err: unable to open %s\n", argv[1]);
        return 1;
    }
    int c;
    unsigned char ch;
    size_t length = 0, maxLength = 1024, i, depth = 0, maxDepth = 1, line = 1, col = 0;
    unsigned char* code = malloc(maxLength * sizeof(char));
    if (!code) {
        fprintf(stderr, "err: malloc failed\n");
        fclose(f);
        return 1;
    }
    while ((c = fgetc(f)) != EOF) {
        ch = (unsigned char) c;
        if (ch == '\n') {
            line++;
            col = 0;
        }
        col++;
        if (ch != '+' && ch != '-' && ch != '<' && ch != '>' && ch != '[' && ch != ']' && ch != '.' && ch != ',')
            continue;
        if (ch == '[') {
            depth++;
            if (depth > maxDepth) {
                // overestimate of loop depth (b/c optimizations)
                maxDepth = depth;
            }
        } else if (ch == ']') {
            if (depth == 0) {
                fprintf(stderr, "err: closing bracket with no opening bracket at line %lu col %lu\n", (unsigned long) line, (unsigned long) col);
                fclose(f);
                free(code);
                return 1;
            }
            depth--;
        }
        length++;
        if (length > maxLength) {
            maxLength *= 2;
            unsigned char* newCode = realloc(code, maxLength * sizeof(char));
            if (!newCode) {
                fprintf(stderr, "err: realloc failed\n");
                fclose(f);
                free(code);
                return 1;
            }
            code = newCode;
        }
        code[length - 1] = ch;
    }
    fclose(f);
    if (depth != 0) {
        fprintf(stderr, "err: opening bracket with no closing bracket (need %lu at end)\n", (unsigned long) depth);
        free(code);
        return 1;
    }
    if (length == 0) {
        // done!
        free(code);
        return 0;
    }
    // optimize
    unsigned char* optCode = calloc(length, sizeof(char));
    size_t *jump = calloc(length, sizeof(size_t));
    size_t *stk = malloc(maxDepth * sizeof(size_t));
    if (!optCode) {
        fprintf(stderr, "err: calloc failed\n");
        free(code);
        return 1;
    }
    if (!jump) {
        fprintf(stderr, "err: calloc failed\n");
        free(code);
        free(optCode);
        return 1;
    }
    if (!stk) {
        fprintf(stderr, "err: malloc failed\n");
        free(code);
        free(optCode);
        free(jump);
        return 1;
    }
    size_t optLength, instrCt;
    unsigned char delta;
    i = optLength = instrCt = depth = delta = 0;
    while (i < length) {
        if (i < length - 2 && code[i] == '[' && (code[i + 1] == '+' || code[i + 1] == '-') && code[i + 2] == ']') {
            optCode[optLength++] = 'z';
            delta = 0;
            i += 3;
            instrCt++;
        } else if (code[i] == '+') {
            delta++;
            i++;
        } else if (code[i] == '-') {
            delta--;
            i++;
        } else {
            if (delta) {
                if (delta == 1) {
                    optCode[optLength++] = '+';
                } else if (delta == (unsigned char) -1) {
                    optCode[optLength++] = '-';
                } else {
                    optCode[optLength++] = 'i';
                    optCode[optLength++] = delta;
                }
                delta = 0;
                instrCt++;
            }
            optCode[optLength] = code[i];
            if (optCode[optLength] == '[') {
                stk[depth++] = instrCt;
            } else if (optCode[optLength] == ']') {
                size_t open = stk[--depth];
                jump[open] = instrCt;
                jump[instrCt] = open;
            }
            optLength++;
            i++;
            instrCt++;
        }
    }
    // repeated code :|
    if (delta) {
        if (delta == 1) {
            optCode[optLength++] = '+';
        } else if (delta == (unsigned char) -1) {
            optCode[optLength++] = '-';
        } else {
            optCode[optLength++] = 'i';
            optCode[optLength++] = delta;
        }
        delta = 0;
        instrCt++;
    }
    free(code);
    free(stk);
    //optCode = realloc_failsoft(optCode, optLength * sizeof(char));
    //jump = realloc_failsoft(jump, instrCt * sizeof(size_t));
    // make code
    Code* compiled = calloc(instrCt, sizeof(Code));
    if (!compiled) {
        fprintf(stderr, "calloc failed\n");
        free(optCode);
        free(jump);
        return 1;
    }
    Code* prev = NULL;
    size_t j;
    i = j = 0;
    while (i < optLength && j < instrCt) {
        Code* instr = &compiled[j];
        ch = optCode[i];
        if (ch == '[') {
            instr->func = bf_lb;
            instr->branch = compiled + jump[j];
        } else if (ch == ']') {
            instr->func = bf_rb;
            instr->branch = compiled + jump[j];
        } else if (ch == '+') {
            instr->func = bf_plus;
        } else if (ch == '-') {
            instr->func = bf_minus;
        } else if (ch == '<') {
            instr->func = bf_left;
        } else if (ch == '>') {
            instr->func = bf_right;
        } else if (ch == '.') {
            instr->func = bf_putc;
        } else if (ch == ',') {
            instr->func = bf_getc;
        } else if (ch == 'z') {
            instr->func = bf_zero;
        } else if (ch == 'i') {
            instr->func = bf_inc;
            instr->arg = optCode[++i];
        }
        if (prev) prev->next = instr;
        prev = instr;
        i++;
        j++;
    }
    free(optCode);
    free(jump);
    // run
    Code *ptr = &compiled[0];
    while (ptr) {
        ptr = ptr->func(ptr);
    }
    free(compiled);
    return 0;
}
