#!/usr/bin/env python

import io
import sys

opcodes = {
    'NOP': (0xF1, ''),
    'RET': (0xF2, ''),
    'IL': (0xF3, ''),
    'IU': (0xF4, ''),

    'INT': (0x81, 'R'),
    'LPC': (0x82, 'R'),
    'LSP': (0x83, 'R'),
    'LIP': (0x84, 'R'),
    'LCR': (0x85, 'R'),
    'NOT': (0x86, 'R'),
    'PUS': (0x87, 'R'),
    'POP': (0x88, 'R'),
    'SIP': (0x89, 'R'),
    'SSP': (0x8A, 'R'),
    'SCR': (0x8B, 'R'),

    'L': (0x01, 'RR'),
    'LS': (0x02, 'RR'),
    'ST': (0x03, 'RR'),
    'A': (0x04, 'RR'),
    'AU': (0x05, 'RR'),
    'S': (0x06, 'RR'),
    'SU': (0x07, 'RR'),
    'M': (0x08, 'RR'),
    'MU': (0x09, 'RR'),
    'AND': (0x0A, 'RR'),
    'OR': (0x0B, 'RR'),
    'XOR': (0x0C, 'RR'),
    'B': (0x0D, 'RR'),
    'BAS': (0x0E, 'RR'),
    'CP': (0x0F, 'RR'),
    'CPU': (0x10, 'RR'),
    'SHL': (0x11, 'RR'),
    'SHR': (0x12, 'RR'),

    'D': (0xC1, 'RRR'),
    'DU': (0xC2, 'RRR'),
    'BAL': (0xC3, 'RRR'),
    'LSM': (0xC4, 'RRR'),
    'STM': (0xC5, 'RRR'),
    'LUM': (0xC6, 'RRR'),
    'SUM': (0xC7, 'RRR'),

    'LI': (0xE1,'RI')
}

def parseParams(params, datas):
    res = []
    pdatas = [x.strip() for x in ' '.join(datas).split(',')]
    for i,c in enumerate(params):
        if c == 'R':
            res.append(int(pdatas[i]) & 0xFF)
        elif c == 'I':
            mode = pdatas[i][0]
            if mode != '#' and mode != '$':
                raise ValueError('Expected immediate #num or $num, got: %s' % pdatas[i])

            if mode == '$':
                imm = int(pdatas[i][1:], 16) & 0xFFFFFFFFFFFFFFFF
            else:
                imm = int(pdatas[i][1:]) & 0xFFFFFFFFFFFFFFFF

            tmp = []
            for x in range(0, 8):
                tmp.append(imm & 0xFF)
                imm >>= 8
            res += reversed(tmp)

    return res

def parseOpcode(opcode, datas):
    res = []
    res.append(opcode[0])
    res += parseParams(opcode[1], datas[1:])
    return res


def parseLine(res, line):
    if not line:
        return res

    if line[0] == ';':
        print('Comment: %s' % line)
        return res

    datas = [x.strip() for x in line.split(' ')]
    if not datas:
        return res

    if datas[0] in opcodes:
        res += parseOpcode(opcodes[datas[0]], datas)

    return res

def parse(data):
    res = []
    for l in data:
        res = parseLine(res, l.strip())

    return res

if __name__ == '__main__':
    if len(sys.argv) <= 2:
        print('Usage: %s source.asm dest.bin' % sys.argv[0])
        sys.exit(1)

    with open(sys.argv[1], 'r') as fd:
        data = fd.readlines()

    res = parse(data)

    with open(sys.argv[2], 'wb+') as fd:
        fd.write(bytearray(res))
