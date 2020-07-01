/*
 * chip8 - CHIP8 Interpreter
 *
 * Mike Mallin, 2020
 */

#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#include "chip8_utils.h"

#define BITS2BYTES(_bits) (_bits / 8)

#define OPC_REGX(_op)   ((_op & 0x0F00) >> 8)
#define OPC_REGY(_op)   ((_op & 0x00F0) >> 4)
#define OPC_N(_op)      (_op & 0x000F)
#define OPC_NN(_op)     (_op & 0x00FF)
#define OPC_NNN(_op)    (_op & 0x0FFF)

/* Information from https://en.wikipedia.org/wiki/CHIP-8 */
#define PROGRAM_LOAD_ADDR       0x200
#define STACK_END_ADDR          0xEA0
/* 0xEFF is the last valid address in the stack, but because the
 * stack stores 16-bit pointers we start at 0xEFE for alignment and to
 * not overwrite past the stack boundaries */
#define STACK_BASE_ADDR         0xEFE
#define DISPLAY_REFRESH_ADDR    0xF00
#define MEMORY_SIZE             0x1000

/* Memory space of the CHIP-8 */
static uint8_t s_memory[MEMORY_SIZE];
#define U16_MEMORY_READ(_addr) (*(uint16_t *)&s_memory[_addr])
#define U16_MEMORY_WRITE(_addr, val) (*(uint16_t *)&s_memory[_addr] = val)

/* 16, 8-bit V registers */
#define NUM_V_REGISTERS 16
static uint8_t s_v_regs[NUM_V_REGISTERS];

/* 16-bit I register */
static uint16_t s_i_reg;

/* 16-bit program counter */
static uint16_t s_pc;

/* 16-bit stack pointer */
static uint16_t s_stack_ptr;

/* Timers */

/* Input */

/* Graphics buffer */
#define DISPLAY_WIDTH_PIXELS    64
#define DISPLAY_HEIGHT_PIXELS   32
static uint8_t s_vram[BITS2BYTES(DISPLAY_WIDTH_PIXELS)]
                     [BITS2BYTES(DISPLAY_HEIGHT_PIXELS)];

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
    uint8_t vx  = OPC_REGX(op);
    uint8_t val = OPC_NN(op);

    if (s_v_regs[vx] == val) {
        s_pc += 2;
    }
}

static void
chip8_interpret_op4 (uint16_t op)
{
    uint8_t vx  = OPC_REGX(op);
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
        case 5: /* 08XY5 */
            if (s_v_regs[OPC_REGX(op)] > s_v_regs[OPC_REGY(op)]) {
                s_v_regs[0xF] = 1;
            } else {
                s_v_regs[0xF] = 0;
            }
            s_v_regs[OPC_REGX(op)] = s_v_regs[OPC_REGX(op)] - s_v_regs[OPC_REGY(op)];
            break;
        case 6: /* 0x8XY6 */
            s_v_regs[0xF] = s_v_regs[OPC_REGX(op)] & 0x1;
            s_v_regs[OPC_REGX(op)] = s_v_regs[OPC_REGX(op)] >> 1;
            break;
        case 7: /* 08XY7 */
            if (s_v_regs[OPC_REGY(op)] > s_v_regs[OPC_REGX(op)]) {
                s_v_regs[0xF] = 1;
            } else {
                s_v_regs[0xF] = 0;
            }
            s_v_regs[OPC_REGX(op)] = s_v_regs[OPC_REGY(op)] - s_v_regs[OPC_REGX(op)];
            break;
        case 0xE: /* 0x8XYE */
            s_v_regs[0xF] = ((s_v_regs[OPC_REGX(op)] & 0x80) != 0);
            s_v_regs[OPC_REGX(op)] = s_v_regs[OPC_REGX(op)] << 1;
            break;
    }
}

static void
chip8_interpret_op9 (uint16_t op)
{
    assert((op & 0xF) == 0);

    if (s_v_regs[OPC_REGX(op)] != s_v_regs[OPC_REGY(op)]) {
        s_pc += 2;
    }
}

static void
chip8_interpret_opA (uint16_t op)
{
    s_i_reg = OPC_NNN(op);
}

static void
chip8_interpret_opB (uint16_t op)
{
    s_i_reg = (uint16_t)OPC_NNN(op) + (uint16_t)s_v_regs[0];
}

static void
chip8_interpret_opC (uint16_t op)
{
    s_v_regs[OPC_REGX(op)] = get_random_byte() & OPC_NN(op);
}

static void
chip8_interpret_opD (uint16_t op)
{
    uint8_t x           = s_v_regs[OPC_REGX(op)];
    uint8_t y           = s_v_regs[OPC_REGY(op)];
    uint8_t num_bytes   = OPC_N(op);
    int     i;
    uint8_t previous_sprite;

    /* Start by assuming no pixels will be erased */
    s_v_regs[0xF] = 0;

    for (i = 0; i < num_bytes; i++) {
        /* Store the previous sprite */
        previous_sprite = s_vram[x][y];
        /* XOR the new byte of the sprite on the screen */
        s_vram[x][y] ^= s_memory[s_i_reg];
        /* The following sets VF without a branch, only a comparison.
         * Let's say we have the following byte in VRAM to start
         * 0x8A (10001010). If the new byte coming in clears any of
         * those bits, the resulting byte in VRAM will always be
         * less than the previous value. We can use that comparison
         * to set the bit in VF indicating that a pixel was erased.
         */
        s_v_regs[0xF] |= (previous_sprite > s_vram[x][y]);
        /* Move to the next line on screen. */
        y++;
        y = y % DISPLAY_HEIGHT_PIXELS;
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
    chip8_interpret_op9,
    chip8_interpret_opA,
    chip8_interpret_opB,
    chip8_interpret_opC,
    chip8_interpret_opD,
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
