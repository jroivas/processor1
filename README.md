## Processor/1

Implementation of [Processor/1](https://esolangs.org/wiki/Processor/1)

Includes assembler and useful extensions

## Building and running

To build:

    mkdir build
    cd build
    cmake ..
    make

Compiling example:

    ../asm/asm.py ../examples/hello.p1 hello.bin

Running:

    emu/emu hello.bin
