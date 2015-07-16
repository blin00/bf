bf
=====

An optimizing brainfuck interpreter written in C.

## Usage
    usage: bf [-e eof_value] [-t tape_size] [-p] [-v] file
       -e: integer value of EOF, or omit option to leave cell unchanged
       -t tape_size: length of tape (default: 30000)
       -p: print minified code to stdout instead of running it
       -v: show verbose messages

The interpreter halts if the pointer falls off the end of the tape (configurable via #define in bf.c)

## Build
    make
