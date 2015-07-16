bf
=====

An optimizing brainf*ck interpreter written in C.

## Usage
    usage: bf [-e eof_value] [-t tape_size] [-p] [-v] file
       -e eof_value: integer value of EOF, or omit option to leave cell unchanged
       -t tape_size: length of tape (default: 30000)
       -p: print minified bf code to stdout instead of running it
       -v: show verbose messages

Program input is taken from stdin. The interpreter halts if the pointer falls off the end of the tape (configurable via #define in bf.c)

## Build
    make

## Optimizations
 * Recognizes simple loops to set current cell to 0 (`[-]`, `[+]`)
 * Lumps sequences of `+`/`-`, `<`/`>`, `[`/`]` into one instruction
 * Compute jumps ahead of time
 * Skips loops that are never entered

## Missing optimizations
 * Some constructs are not fully optimized e.g. `+[.[-]]+[-]` optimizes to `+[.[-]][-]` and not `+[.[-]]` (but shouldn't ever appear in well-written code ;) )
 * Adding one cell to another e.g. `[->+<]`
