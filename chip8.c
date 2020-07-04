/*
 * chip8 - CHIP8 Interpreter
 *
 * Mike Mallin, 2020
 */

#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
/* For byteswapping utilities */
#include <arpa/inet.h>

#include "chip8.h"
#include "chip8_utils.h"

#define OPC_REGX(_op)   ((_op & 0x0F00) >> 8)
#define OPC_REGY(_op)   ((_op & 0x00F0) >> 4)
#define OPC_N(_op)      (_op & 0x000F)
#define OPC_NN(_op)     (_op & 0x00FF)
#define OPC_NNN(_op)    (_op & 0x0FFF)

#if 0
#define INTERPRETER_TRACE(...) (printf( __VA_ARGS__ ))
#else
#define INTERPRETER_TRACE(...)
#endif

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
#define U16_MEMORY_WRITE(_addr, val) (*((uint16_t *)&s_memory[_addr]) = (val))

/* 16, 8-bit V registers */
#define NUM_V_REGISTERS 16
static uint8_t s_v_regs[NUM_V_REGISTERS];

/* 16-bit I register */
static uint16_t s_i_reg;

/* 16-bit program counter */
static uint16_t s_pc;

/* 16-bit stack pointer */
static uint16_t s_stack_ptr;

/* Set to true if execution is paused.
 * Mainly used for opcode LD Vx, K */
static bool s_execution_paused_for_key_ld = false;

/* Timers */

/* Input */

/* Graphics buffer */
static uint8_t s_vram[BITS2BYTES(DISPLAY_WIDTH_PIXELS)]
                     [BITS2BYTES(DISPLAY_HEIGHT_PIXELS)];

/* Sprites are loaded to the start of memory,
 * into the interpreter reserved area (0x0 - 0x1FF)
 */
#define SPRITE_LOAD_ADDR    0

/* Gets the address in memory of the given sprite */
#define SPRITE_ADDR(_char)  (SPRITE_LOAD_ADDR + (_char * 5))
static uint8_t s_character_sprite_data[] = {
    0xF0, /* **** */
    0x90, /* *  * */
    0x90, /* *  * */
    0x90, /* *  * */
    0xF0, /* **** */
/******************/
    0x20, /*   *  */
    0x60, /*  **  */
    0x20, /*   *  */
    0x20, /*   *  */
    0x70, /*  *** */
/******************/
    0xF0, /* **** */
    0x10, /*    * */
    0xF0, /* **** */
    0x80, /* *    */
    0xF0, /* **** */
/******************/
    0xF0, /* **** */
    0x10, /*    * */
    0xF0, /* **** */
    0x10, /*    * */
    0xF0, /* **** */
/******************/
    0x90, /* *  * */
    0x90, /* *  * */
    0xF0, /* **** */
    0x10, /*    * */
    0x10, /*    * */
/******************/
    0xF0, /* **** */
    0x80, /* *    */
    0xF0, /* **** */
    0x10, /*    * */
    0xF0, /* **** */
/******************/
    0xF0, /* **** */
    0x80, /* *    */
    0xF0, /* **** */
    0x90, /* *  * */
    0xF0, /* **** */
/******************/
    0xF0, /* **** */
    0x10, /*    * */
    0x20, /*   *  */
    0x40, /*  *   */
    0x40, /*  *   */
/******************/
    0xF0, /* **** */
    0x90, /* *  * */
    0xF0, /* **** */
    0x90, /* *  * */
    0xF0, /* **** */
/******************/
    0xF0, /* **** */
    0x90, /* *  * */
    0xF0, /* **** */
    0x10, /*    * */
    0xF0, /* **** */
/******************/
    0xF0, /* **** */
    0x90, /* *  * */
    0xF0, /* **** */
    0x90, /* *  * */
    0x90, /* *  * */
/******************/
    0xE0, /* ***  */
    0x90, /* *  * */
    0xE0, /* ***  */
    0x90, /* *  * */
    0xE0, /* ***  */
/******************/
    0xF0, /* **** */
    0x80, /* *    */
    0x80, /* *    */
    0x80, /* *    */
    0xF0, /* **** */
/******************/
    0xE0, /* ***  */
    0x90, /* *  * */
    0x90, /* *  * */
    0x90, /* *  * */
    0xE0, /* ***  */
/******************/
    0xF0, /* **** */
    0x80, /* *    */
    0xF0, /* **** */
    0x80, /* *    */
    0xF0, /* **** */
/******************/
    0xF0, /* **** */
    0x80, /* *    */
    0xF0, /* **** */
    0x80, /* *    */
    0x80, /* *    */
};

static void
clear_display (void)
{
    memset(s_vram, 0, sizeof(s_vram));
}

static void
stack_push (uint16_t val)
{
    INTERPRETER_TRACE("Stack push: SP: 0x%x - 0x%x\n", s_stack_ptr, val);
    U16_MEMORY_WRITE(s_stack_ptr, val);
    s_stack_ptr -= 2;
    INTERPRETER_TRACE("Stack push: SP: 0x%x - 0x%x\n", s_stack_ptr, val);
}

static uint16_t
stack_pop (void)
{
    uint16_t ret = 0;

    INTERPRETER_TRACE("Stack pop: SP: 0x%x - 0x%x\n", s_stack_ptr, ret);
    s_stack_ptr += 2;
    ret = U16_MEMORY_READ(s_stack_ptr);
    INTERPRETER_TRACE("Stack pop: SP: 0x%x - 0x%x\n", s_stack_ptr, ret);

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
        case 0x00E0: /* CLS */
            clear_display();
            break;
        case 0x00EE: /* RET */
            /* Return from a subroutine.
             * The interpreter sets the program counter to the address at the
             * top of the stack, then subtracts 1 from the stack pointer.
             */
            s_pc = stack_pop();
            break;
    }
}

static void
chip8_interpret_op1 (uint16_t op)
{
    /* JP - Jump to location NNN.
     * The interpreter sets the program counter to nnn.
     */
    s_pc = OPC_NNN(op);
}

static void
chip8_interpret_op2 (uint16_t op)
{
    /* CALL - Call subroutine at NNN.
     * The interpreter increments the stack pointer, then puts the current PC
     * on the top of the stack. The PC is then set to nnn.
     */
    stack_push(s_pc);
    s_pc = OPC_NNN(op);
}

static void
chip8_interpret_op3 (uint16_t op)
{
    /* SE Vx, NN
     * Skip next instruction if Vx = NN.
     */
    uint8_t vx  = OPC_REGX(op);
    uint8_t val = OPC_NN(op);

    if (s_v_regs[vx] == val) {
        s_pc += 2;
    }
}

static void
chip8_interpret_op4 (uint16_t op)
{
    /* SNE - Vx, NN
     * Skip next instruction if Vx != NN.
     */
    uint8_t vx  = OPC_REGX(op);
    uint8_t val = OPC_NN(op);

    if (s_v_regs[vx] != val) {
        s_pc += 2;
    }
}

static void
chip8_interpret_op5 (uint16_t op)
{
    /* SE Vx, Vy
     * Skip next instruction if Vx = Vy.
     */
    uint8_t vx = OPC_REGX(op);
    uint8_t vy = OPC_REGY(op);

    if (s_v_regs[vx] == s_v_regs[vy]) {
        s_pc += 2;
    }
}

static void
chip8_interpret_op6 (uint16_t op)
{
    /* LD Vx, NN
     * Set Vx = NN.
     */
    s_v_regs[OPC_REGX(op)] = OPC_NN(op);
}

static void
chip8_interpret_op7 (uint16_t op)
{
    /* ADD Vx, NN
     * Set Vx = Vx + kk.
     */
    s_v_regs[OPC_REGX(op)] += OPC_NN(op);
}

static void
chip8_interpret_op8 (uint16_t op)
{
    switch (op & 0xF) {
        default:
            assert(false);
        case 0: /* LD Vx, Vy - Set Vx = Vy. */
            s_v_regs[OPC_REGX(op)] = s_v_regs[OPC_REGY(op)];
            break;
        case 1: /* OR Vx, Vy - Set Vx = Vx OR Vy. */
            s_v_regs[OPC_REGX(op)] =
                s_v_regs[OPC_REGX(op)] | s_v_regs[OPC_REGY(op)];
            break;
        case 2: /* AND Vx, Vy - Set Vx = Vx AND Vy. */
            s_v_regs[OPC_REGX(op)] =
                s_v_regs[OPC_REGX(op)] & s_v_regs[OPC_REGY(op)];
            break;
        case 3: /* XOR Vx, Vy - Set Vx = Vx XOR Vy. */
            s_v_regs[OPC_REGX(op)] =
                s_v_regs[OPC_REGX(op)] ^ s_v_regs[OPC_REGY(op)];
            break;
        case 4: /* ADD Vx, Vy - Set Vx = Vx + Vy, set VF = carry. */
            {
                uint16_t tmp =
                    (uint16_t)(s_v_regs[OPC_REGX(op)]) +
                    (uint16_t)(s_v_regs[OPC_REGY(op)]);
                s_v_regs[OPC_REGX(op)] = tmp & 0xFF;
                /* Detect carry into VF */
                s_v_regs[0xF] = ((tmp & 0x100) >> 8);
            }
            break;
        case 5: /* SUB Vx, Vy - Set Vx = Vx - Vy, set VF = NOT borrow. */
            s_v_regs[0xF] =
                (s_v_regs[OPC_REGX(op)] > s_v_regs[OPC_REGY(op)]) & 0x1;
            s_v_regs[OPC_REGX(op)] =
                s_v_regs[OPC_REGX(op)] - s_v_regs[OPC_REGY(op)];
            break;
        case 6: /* SHR Vx - Set Vx = Vx SHR 1. */
            s_v_regs[0xF] = s_v_regs[OPC_REGX(op)] & 0x1;
            s_v_regs[OPC_REGX(op)] = s_v_regs[OPC_REGX(op)] >> 1;
            break;
        case 7: /* SUBN Vx, Vy - Set Vx = Vy - Vx, set VF = NOT borrow. */
            s_v_regs[0xF] =
                (s_v_regs[OPC_REGY(op)] > s_v_regs[OPC_REGX(op)]) & 0x1;
            s_v_regs[OPC_REGX(op)] =
                s_v_regs[OPC_REGY(op)] - s_v_regs[OPC_REGX(op)];
            break;
        case 0xE: /* SHL Vx - Set Vx = Vx SHL 1. */
            s_v_regs[0xF] = ((s_v_regs[OPC_REGX(op)] & 0x80) != 0);
            s_v_regs[OPC_REGX(op)] = s_v_regs[OPC_REGX(op)] << 1;
            break;
    }
}

static void
chip8_interpret_op9 (uint16_t op)
{
    /* SNE Vx, Vy
     * Skip next instruction if Vx != Vy.
     */
    assert((op & 0xF) == 0);

    if (s_v_regs[OPC_REGX(op)] != s_v_regs[OPC_REGY(op)]) {
        s_pc += 2;
    }
}

static void
chip8_interpret_opA (uint16_t op)
{
    /* LD I, NNN
     * Set I = NNN.
     */
    s_i_reg = OPC_NNN(op);
}

static void
chip8_interpret_opB (uint16_t op)
{
    /* JP V0, NNN
     * Jump to location NNN + V0.
     */
    s_i_reg = (uint16_t)OPC_NNN(op) + (uint16_t)s_v_regs[0];
}

static void
chip8_interpret_opC (uint16_t op)
{
    /* RND Vx, NN
     * Set Vx = random byte AND NN.
     */
    s_v_regs[OPC_REGX(op)] = get_random_byte() & OPC_NN(op);
}

static void
chip8_interpret_opD (uint16_t op)
{
    /* DRW Vx, Vy, N
     * Display N-byte sprite starting at memory location I at (Vx, Vy),
     * set VF = collision.
     */
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
        s_v_regs[0xF] |= (previous_sprite > s_vram[x][y]) & 0x1;
        /* Move to the next line on screen. */
        y++;
        y = y % DISPLAY_HEIGHT_PIXELS;
    }
}

static void
chip8_interpret_opE (uint16_t op)
{
    switch (OPC_NN(op)) {
        default:
            assert(false);
        case 0x9E: /* SKP Vx */
            if (get_key_pressed(s_v_regs[OPC_REGX(op)])) {
                s_pc += 2;
            }
            break;
        case 0xA1: /* SKNP Vx */
            if (!get_key_pressed(s_v_regs[OPC_REGX(op)])) {
                s_pc += 2;
            }
            break;
    }
}

void
chip8_notify_key_pressed (chip8_key_et key)
{
    uint16_t key_opcode = U16_MEMORY_READ(s_pc - 2);

    if (s_execution_paused_for_key_ld) {
        s_v_regs[OPC_REGX(key_opcode)] = key;
        s_execution_paused_for_key_ld = false;
    }
}

static void
chip8_interpret_opF (uint16_t op)
{
    switch (OPC_NN(op)) {
        default:
            assert(false);
        case 0x07: /* LD Vx, DT */
            s_v_regs[OPC_REGX(op)] = get_delay_timer_remaining();
            break;
        case 0x0A: /* LD Vx, K */
            s_execution_paused_for_key_ld = true;
            break;
        case 0x15: /* LD DT, Vx */
            set_delay_timer(s_v_regs[OPC_REGX(op)]);
            break;
        case 0x18: /* LD ST, Vx */
            set_sound_timer(s_v_regs[OPC_REGX(op)]);
            break;
        case 0x1E: /* ADD I , Vx */
            s_i_reg = s_i_reg + s_v_regs[OPC_REGX(op)];
            break;
        case 0x29: /* LD F, Vx */
            assert(s_v_regs[OPC_REGX(op)] <= 0xF);
            s_i_reg = SPRITE_ADDR(s_v_regs[OPC_REGX(op)]);
            break;
        case 0x33: /* LD B, Bx */
            {
                uint8_t val = s_v_regs[OPC_REGX(op)];
                s_memory[s_i_reg] = val / 100;
                s_memory[s_i_reg + 1] = (val / 10) % 10;
                s_memory[s_i_reg + 2] = val % 10;
            }
            break;
        case 0x55: /* LD [I], Vx */
            memcpy(&s_memory[s_i_reg], s_v_regs,
                   OPC_REGX(op) + sizeof(s_v_regs[0]));
            break;
        case 0x65: /* LD Vx, [I] */
            memcpy(s_v_regs, &s_memory[s_i_reg],
                   OPC_REGX(op) + sizeof(s_v_regs[0]));
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
    chip8_interpret_opE,
    chip8_interpret_opF,
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

    INTERPRETER_TRACE("PC: 0x%x - 0x%x\n", s_pc, U16_MEMORY_READ(s_pc));

    if (!s_execution_paused_for_key_ld) {
        /* Increment PC for next instruction */
        s_pc += 2;
        chip8_interpret_op(op);
    }
}

uint8_t *
chip8_get_vram (void)
{
    return &s_vram[0][0];
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
    memcpy(&s_memory[SPRITE_LOAD_ADDR], s_character_sprite_data,
           sizeof(s_character_sprite_data));
}

static bool
need_to_byteswap_image (void)
{
    if (htonl(47) == 47) {
        /* Big endian - No change needed for CHIP8*/
        return false;
    } else {
        /* Little endian - Need to byteswap the loaded program */
        return true;
    }
}

static void
byteswap_image (size_t program_size)
{
    size_t i;
    uint16_t instruction;

    for (i = 0; i < program_size; i += 2) {
        instruction = U16_MEMORY_READ(PROGRAM_LOAD_ADDR + i);
        instruction = htons(instruction);
        U16_MEMORY_WRITE(PROGRAM_LOAD_ADDR + i, instruction);
    }
}

void
chip8_load_program (char *file_path)
{
    FILE *fp = fopen(file_path, "r");
    size_t file_size = 0;
    size_t total_bytes_read = 0;
    size_t bytes_read = 0;

    if (fp == NULL) {
        printf("Unable to open file %s - %s\n", file_path, strerror(errno));
        exit(1);
    }

    fseek(fp, 0L, SEEK_END);
    file_size = ftell(fp);

    fseek(fp, 0L, SEEK_SET);
    printf("Loading %lu bytes from %s\n", file_size, file_path);

    while (total_bytes_read < file_size) {
        bytes_read = fread(&s_memory[PROGRAM_LOAD_ADDR], 1,
                           file_size - total_bytes_read, fp);
        if (bytes_read == 0) {
            printf("Unable to read more data from file\n");
            break;
        }

        total_bytes_read += bytes_read;
    }

    printf("Loaded program into memory\n");

    if (need_to_byteswap_image()) {
        printf("LE architecture detected, byteswapping image...");
        byteswap_image(total_bytes_read);
        printf(" Done!\n");
    }

    fclose(fp);
}
