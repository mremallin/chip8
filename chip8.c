/*
 * chip8 - CHIP8 Interpreter
 *
 * Mike Mallin, 2020
 */

#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#define BITS2BYTES(_bits) (_bits / 8)

#define OPC_REGX(_op)   ((_op & 0x0F00) >> 8)
#define OPC_REGY(_op)   ((_op & 0x00F0) >> 4)
#define OPC_NN(_op)     (_op & 0x00FF)
#define OPC_NNN(_op)    (_op & 0x0FFF)

/* Information from https://en.wikipedia.org/wiki/CHIP-8 */
#define PROGRAM_LOAD_ADDR       0x200
#define STACK_END_ADDR          0xEA0
/* 0xEFF is the last valid address in the stack, but because the
 * stack stores 16-bit pointers we start at 0xEFE to not overwrite
 * past the stack boundaries */
#define STACK_BASE_ADDR         0xEFE
#define DISPLAY_REFRESH_ADDR    0xF00
#define MEMORY_SIZE             0x1000

/* Memory space of the CHIP-8 */
static uint8_t s_memory[MEMORY_SIZE];
#define U16_MEMORY_READ(_addr) (*(uint16_t *)&s_memory[_addr])
#define U16_MEMORY_WRITE(_addr, val) (*(uint16_t *)&s_memory[_addr] = val)

/* 16, 8-bit V registers */
static uint8_t s_v_regs[16];

/* 16-bit I register */
static uint16_t s_i_reg;

/* 16-bit program counter */
static uint16_t s_pc;

/* 16-bit stack pointer */
static uint16_t s_stack_ptr;

/* Timers */

/* Input */

/* Graphics buffer */
#define DISPLAY_WIDTH   64
#define DISPLAY_HEIGHT  32
static uint8_t s_vram[BITS2BYTES(DISPLAY_WIDTH * DISPLAY_HEIGHT)];

static void
clear_display (void)
{
    memset(s_vram, 0, sizeof(s_vram));
}

static void
stack_push (uint16_t val)
{
    U16_MEMORY_WRITE(s_stack_ptr, val);
    s_stack_ptr -= 2;
}

static uint16_t
stack_pop (void)
{
    uint16_t ret = U16_MEMORY_READ(s_stack_ptr);
    s_stack_ptr += 2;
    return (ret);
}

/* Opcode decoding */
static void
chip8_interpret_op0 (uint16_t op)
{
    switch (op) {
        default:
            /* Would call machine-code routine at the given address */
            assert(false);
        case 0x00E0:
            clear_display();
            break;
        case 0x00EE:
            s_pc = stack_pop();
            break;
    }
}

static void
chip8_interpret_op1 (uint16_t op)
{
    s_pc = OPC_NNN(op);
}

static void
chip8_interpret_op2 (uint16_t op)
{
    stack_push(s_pc);
    s_pc = OPC_NNN(op);
}

static void
chip8_interpret_op3 (uint16_t op)
{
    uint8_t vx = OPC_REGX(op);
    uint8_t val = OPC_NN(op);

    if (s_v_regs[vx] == val) {
        s_pc += 2;
    }
}

static void
chip8_interpret_op4 (uint16_t op)
{
    uint8_t vx = OPC_REGX(op);
    uint8_t val = OPC_NN(op);

    if (s_v_regs[vx] != val) {
        s_pc += 2;
    }
}

static void
chip8_interpret_op5 (uint16_t op)
{
    uint8_t vx = OPC_REGX(op);
    uint8_t vy = OPC_REGY(op);

    if (s_v_regs[vx] == s_v_regs[vy]) {
        s_pc += 2;
    }
}

static void
chip8_interpret_op6 (uint16_t op)
{
    s_v_regs[OPC_REGX(op)] = OPC_NN(op);
}

static void
chip8_interpret_op7 (uint16_t op)
{
    s_v_regs[OPC_REGX(op)] += OPC_NN(op);
}

static void
chip8_interpret_op8 (uint16_t op)
{
    switch (op & 0xF) {
        default:
            assert(false);
        case 0: /* 0x8XY0 */
            s_v_regs[OPC_REGX(op)] = s_v_regs[OPC_REGY(op)];
            break;
        case 1: /* 0x8XY1 */
            s_v_regs[OPC_REGX(op)] = s_v_regs[OPC_REGX(op)] | s_v_regs[OPC_REGY(op)];
            break;
        case 2: /* 0x8XY2 */
            s_v_regs[OPC_REGX(op)] = s_v_regs[OPC_REGX(op)] & s_v_regs[OPC_REGY(op)];
            break;
        case 3: /* 0x8XY3 */
            s_v_regs[OPC_REGX(op)] = s_v_regs[OPC_REGX(op)] ^ s_v_regs[OPC_REGY(op)];
            break;
        case 4: /* 0x8XY4 */
            {
                uint16_t tmp = (uint16_t)(s_v_regs[OPC_REGX(op)]) + (uint16_t)(s_v_regs[OPC_REGY(op)]);
                s_v_regs[OPC_REGX(op)] = tmp & 0xFF;
                /* Detect carry into VF */
                s_v_regs[0xF] = ((tmp & 0x100) >> 8);
            }
            break;
    }
}

/* Dispatch table for different opcode types, upper-most nibble */
typedef void (*op_decoder_t)(uint16_t op);

static op_decoder_t s_opcode_decoder[] = {
    chip8_interpret_op0,
    chip8_interpret_op1,
    chip8_interpret_op2,
    chip8_interpret_op3,
    chip8_interpret_op4,
    chip8_interpret_op5,
    chip8_interpret_op6,
    chip8_interpret_op7,
    chip8_interpret_op8,
};

static void
chip8_interpret_op (uint16_t op)
{
    uint16_t op_class = (op & 0xF000) >> 12;

    /* Dispatch opcode decoding to the right class of opcode.
     * The uppermost nibble is a fixed constant from 0-F. */
    s_opcode_decoder[op_class](op);
}

void
chip8_step (void)
{
    uint16_t op = U16_MEMORY_READ(s_pc);

    /* Increment PC for next instruction */
    s_pc += 2;
    chip8_interpret_op(op);
}

void
chip8_init (void)
{
    memset(s_memory, 0, sizeof(s_memory));
    memset(s_v_regs, 0, sizeof(s_v_regs));
    s_i_reg = 0;
    s_pc = PROGRAM_LOAD_ADDR;
    s_stack_ptr = STACK_BASE_ADDR;
    memset(s_vram, 0, sizeof(s_vram));
}
