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

/* Information from https://en.wikipedia.org/wiki/CHIP-8 */
#define PROGRAM_LOAD_ADDR       0x200
#define STACK_END_ADDR          0xEA0
#define STACK_BASE_ADDR         0xEFF
#define DISPLAY_REFRESH_ADDR    0xF00
#define MEMORY_SIZE             0x1000

/* Memory space of the CHIP-8 */
static uint8_t s_memory[MEMORY_SIZE];

/* 16, 8-bit V registers */
static uint8_t s_v_regs[16];

/* 16-bit I register */
static uint16_t s_i_reg;

/* 16-bit program counter */
static uint16_t s_pc;

/* Timers */

/* Input */

/* Graphics buffer */
#define DISPLAY_WIDTH   64
#define DISPLAY_HEIGHT  32
static uint8_t s_vram[BITS2BYTES(DISPLAY_WIDTH*DISPLAY_HEIGHT)];

static void
clear_display (void)
{
    memset(s_vram, 0, sizeof(s_vram));
}

/* Opcode decoding */
static void
chip8_interpret_op0 (uint16_t op)
{
    switch (op) {
        default:
            assert(false);
        case 0x00E0:
            clear_display();
            break;
    }
}

typedef void (*op_decoder_t)(uint16_t op);

static op_decoder_t s_opcode_decoder[] = {
    chip8_interpret_op0,
};

static void
chip8_interpret_op (uint16_t op)
{
    uint16_t op_class = (op & 0xF000) >> 12;

    /* Dispatch opcode decoding to the right class of opcode.
     * The uppermost nibble is a fixed constant from 0-F. */
    s_opcode_decoder[op_class](op);
}
