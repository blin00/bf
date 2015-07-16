#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#define ENABLE_BOUNDS_CHECKING

#ifdef ENABLE_BOUNDS_CHECKING
#define OUT_OF_BOUNDS (tapePtr < tape || tapePtr >= tape + tapeLength)
#define CHECK_BOUNDS if (OUT_OF_BOUNDS) { fprintf(stderr, "err: tape pointer out of bounds\n"); return NULL; }
#else
#define OUT_OF_BOUNDS (false)
#define CHECK_BOUNDS
#endif

typedef enum {
    EOF_UNCHANGED,
    EOF_VALUE
} eof_mode;

unsigned char* tape;
unsigned char* tapePtr;
size_t tapeLength = 30000;
eof_mode onEof = EOF_UNCHANGED;
unsigned char eofValue = 0;

typedef struct Code Code;

typedef Code* (*bf_func)(Code* self);

struct Code {
    bf_func func;
    Code *next;
    Code *branch;
    int shift;
    unsigned char inc;    
};

Code* bf_plus(Code* self) {
    (*tapePtr)++;
    return self->next;
}

Code* bf_minus(Code* self) {
    (*tapePtr)--;
    return self->next;
}

Code* bf_left(Code* self) {
    tapePtr--;
    return self->next;
}

Code* bf_right(Code* self) {
    tapePtr++;
    return self->next;
}

Code* bf_lb(Code* self) {
    if (*tapePtr) return self->next;
    else return self->branch;
}

Code* bf_rb(Code* self) {
    if (*tapePtr) return self->branch;
    else return self->next;
}

Code* bf_rb_nop(Code* self) {
    return self->next;
}

Code* bf_getc_unc(Code* self) {
    int c = getchar();
    if (c != EOF) *tapePtr = (unsigned char) c;
    return self->next;
}

Code* bf_getc_val(Code* self) {
    int c = getchar();
    if (c == EOF) *tapePtr = eofValue;
    else *tapePtr = (unsigned char) c;
    return self->next;
}

Code* bf_putc(Code* self) {
    putchar(*tapePtr);
    return self->next;
}

Code* bf_zero(Code* self) {
    *tapePtr = 0;
    return self->next;
}

Code* bf_inc(Code* self) {
    *tapePtr += self->inc;
    return self->next;
}

Code* bf_shift(Code* self) {
    tapePtr += self->shift;
    CHECK_BOUNDS
    return self->next;
}

Code* bf_inc_shift(Code* self) {
    *tapePtr += self->inc;
    tapePtr += self->shift;
    CHECK_BOUNDS
    return self->next;
}

Code* bf_inc_shift_lb(Code* self) {
    *tapePtr += self->inc;
    tapePtr += self->shift;
    CHECK_BOUNDS
    if (*tapePtr) return self->next;
    else return self->branch;
}

Code* bf_inc_shift_rb(Code* self) {
    *tapePtr += self->inc;
    tapePtr += self->shift;
    CHECK_BOUNDS
    if (*tapePtr) return self->branch;
    else return self->next;
}

Code* bf_end(Code* self) {
    return NULL;
}

// at least one of inc, shift must be nonzero
Code* emit_code(Code* c, unsigned char inc, int shift) {
    c->inc = inc;
    c->shift = shift;
    if (!inc) {
        if (shift == 1) {
            c->func = bf_right;
        } else if (shift == -1) {
            c->func = bf_left;
        } else {
            c->func = bf_shift;
        }
    } else if (!shift) {
        if (inc == 1) {
            c->func = bf_plus;
        } else if (inc == (unsigned char) -1) {
            c->func = bf_minus;
        } else {
            c->func = bf_inc;
        }
    } else {
        c->func = bf_inc_shift;
    }
    return c;
}

void print_repeat(FILE* f, int num, char up, char down) {
    unsigned char ch = num < 0 ? down : up;
    signed char dir = num < 0 ? 1 : -1;
    while (num != 0) {
        putc(ch, f);
        num += dir;
    }
}

void print_code(FILE* f, Code* opt, bool bf) {
    size_t i = 0;
    while (true) {
        Code* c = opt + i;
        if (c->func == bf_end) {
            break;
        } else if (c->func == bf_lb) {
            fputc('[', f);
        } else if (c->func == bf_rb || c->func == bf_rb_nop) {
            fputc(']', f);
        } else if (c->func == bf_putc) {
            fputc('.', f);
        } else if (c->func == bf_getc_unc || c->func == bf_getc_val) {
            fputc(',', f);
        } else if (c->func == bf_left) {
            fputc('<', f);
        } else if (c->func == bf_right) {
            fputc('>', f);
        } else if (c->func == bf_plus) {
            fputc('+', f);
        } else if (c->func == bf_minus) {
            fputc('-', f);
        } else if (c->func == bf_zero) {
            if (bf) fprintf(f, "[-]");
            else putc('z', f);
        } else if (c->func == bf_inc) {
            int inc = (signed char) c->inc;
            if (bf) {
                print_repeat(f, inc, '+', '-');
            } else fprintf(f, "i(%d)", inc);
        } else if (c->func == bf_shift) {
            if (bf) {
                print_repeat(f, c->shift, '>', '<');
            } else fprintf(f, "s(%d)", c->shift);
        } else if (c->func == bf_inc_shift) {
            int inc = (signed char) c->inc;
            if (bf) {
                print_repeat(f, inc, '+', '-');
                print_repeat(f, c->shift, '>', '<');
            } else fprintf(f, "c(%d,%d)", inc, c->shift);
        } else if (c->func == bf_inc_shift_lb) {
            int inc = (signed char) c->inc;
            if (bf) {
                print_repeat(f, inc, '+', '-');
                print_repeat(f, c->shift, '>', '<');
                fputc('[', f);
            } else fprintf(f, "c(%d,%d)[", inc, c->shift);
        } else if (c->func == bf_inc_shift_rb) {
            int inc = (signed char) c->inc;
            if (bf) {
                print_repeat(f, inc, '+', '-');
                print_repeat(f, c->shift, '>', '<');
                fputc(']', f);
            } else fprintf(f, "c(%d,%d)]", inc, c->shift);
        } else {
            fputc('?', f);
        }
        i++;
    }
    fputc('\n', f);
}

void print_usage() {
    fprintf(stderr, "usage: bf [-e eof_value] [-t tape_size] [-p] file\n");
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage();
        return 1;
    }
    int c;
    unsigned char ch;
    bool onlyPrint = false;
    opterr = 0;
    while ((c = getopt(argc, argv, ":e:t:hp")) != -1) {
        ch = (unsigned char) c;
        if (c == 'e') {
            onEof = EOF_VALUE;
            eofValue = (unsigned char) atoi(optarg);
        } else if (c == 't') {
            int len = atoi(optarg);
            if (len <= 0) {
                fprintf(stderr, "err: tape size must be positive integer\n");
                return 1;
            }
            tapeLength = (size_t) len;
        } else if (c == 'h') {
            print_usage();
            return 0;
        } else if (c == 'p') {
            onlyPrint = true;
        } else if (c == '?') {
            fprintf(stderr, "err: unknown option '%c'\n", optopt);
            return 1;
        } else if (c == ':') {
            fprintf(stderr, "err: option '%c' requires argument\n", optopt);
            return 1;
        }
    }
    if (optind >= argc) {
        fprintf(stderr, "err: no file provided\n");
        return 1;
    }
    FILE* f = fopen(argv[optind], "r");
    if (!f) {
        fprintf(stderr, "err: unable to open file '%s'\n", argv[1]);
        return 1;
    }
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
    // optimize
    size_t *stk = malloc(maxDepth * sizeof(size_t));
    if (!stk) {
        fprintf(stderr, "err: malloc failed\n");
        free(code);
        return 1;
    }
    size_t instrCt;
    bool definiteZero = true;
    unsigned char deltaInc;
    int deltaShift;
    i = instrCt = depth = deltaInc = deltaShift = 0;
    Code* compiled = calloc(length + 1, sizeof(Code));
    if (!compiled) {
        fprintf(stderr, "err: calloc failed\n");
        free(code);
        free(stk);
        return 1;
    }
    bf_func bf_getc = onEof == EOF_UNCHANGED ? bf_getc_unc : bf_getc_val;
    while (i < length) {
        ch = code[i];
        // zero opt
        if (!definiteZero && i < length - 2 && ch == '[' && (code[i + 1] == '+' || code[i + 1] == '-') && code[i + 2] == ']') {
            if (deltaShift) {
                emit_code(&compiled[instrCt], deltaInc, deltaShift);
                deltaInc = deltaShift = 0;
                instrCt++;
            }
            deltaInc = 0;
            compiled[instrCt].func = bf_zero;
            i += 2;
            instrCt++;
            definiteZero = true;
        } else if (ch == '<') {
            deltaShift--;
            definiteZero = false;
        } else if (ch == '>') {
            deltaShift++;
            definiteZero = false;
        } else if (ch == '-') {
            if (deltaShift) {
                emit_code(&compiled[instrCt], deltaInc, deltaShift);
                deltaInc = deltaShift = 0;
                instrCt++;
            }
            deltaInc--;
            definiteZero = false;
        } else if (ch == '+') {
            if (deltaShift) {
                emit_code(&compiled[instrCt], deltaInc, deltaShift);
                deltaInc = deltaShift = 0;
                instrCt++;
            }
            deltaInc++;
            definiteZero = false;
        } else if (ch == '[') {
            bool emitted = false;
            if (deltaInc || deltaShift) {
                emit_code(&compiled[instrCt], deltaInc, deltaShift);
                deltaInc = deltaShift = 0;
                //instrCt++;    // don't increment instrction count yet
                definiteZero = false;
                emitted = true;
            }
            // prune dead code
            if (definiteZero) {
                size_t currentDepth = depth;
                depth++;
                while (depth > currentDepth) {
                    i++;
                    if (code[i] == '[') depth++;
                    else if (code[i] == ']') depth--;
                    // i now pts to closing bracket, but incremented again at bottom of loop
                }
            } else {
                compiled[instrCt].func = emitted ? bf_inc_shift_lb : bf_lb;
                stk[depth++] = instrCt;
                // now increment ct here
                instrCt++;
            }
        } else if (ch == ']') {
            bool emitted = false;
            if (deltaInc || deltaShift) {
                emit_code(&compiled[instrCt], deltaInc, deltaShift);
                deltaInc = deltaShift = 0;
                //instrCt++;
                definiteZero = false;
                emitted = true;
            }
            compiled[instrCt].func = emitted ? bf_inc_shift_rb : definiteZero ? bf_rb_nop : bf_rb;
            size_t open = stk[--depth];
            compiled[open].branch = &compiled[instrCt + 1];
            compiled[instrCt].branch = &compiled[open + 1];
            instrCt++;
            definiteZero = true;
        } else if (ch == '.') {
            if (deltaInc || deltaShift) {
                emit_code(&compiled[instrCt], deltaInc, deltaShift);
                deltaInc = deltaShift = 0;
                instrCt++;
                definiteZero = false;
            }
            compiled[instrCt].func = bf_putc;
            instrCt++;
        } else if (ch == ',') {
            if (deltaInc || deltaShift) {
                emit_code(&compiled[instrCt], deltaInc, deltaShift);
                deltaInc = deltaShift = 0;
                instrCt++;
            }
            deltaInc = 0;
            compiled[instrCt].func = bf_getc;
            instrCt++;
            definiteZero = false;
        }
        i++;
    }
    if (deltaInc || deltaShift) {
        emit_code(&compiled[instrCt], deltaInc, deltaShift);
        deltaInc = deltaShift = 0;
        instrCt++;
    }
    // emit last instruction
    compiled[instrCt].func = bf_end;
    instrCt++;
    // set next ptrs
    for (i = 0; i < instrCt - 1; i++) {
        if (compiled[i + 1].func == bf_rb_nop) {
            compiled[i].next = &compiled[i + 2];
        } else {
            compiled[i].next = &compiled[i + 1];            
        }
    }
    free(code);
    free(stk);
    // run
    if (onlyPrint) {
        print_code(stdout, compiled, true);
    } else {
        tape = tapePtr = calloc((size_t) tapeLength, sizeof(unsigned char));
        if (!tape) {
            fprintf(stderr, "err: calloc failed\n");
            free(compiled);
            return 1;
        }
        Code *ptr = compiled;
        while (ptr) {
            ptr = ptr->func(ptr);
        }
        free(tape);
    }
    free(compiled);
    return 0;
}
