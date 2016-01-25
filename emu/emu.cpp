#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>

#define RegType unsigned long long
#define RegTypeS signed long long
#define ByteType unsigned char

#define AllMask 0xF0
#define OneMask 0xF0
#define TwoMask 0x80
#define FourMask 0xC0
#define ImmMask 0xE0

enum OpType {
    NOP = 0xF1,
    RET = 0xF2,
    IL  = 0xF3,
    IU  = 0xF4,

    INT = 0x81,
    LPC = 0x82,
    LSP = 0x83,
    LIP = 0x84,
    LCR = 0x85,
    NOT = 0x86,
    PUS = 0x87,
    POP = 0x88,
    SIP = 0x89,
    SSP = 0x8A,
    SCR = 0x8B,

    L   = 0x01,
    LS  = 0x02,
    ST  = 0x03,
    A   = 0x04,
    AU  = 0x05,
    S   = 0x06,
    SU  = 0x07,
    M   = 0x08,
    MU  = 0x09,
    AND = 0x0A,
    OR  = 0x0B,
    XOR = 0x0C,
    B   = 0x0D,
    BAS = 0x0E,
    CP  = 0x0F,
    CPU = 0x10,
    SHL = 0x11,
    SHR = 0x12,

    D   = 0xC1,
    DU  = 0xC2,
    BAL = 0xC3,
    LSM = 0xC4,
    STM = 0xC5,
    LUM = 0xC6,
    SUM = 0xC7,

    LI  = 0xE1
};

enum CRStates {
    CRLessThan = 1L << 63,
    CRGreaterThan = 1L << 62,
    CREqual = 1L << 61,
    Overflow = 1L << 61
};


class World {

public:
    World(RegType size=0) {
        interrupts = true;
        PC = 0;
        SP = 0;
        IP = 0;
        CR = 0;
        mem = nullptr;
        stack_size = 0;

        genMem(size);
    }

    ~World() {
        if (mem) delete[] mem;
    }

    void genMem(RegType size) {
        mem_size = size;
        if (size == 0) return;
        if (mem) delete[] mem;
        mem = new ByteType[size];
    }

    void setupStack(unsigned int size=1024) {
        stack_size = size;
        // Stack at end
        SP = mem_size - sizeof(RegType);
    }

    void push(RegType val) {
        SP -= sizeof(RegType);
        setAddress(SP, val);
    }

    RegType pop() {
        RegType res = getAddress(SP);
        SP += sizeof(RegType);
        return res;
    }

    void setupInterruptVector(RegType addr=0) {
        if (addr == 0) {
            if (SP == 0) setupStack();
            IP = SP - sizeof(RegType) * stack_size;
        } else {
            IP = addr;
        }
    }

    void setAddress(RegType base, RegType addr) {
        for (unsigned int i = 0; i < 64/8; ++i) {
            mem[base + 64/8 - i] = (addr >> (i*8)) & 0xFF;
        }
    }

    void setupInterrupt(ByteType num, RegType addr) {
        setAddress(IP + num * sizeof(RegType), addr);
    }

    RegType getAddress(RegType base) {
        RegType res = 0;
        for (unsigned int i = 0; i < 64/8; ++i) {
            res |= mem[base + i];
            res <<= 8;
        }
        return res;
    }

    RegType getInterrupt(ByteType num) {
        return getAddress(IP + num * sizeof(RegType));
    }

    void error(std::string message) {
        std::cerr << "ERROR @" << PC << ": " << message << "\n";
        dump();
        exit(1);
    }

    void dumpval(RegType val) {
        std::cout << "=" << std::setw(16) << std::setfill('0') << val;
    }

    void dumpnamedreg(std::string name, RegType val, bool newline=true) {
        std::cout << std::setfill(' ') << std::setw(2) << name;
        dumpval(val);
        if (newline) std::cout << "\n";
    }

    void dumpreg(RegType num, RegType val, bool newline=true) {
        std::cout << std::setfill(' ') << std::setw(2) << std::hex << num;
        dumpval(val);
        if (newline) std::cout << "\n";
    }

    bool readFile(std::string name) {
        std::fstream ifs(name.c_str(), std::ios::in | std::ios::binary);
        char c;
        RegType r = 0;
        while (ifs.get(c)) {
            mem[r++] = c;
        }

        return true;
    }

    void dump() {
        std::cout << "\n";
        std::cout << "====================\n";
        dumpnamedreg("PC", PC);
        dumpnamedreg("SP", SP);
        dumpnamedreg("IP", IP);
        dumpnamedreg("CR", CR);
        std::cout << "\n";
        for (RegType i = 0; i < 256; ++i) {
            dumpreg(i, registers[i], false);
            if ((i+1) % 8 == 0) std::cout << "\n";
            else std::cout << " ";
        }
        std::cout << "\n";
    }

    RegType mem_size;
    RegType stack_size;
    ByteType *mem;
    bool interrupts;
    RegType registers[256];
    RegType PC;
    RegType SP;
    RegType IP;
    RegType CR;
};

void decodeOne(World *w, OpType op) {
    std::cout << "OP " << op << "\n";

    w->PC++;
    switch (op) {
        case NOP:
            break;
        case RET:
            w->PC = w->pop();
            break;
        case IL:
            w->interrupts = false;
            break;
        case IU:
            w->interrupts = true;
            break;
        default:
            w->error("1: Unknown instruction: " + std::to_string(op));
            break;
    }
}

void runInterrupt(World *w, RegType intnum) {
    if (intnum == 1) {
        // Special system interrupt
        switch (w->registers[0]) {
            case 1:
                // Putchar
                std::cout << (char)(w->registers[1] & 0xFF);
                break;
            default:
                w->error("Invalid interrupt command at INT 0x1: " +
                    std::to_string(w->registers[0]));
        }
    } else {
        //Return from int with RET
        w->push(w->PC);
        w->PC = w->getInterrupt(intnum);
    }
}

void decodeTwo(World *w, OpType op) {
    ByteType reg1 = w->mem[w->PC + 1];
    w->PC += 2;
    switch (op) {
        case INT:
            runInterrupt(w, reg1);
            break;
        case LPC:
            w->registers[reg1] = w->PC;
            break;
        case LSP:
            w->registers[reg1] = w->SP;
            break;
        case LIP:
            w->registers[reg1] = w->IP;
            break;
        case LCR:
            w->registers[reg1] = w->CR;
            break;
        case NOT:
            w->registers[reg1] = ~w->registers[reg1];
            break;
        case PUS:
            w->push(w->registers[reg1]);
            break;
        case POP:
            w->registers[reg1] = w->pop();
            break;
        case SIP:
            w->IP = w->registers[reg1];
            break;
        case SSP:
            w->SP = w->registers[reg1];
            break;
        case SCR:
            w->CR = w->registers[reg1];
            break;
        default:
            w->error("2: Unknown instruction: " + std::to_string(op));
            break;
    }
}

void decodeThree(World *w, OpType op) {
    ByteType reg1 = w->mem[w->PC + 1];
    ByteType reg2 = w->mem[w->PC + 2];
    w->PC += 3;

    switch (op) {
        case L:
            w->registers[reg1] = w->registers[reg2];
            break;
        case LS:
            w->registers[reg1] = w->mem[w->registers[reg2]] & 0xFF;
            break;
        case ST:
            w->mem[w->registers[reg2]] = w->registers[reg1] & 0xFF;
            break;
        case A:
            w->registers[reg1] = (RegTypeS)w->registers[reg1] + (RegTypeS)w->registers[reg2];
            break;
        case AU:
            w->registers[reg1] += w->registers[reg2];
            break;
        case S:
            w->registers[reg1] = (RegTypeS)w->registers[reg1] - (RegTypeS)w->registers[reg2];
            break;
        case SU:
            w->registers[reg1] -= w->registers[reg2];
            break;
        case M:
            w->registers[reg1] = (RegTypeS)w->registers[reg1] * (RegTypeS)w->registers[reg2];
            break;
        case MU:
            w->registers[reg1] *= w->registers[reg2];
            break;
        case AND:
            w->registers[reg1] &= w->registers[reg2];
            break;
        case OR:
            w->registers[reg1] |= w->registers[reg2];
            break;
        case XOR:
            w->registers[reg1] ^= w->registers[reg2];
            break;
        case B:
        case BAS:
            {
                RegType val = w->registers[reg1];
                if (w->CR & val == val) {
                    if (op == BAS)
                        w->push(w->PC);
                    w->PC = w->registers[reg2];
                }
                break;
            }
        case CP:
            if ((RegTypeS)w->registers[reg1] < (RegTypeS)w->registers[reg2]) {
                w->CR &= CRLessThan;
            }
            else if ((RegTypeS)w->registers[reg1] > (RegTypeS)w->registers[reg2]) {
                w->CR &= CRGreaterThan;
            }
            else if ((RegTypeS)w->registers[reg1] == (RegTypeS)w->registers[reg2]) {
                w->CR &= CREqual;
            }
            break;
        case CPU:
            if (w->registers[reg1] < w->registers[reg2]) {
                w->CR &= CRLessThan;
            }
            else if (w->registers[reg1] > w->registers[reg2]) {
                w->CR &= CRGreaterThan;
            }
            else if (w->registers[reg1] == w->registers[reg2]) {
                w->CR &= CREqual;
            }
            break;
        case SHL:
            w->registers[reg1] <<= w->registers[reg2];
            break;
        case SHR:
            w->registers[reg1] >>= w->registers[reg2];
            break;
        default:
            w->error("3: Unknown instruction: " + std::to_string(op));
            break;
    }
}

void decodeFour(World *w, OpType op) {
    ByteType reg1 = w->mem[w->PC + 1];
    ByteType reg2 = w->mem[w->PC + 2];
    ByteType reg3 = w->mem[w->PC + 3];
    w->PC += 4;

    switch (op) {
        case D:
            {
                RegTypeS a = w->registers[reg1];
                RegTypeS b = w->registers[reg2];
                w->registers[reg1] = a / b;
                w->registers[reg3] = a % b;
            }
            break;
        case DU:
            {
                RegType a = w->registers[reg1];
                RegType b = w->registers[reg2];
                w->registers[reg1] = a / b;
                w->registers[reg3] = a % b;
            }
            break;
        case BAL:
            {
                RegType val = w->registers[reg1];
                if (w->CR & val == val) {
                    w->registers[reg3] = w->PC;
                    w->PC = w->registers[reg2];
                }
                break;
            }
            break;
        case LSM:
            {
                RegType val = w->registers[reg3];
                for (unsigned int i = reg1; i <= reg2; i++) {
                    // FIXME val increment?
                    w->registers[i] = w->getAddress(val);
                }
            }
            break;
        case STM:
            {
                RegType val = w->registers[reg3];
                for (unsigned int i = reg1; i <= reg2; i++) {
                    // FIXME val increment?
                    w->setAddress(val, w->registers[i]);
                }
            }
            break;
        case LUM:
            {
                RegType addr = w->registers[reg3];
                RegType val = w->registers[reg2];
                RegType mask = w->registers[reg1];
                val &= ~mask;
                val |= w->getAddress(val & mask);
                w->registers[reg2] = val;
            }
            break;
        case SUM:
            {
                RegType addr = w->registers[reg3];
                RegType val = w->registers[reg2];
                RegType val2 = w->getAddress(addr);
                RegType mask = w->registers[reg1];

                val &= ~mask;
                val |= val & mask;
                w->setAddress(addr, val);
            }
            break;
        default:
            w->error("4: Unknown instruction: " + std::to_string(op));
            break;
    }
}

void decodeImm(World *w, OpType op) {
    ByteType reg1 = w->mem[w->PC + 1];
    w->PC += 2;
    RegType imm = 0;
    for (RegType i = 0; i < 8; ++i) {
        imm |= w->mem[w->PC + i];
        if (i < 7) imm <<= 8;
    }
    w->PC += 8;

    switch (op) {
        case LI:
            w->registers[reg1] = imm;
            break;
        default:
            w->error("5: Unknown instruction: " + std::to_string(op));
            break;
    }
}

void decode(World *w) {
    while (w->PC < w->mem_size) {
        ByteType inst = w->mem[w->PC];

        ByteType mask = inst & AllMask;

        // Special halt
        if (inst == 0) break;

        else if (mask == OneMask)
            decodeOne(w, (OpType)inst);
        else if (mask == TwoMask)
            decodeTwo(w, (OpType)inst);
        else if (mask == FourMask)
            decodeFour(w, (OpType)inst);
        else if (mask == ImmMask)
            decodeImm(w, (OpType)inst);
        else
            decodeThree(w, (OpType)inst);
    }
}

int main(int argc, char **argv) {
    // 5 MB
    unsigned long long memsize = 1024 * 1024 * 5;

    World *w = new World(memsize);
    w->setupStack();
    w->setupInterruptVector();

    if (argc > 1) {
        w->readFile(argv[1]);
    }

    decode(w);

    //w->dump();
}
