/*
    special opcodes ([n] = value of raw byte):
    z:      zero memory cell
    i [n]:  increment cell by n
    l [n]:  shift ptr left n positions
    r [n]:  shift ptr right n positions
*/

#include <stdio.h>
#include <stdlib.h>

// e.g. 0, (-1), (*tape)
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
    int arg2;
    unsigned char arg1;    
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

Code* bf_rb_nop(Code* self) {
    return self->next;
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
    *tape += self->arg1;
    return self->next;
}

Code* bf_shift(Code* self) {
    tape += self->arg2;
    return self->next;
}

Code* bf_inc_shift(Code* self) {
    *tape += self->arg1;
    tape += self->arg2;
    return self->next;
}

Code* bf_end(Code* self) {
    return NULL;
}

// at least one of inc, shift must be nonzero
Code* emit_code(Code* c, unsigned char inc, int shift) {
    if (!inc) {
        if (shift == 1) {
            c->func = bf_right;
        } else if (shift == -1) {
            c->func = bf_left;
        } else {
            c->func = bf_shift;
            c->arg2 = shift;
        }
    } else if (!shift) {
        if (inc == 1) {
            c->func = bf_plus;
        } else if (inc == (unsigned char) -1) {
            c->func = bf_minus;
        } else {
            c->func = bf_inc;
            c->arg1 = inc;
        }
    } else {
        c->func = bf_inc_shift;
        c->arg1 = inc;
        c->arg2 = shift;
    }
    return c;
}

void print_repeat(int num, char up, char down) {
    unsigned char ch = num < 0 ? down : up;
    signed char dir = num < 0 ? 1 : -1;
    while (num != 0) {
        putc(ch, stderr);
        num += dir;
    }
}

void print_opt(Code* opt, unsigned char bf) {
    size_t i = 0;
    while (1) {
        Code* c = opt + i;
        if (c->func == bf_end) {
            break;
        } else if (c->func == bf_lb) {
            fputc('[', stderr);
        } else if (c->func == bf_rb || c->func == bf_rb_nop) {
            fputc(']', stderr);
        } else if (c->func == bf_putc) {
            fputc('.', stderr);
        } else if (c->func == bf_getc) {
            fputc(',', stderr);
        } else if (c->func == bf_left) {
            fputc('<', stderr);
        } else if (c->func == bf_right) {
            fputc('>', stderr);
        } else if (c->func == bf_plus) {
            fputc('+', stderr);
        } else if (c->func == bf_minus) {
            fputc('-', stderr);
        } else if (c->func == bf_zero) {
            if (bf) fprintf(stderr, "[-]");
            else putc('z', stderr);
        } else if (c->func == bf_inc) {
            int inc = (signed char) c->arg1;
            if (bf) {
                print_repeat(inc, '+', '-');
            } else fprintf(stderr, "i(%d)", inc);
        } else if (c->func == bf_shift) {
            if (bf) {
                print_repeat(c->arg2, '>', '<');
            } else fprintf(stderr, "s(%d)", c->arg2);
        } else if (c->func == bf_inc_shift) {
            int inc = (signed char) c->arg1;
            if (bf) {
                print_repeat(inc, '+', '-');
                print_repeat(c->arg2, '>', '<');
            } else fprintf(stderr, "c(%d,%d)", inc, c->arg2);
        } else {
            fputc('?', stderr);
        }
        i++;
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
    // optimize
    size_t *stk = malloc(maxDepth * sizeof(size_t));
    if (!stk) {
        fprintf(stderr, "err: malloc failed\n");
        free(code);
        return 1;
    }
    size_t instrCt;
    unsigned char definiteZero = 1;
    unsigned char deltaInc;
    int deltaShift;
    i = instrCt = depth = deltaInc = deltaShift = 0;
    Code* compiled = calloc(length + 1, sizeof(Code));
    if (!compiled) {
        fprintf(stderr, "calloc failed\n");
        free(code);
        free(stk);
        return 1;
    }
    while (i < length) {
        if (definiteZero) {
            if (deltaInc || deltaShift) {
                fprintf(stderr, "invalid state\n");
                return 1;
            }
        }
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
            definiteZero = 1;
        } else if (ch == '<') {
            deltaShift--;
            definiteZero = 0;
        } else if (ch == '>') {
            deltaShift++;
            definiteZero = 0;
        } else if (ch == '-') {
            if (deltaShift) {
                emit_code(&compiled[instrCt], deltaInc, deltaShift);
                deltaInc = deltaShift = 0;
                instrCt++;
            }
            deltaInc--;
            definiteZero = 0;
        } else if (ch == '+') {
            if (deltaShift) {
                emit_code(&compiled[instrCt], deltaInc, deltaShift);
                deltaInc = deltaShift = 0;
                instrCt++;
            }
            deltaInc++;
            definiteZero = 0;
        } else if (ch == '[') {
            if (deltaInc || deltaShift) {
                emit_code(&compiled[instrCt], deltaInc, deltaShift);
                deltaInc = deltaShift = 0;
                instrCt++;
                definiteZero = 0;
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
                compiled[instrCt].func = bf_lb;
                stk[depth++] = instrCt;
                instrCt++;
            }
        } else if (ch == ']') {
            if (deltaInc || deltaShift) {
                emit_code(&compiled[instrCt], deltaInc, deltaShift);
                deltaInc = deltaShift = 0;
                instrCt++;
                definiteZero = 0;
            }
            compiled[instrCt].func = definiteZero ? bf_rb_nop : bf_rb;
            size_t open = stk[--depth];
            compiled[open].branch = &compiled[instrCt + 1];
            compiled[instrCt].branch = &compiled[open + 1];
            instrCt++;
            definiteZero = 1;
        } else if (ch == '.') {
            if (deltaInc || deltaShift) {
                emit_code(&compiled[instrCt], deltaInc, deltaShift);
                deltaInc = deltaShift = 0;
                instrCt++;
                definiteZero = 0;
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
            definiteZero = 0;
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
        compiled[i].next = &compiled[i + 1];
    }
    free(code);
    free(stk);
    // run
    //print_opt(compiled, 0);
    Code *ptr = compiled;
    while (ptr) {
        ptr = ptr->func(ptr);
    }
    free(compiled);
    return 0;
}
