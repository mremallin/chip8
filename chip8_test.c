/*
 * chip8 - CHIP8 Interpreter
 *
 * Mike Mallin, 2020
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdio.h>
#include <unistd.h>

#define chip8_interpret_op chip8_interpret_op_bswap

#include "chip8.c"

#undef chip8_interpret_op

void
chip8_interpret_op (uint16_t op)
{
    chip8_interpret_op_bswap(htons(op));
}

static bool s_debug = false;

static void
debug_printf (const char *fmt, ...)
{
    va_list ap;

    if (s_debug) {
        va_start(ap, fmt);
        vprintf(fmt, ap);
        va_end(ap);
    }
}

uint8_t
get_random_byte (void)
{
    function_called();

    /* Guaranteed to be random */
    return 4;
}

static bool s_test_key_is_pressed = false;

bool
get_key_pressed (chip8_key_et key)
{
    function_called();

    return s_test_key_is_pressed;
}

uint8_t
get_delay_timer_remaining (void)
{
    function_called();

    return (42);
}

void
set_delay_timer (uint8_t ticks)
{
    function_called();

    assert_int_equal(45, ticks);
}

void
set_sound_timer (uint8_t ticks)
{
    function_called();

    assert_int_equal(50, ticks);
}

#define DEBUG_PRINTF(fmt, ...) (debug_printf("- "fmt"\n", __VA_ARGS__))

#define BUILD_XNN_OPC(_opc, _x, _nn) (((_opc & 0xF) << 12) | ((_x & 0xF) << 8) | (_nn & 0xFF))
#define BUILD_NNN_OPC(_opc, _nnn) (((_opc & 0xF) << 12) | (_nnn & 0xFFF))

#define LOAD_X(_x, _nn) (chip8_interpret_op(BUILD_XNN_OPC(6, _x, _nn)))
#define LOAD_I(_nnn)    (chip8_interpret_op(BUILD_NNN_OPC(0xA, _nnn)))

static void
opc_00E0 (void **state)
{
    /* Clears the display */
    static uint8_t s_zero[DISPLAY_WIDTH_PIXELS]
                         [DISPLAY_HEIGHT_PIXELS] = {{0}};

    memset(s_vram, 0xFE, sizeof(s_vram));
    chip8_interpret_op(0x00E0);
    assert_memory_equal(s_vram, s_zero, sizeof(s_vram));
}

static void
opc_00EE (void **state)
{
    /* Returns from a subroutine */
    s_stack_ptr = 0xE00;
    *(uint16_t *)&s_memory[s_stack_ptr + 2] = 0xDEAD;
    chip8_interpret_op(0x00EE);
    assert_int_equal(s_pc, 0xDEAD);
    assert_int_equal(s_stack_ptr, 0xE02);
}

static void
opc_1NNN (void **state)
{
    /* Jumps to address NNN. */
    uint16_t i = 0x1000;

    for (; i <= 0x1FFF; i++) {
        chip8_interpret_op(i);
        assert_int_equal(s_pc, OPC_NNN(i));
    }
}

static void
opc_2NNN (void **state)
{
    /* Calls subroutine at NNN. */
    uint16_t i = 0x2000;

    for (; i <= 0x2FFF; i++){
        chip8_interpret_op(i);
        /* Push current PC on stack */
        assert_int_equal(s_stack_ptr, STACK_BASE_ADDR-2);
        /* Load PC with 3 nibbles of the op */
        assert_int_equal(s_pc, OPC_NNN(i));
        /* Return from the subroutine to clean up */
        chip8_interpret_op(0x00EE);
    }
}

static void
opc_3XNN_skip (void **state)
{
    /* Skips the next instruction if VX equals NN.
     * (Usually the next instruction is a jump to skip a code block)  */

    int i = 0;
    uint16_t op;
    memset(&s_v_regs, 0, sizeof(s_v_regs));

    /* Test each register behaves the same */
    for (; i <= NUM_V_REGISTERS; i++) {
        op = BUILD_XNN_OPC(3, i, 00);
        DEBUG_PRINTF("Before - Op: 0x%x, s_pc: 0x%x", op, s_pc);
        chip8_interpret_op(op);
        DEBUG_PRINTF("After - Op: 0x%x, s_pc: 0x%x", op, s_pc);
        /* Matches this time, so PC should be incremented in addition to
         * the normal interpreter step. */
        assert_int_equal(s_pc, 0x202);
        /* Reset for next instruction */
        s_pc = PROGRAM_LOAD_ADDR;
    }
}

static void
opc_3XNN_noskip (void **state)
{
    /* Skips the next instruction if VX equals NN.
     * (Usually the next instruction is a jump to skip a code block)  */

    int i = 0;
    uint16_t op;
    memset(&s_v_regs, 0xDE, sizeof(s_v_regs));

    /* Test each register behaves the same */
    for (; i <= NUM_V_REGISTERS; i++) {
        op = BUILD_XNN_OPC(3, i, 00);
        DEBUG_PRINTF("Before - Op: 0x%x, s_pc: 0x%x", op, s_pc);
        chip8_interpret_op(op);
        DEBUG_PRINTF("After - Op: 0x%x, s_pc: 0x%x", op, s_pc);
        /* Does not match, so PC will not be incremented again */
        assert_int_equal(s_pc, 0x200);
        /* Reset for next instruction */
        s_pc = PROGRAM_LOAD_ADDR;
    }
}

static void
opc_4XNN_skip (void **state)
{
    /* Skips the next instruction if VX doesn't equal NN.
     * (Usually the next instruction is a jump to skip a code block)  */

    int i = 0;
    uint16_t op;
    memset(&s_v_regs, 0, sizeof(s_v_regs));

    /* Test each register behaves the same */
    for (; i <= NUM_V_REGISTERS; i++) {
        op = BUILD_XNN_OPC(4, i, 0xDE);
        DEBUG_PRINTF("Before - Op: 0x%x, s_pc: 0x%x", op, s_pc);
        chip8_interpret_op(op);
        DEBUG_PRINTF("After - Op: 0x%x, s_pc: 0x%x", op, s_pc);
        /* Match, so PC should be incremented again. */
        assert_int_equal(s_pc, 0x202);
        /* Reset for next instruction */
        s_pc = PROGRAM_LOAD_ADDR;
    }
}

static void
opc_4XNN_noskip (void **state)
{
    /* Skips the next instruction if VX doesn't equal NN.
     * (Usually the next instruction is a jump to skip a code block)  */

    int i = 0;
    uint16_t op;
    memset(&s_v_regs, 0, sizeof(s_v_regs));

    /* Test each register behaves the same */
    for (; i <= NUM_V_REGISTERS; i++) {
        op = BUILD_XNN_OPC(4, i, 0);
        DEBUG_PRINTF("Before - Op: 0x%x, s_pc: 0x%x", op, s_pc);
        chip8_interpret_op(op);
        DEBUG_PRINTF("After - Op: 0x%x, s_pc: 0x%x", op, s_pc);
        /* Doesn't match, so PC should not be incremented again. */
        assert_int_equal(s_pc, 0x200);
        /* Reset for next instruction */
        s_pc = PROGRAM_LOAD_ADDR;
    }
}

static void
opc_5XY0_skip (void **state)
{
    /* Skips the next instruction if VX equals VY.
     * (Usually the next instruction is a jump to skip a code block)  */

    uint16_t i = 0x5000;
    uint16_t j = 0x0;
    memset(&s_v_regs, 0, sizeof(s_v_regs));

    /* Test each register behaves the same */
    for (; i <= 0x5FFF; i += 0x100) {
        for (j = 0; j < 0x00F0; j += 0x10) {
            i = (i & 0xFF00) | j;
            DEBUG_PRINTF("Before - Op: 0x%x, s_pc: 0x%x", i, s_pc);
            chip8_interpret_op(i);
            DEBUG_PRINTF("After - Op: 0x%x, s_pc: 0x%x", i, s_pc);
            /* Match, so PC should be incremented again. */
            assert_int_equal(s_pc, 0x202);
            /* Reset for next instruction */
            s_pc = PROGRAM_LOAD_ADDR;
        }
    }   
}

static void
opc_5XY0_noskip (void **state)
{
    /* Skips the next instruction if VX equals VY.
     * (Usually the next instruction is a jump to skip a code block)  */

    uint16_t i = 0x5000;
    uint16_t j = 0x0;
    memset(&s_v_regs, 0, sizeof(s_v_regs));

    /* Test each register behaves the same */
    for (; i <= 0x5FFF; i += 0x100) {
        for (j = 0; j < 0x00F0; j += 0x10) {
            /* Combine regs to make the opcode */
            i = (i & 0xFF00) | (j & 0xFF);

            DEBUG_PRINTF("Before - Op: 0x%x, s_pc: 0x%x", i, s_pc);
            /* Ensure that VI != VJ */
            if (((i & 0x0F00) >> 4) == (j & 0xFF)) {
                chip8_interpret_op(i);
                DEBUG_PRINTF("After - Op: 0x%x, s_pc: 0x%x", i, s_pc);
                /* X == Y so the contents of the register is guaranteed to
                 * be identical */
                assert_int_equal(s_pc, 0x202);
            } else {
                s_v_regs[(i & 0x0F00) >> 8] = 1;
                s_v_regs[j >> 4] = 2;
                chip8_interpret_op(i);
                DEBUG_PRINTF("After - Op: 0x%x, s_pc: 0x%x", i, s_pc);
                /* No match, so PC should not be incremented. */
                assert_int_equal(s_pc, 0x200);
            }
            /* Reset for next instruction */
            s_pc = PROGRAM_LOAD_ADDR;
        }
    }   
}

static void
opc_6XNN (void **state)
{
    /* Sets VX to NN. */

    int i = 0x6000;

    for (; i < 0x6FFF; i++) {
        chip8_interpret_op(i);
        assert_int_equal(s_v_regs[(i & 0x0F00) >> 8], i & 0xFF);
    }
}

static void
opc_7XNN (void **state)
{
    /* Adds NN to VX. (Carry flag is not changed)  */

    int i = 0x7000;

    for (; i < 0x7FFF; i++) {
        s_v_regs[(i & 0x0F00) >> 8] = 5;
        chip8_interpret_op(i);
        assert_int_equal(s_v_regs[(i & 0x0F00) >> 8], (uint8_t)((i & 0xFF) + 5));
        if ((i & 0x0F00) != 0x0F00) {
            assert_int_equal(s_v_regs[0xF], 0);
        }
    }
}

static void
opc_8XY0 (void **state)
{
    /* Sets VX to the value of VY. */

    int i = 0x8000;
    int j = 0;

    for (; i < 0x8FFF; i += 0x100) {
        for (j = 0; j < 0xF0; j += 0x10) {
            i = (i & 0xFF00) | (j & 0xFF);
            /* Set destination with a known value */
            chip8_interpret_op(0x6005 | (i & 0x0F00));
            /* Set source to test value */
            chip8_interpret_op(0x60a0 | ((j & 0xF0) << 4));
            /* Set VX to VY */
            chip8_interpret_op(i);
            assert_int_equal(s_v_regs[(i & 0x00F0) >> 4], s_v_regs[(i & 0x0F00) >> 8]);
        }
    }
}

static void
opc_8XY1 (void **state)
{
    /* Sets VX to (VX or VY). (Bitwise OR operation) */

    int i = 0x8001;
    int j = 0;

    for (; i < 0x8FFF; i += 0x100) {
        for (j = 0; j < 0xF1; j += 0x10) {
            i = (i & 0xFF0F) | (j & 0xFF);
            /* Set destination with a known value */
            chip8_interpret_op(0x6005 | (i & 0x0F00));
            DEBUG_PRINTF("VX: 0x%x", s_v_regs[(i & 0x0F00) >> 8]);
            /* Set source to test value */
            chip8_interpret_op(0x60a0 | ((j & 0xF0) << 4));
            DEBUG_PRINTF("VY: 0x%x", s_v_regs[(i & 0x00F0) >> 4]);
            DEBUG_PRINTF("VX2: 0x%x", s_v_regs[(i & 0x0F00) >> 8]);
            /* Set VX to VY */
            chip8_interpret_op(i);
            DEBUG_PRINTF("After - Op: 0x%x, VX: 0x%x", i, s_v_regs[(i & 0x0F00) >> 8]);
            if (((i & 0x0F00) >> 4) != (j & 0xF0)) {
                assert_int_equal(s_v_regs[(i & 0x0F00) >> 8], 0xa5);
            } else {
                assert_int_equal(s_v_regs[(i & 0x0F00) >> 8], 0xa0);
            }
        }
    }
}

static void
opc_8XY2 (void **state)
{
    /* Sets VX to (VX and VY). (Bitwise AND operation) */

    int i = 0x8002;
    int j = 0;

    for (; i < 0x8FFF; i += 0x100) {
        for (j = 0; j < 0xF2; j += 0x10) {
            i = (i & 0xFF0F) | (j & 0xFF);
            /* Set destination with a known value */
            chip8_interpret_op(0x6005 | (i & 0x0F00));
            DEBUG_PRINTF("VX: 0x%x", s_v_regs[(i & 0x0F00) >> 8]);
            /* Set source to test value */
            chip8_interpret_op(0x60a0 | ((j & 0xF0) << 4));
            DEBUG_PRINTF("VY: 0x%x", s_v_regs[(i & 0x00F0) >> 4]);
            DEBUG_PRINTF("VX2: 0x%x", s_v_regs[(i & 0x0F00) >> 8]);
            /* Set VX to VY */
            chip8_interpret_op(i);
            DEBUG_PRINTF("After - Op: 0x%x, VX: 0x%x", i, s_v_regs[(i & 0x0F00) >> 8]);
            if (((i & 0x0F00) >> 4) == (j & 0xF0)) {
                assert_int_equal(s_v_regs[(i & 0x0F00) >> 8], 0xa0);
            } else {
                assert_int_equal(s_v_regs[(i & 0x0F00) >> 8], 0x0);
            }
        }
    }
}

static void
opc_8XY3 (void **state)
{
    /* Sets VX to (VX xor VY). (Bitwise XOR operation) */

    int i = 0x8003;
    int j = 0;

    for (; i < 0x8FFF; i += 0x100) {
        for (j = 0; j < 0xF3; j += 0x10) {
            i = (i & 0xFF0F) | (j & 0xFF);
            /* Set destination with a known value */
            chip8_interpret_op(0x60b5 | (i & 0x0F00));
            DEBUG_PRINTF("VX: 0x%x", s_v_regs[(i & 0x0F00) >> 8]);
            /* Set source to test value */
            chip8_interpret_op(0x60a0 | ((j & 0xF0) << 4));
            DEBUG_PRINTF("VY: 0x%x", s_v_regs[(i & 0x00F0) >> 4]);
            DEBUG_PRINTF("VX2: 0x%x", s_v_regs[(i & 0x0F00) >> 8]);
            /* Set VX to VY */
            chip8_interpret_op(i);
            DEBUG_PRINTF("After - Op: 0x%x, VX: 0x%x", i, s_v_regs[(i & 0x0F00) >> 8]);
            if (((i & 0x0F00) >> 4) == (j & 0xF0)) {
                assert_int_equal(s_v_regs[(i & 0x0F00) >> 8], 0x0);
            } else {
                assert_int_equal(s_v_regs[(i & 0x0F00) >> 8], 0x15);
            }
        }
    }
}

static void
opc_8XY4_no_carry (void **state)
{
    /* Adds VY to VX. VF is set to 1 when there's a carry,
     * and to 0 when there isn't.   */

    int i = 0x8004;
    int j = 0;

    for (; i < 0x8FFF; i += 0x100) {
        for (j = 0; j < 0xF4; j += 0x10) {
            i = (i & 0xFF0F) | (j & 0xFF);
            /* Set destination with a known value */
            chip8_interpret_op(0x6005 | (i & 0x0F00));
            DEBUG_PRINTF("VX: 0x%x", s_v_regs[(i & 0x0F00) >> 8]);
            /* Set source to test value */
            chip8_interpret_op(0x6010 | ((j & 0xF0) << 4));
            DEBUG_PRINTF("VY: 0x%x", s_v_regs[(i & 0x00F0) >> 4]);
            DEBUG_PRINTF("VX2: 0x%x", s_v_regs[(i & 0x0F00) >> 8]);
            /* Set VX to VY */
            chip8_interpret_op(i);
            DEBUG_PRINTF("After - Op: 0x%x, VX: 0x%x", i, s_v_regs[(i & 0x0F00) >> 8]);

            /* It's entirely legal for someone to want to drop the result
             * into the flag register (VF), but that means it will be
             * overwritten by the carry flag result */
            if ((i & 0x0F00) >> 8 == 0xF) {
                assert_int_equal(s_v_regs[0xF], 0);
            } else if (((i & 0x0F00) >> 4) == (j & 0xF0)) {
                assert_int_equal(s_v_regs[(i & 0x0F00) >> 8], 0x20);
                assert_int_equal(s_v_regs[0xF], 0);
            } else {
                assert_int_equal(s_v_regs[(i & 0x0F00) >> 8], 0x15);
                assert_int_equal(s_v_regs[0xF], 0);
            }
        }
    }
}

static void
opc_8XY4_carry (void **state)
{
    /* Adds VY to VX. VF is set to 1 when there's a carry,
     * and to 0 when there isn't.   */

    int i = 0x8004;
    int j = 0;

    for (; i < 0x8FFF; i += 0x100) {
        for (j = 0; j < 0xF4; j += 0x10) {
            i = (i & 0xFF0F) | (j & 0xFF);
            /* Set destination with a known value */
            chip8_interpret_op(0x60FF | (i & 0x0F00));
            DEBUG_PRINTF("VX: 0x%x", s_v_regs[(i & 0x0F00) >> 8]);
            /* Set source to test value */
            chip8_interpret_op(0x60AF | ((j & 0xF0) << 4));
            DEBUG_PRINTF("VY: 0x%x", s_v_regs[(i & 0x00F0) >> 4]);
            DEBUG_PRINTF("VX2: 0x%x", s_v_regs[(i & 0x0F00) >> 8]);
            /* Set VX to VY */
            chip8_interpret_op(i);
            DEBUG_PRINTF("After - Op: 0x%x, VX: 0x%x", i, s_v_regs[(i & 0x0F00) >> 8]);

            /* It's entirely legal for someone to want to drop the result
             * into the flag register (VF), but that means it will be
             * overwritten by the carry flag result */
            if ((i & 0x0F00) >> 8 == 0xF) {
                assert_int_equal(s_v_regs[0xF], 1);
            } else if (((i & 0x0F00) >> 4) == (j & 0xF0)) {
                assert_int_equal(s_v_regs[(i & 0x0F00) >> 8], 0x5E);
                assert_int_equal(s_v_regs[0xF], 1);
            } else {
                assert_int_equal(s_v_regs[(i & 0x0F00) >> 8], 0xAE);
                assert_int_equal(s_v_regs[0xF], 1);
            }
        }
    }
}

static void
opc_8XY5 (void **state)
{
    /* Subtract VY from VX. VF is set to 1 when Vx > Vy, and
     * 0 otherwise. */

    int i = 0x8005;
    int j = 0;

    for (; i < 0x8FFF; i += 0x100) {
        for (j = 0; j < 0xF5; j += 0x10) {
            i = (i & 0xFF0F) | (j & 0xFF);
            /* Set destination with a known value */
            chip8_interpret_op(0x6010 | (i & 0x0F00));
            DEBUG_PRINTF("VX: 0x%x", s_v_regs[(i & 0x0F00) >> 8]);
            /* Set source to test value */
            chip8_interpret_op(0x6005 | ((j & 0xF0) << 4));
            DEBUG_PRINTF("VY: 0x%x", s_v_regs[(i & 0x00F0) >> 4]);
            DEBUG_PRINTF("VX2: 0x%x", s_v_regs[(i & 0x0F00) >> 8]);
            /* Set VX to VY */
            chip8_interpret_op(i);
            DEBUG_PRINTF("After - Op: 0x%x, VX: 0x%x", i, s_v_regs[(i & 0x0F00) >> 8]);

            /* Different from the Add operation, the borrow flag is set
             * first and then subtraction is performed. This means that
             * use of the flag register will impact the result. */
            if (((i & 0x0F00) >> 8) == 0xF &&
                       (j & 0xF0) >> 4 == 0xF) {
                assert_int_equal(s_v_regs[0xF], 0);
            } else if ((j & 0xF0) == 0xF0) {
                /* Vx - 0x1 = 0xF */
                assert_int_equal(s_v_regs[(i & 0x0F00) >> 8], 0xF);
            } else if ((i & 0x0F00) == 0xF00) {
                /* VF (0x1) - Vy = 0xFC */
                assert_int_equal(s_v_regs[0xF], 0xFC);
            } else if (((i & 0x0F00) >> 4) == (j & 0xF0)) {
                assert_int_equal(s_v_regs[(i & 0x0F00) >> 8], 0x00);
                assert_int_equal(s_v_regs[0xF], 0);
            } else {
                assert_int_equal(s_v_regs[(i & 0x0F00) >> 8], 0x0B);
                assert_int_equal(s_v_regs[0xF], 1);
            }
        }
    }
}

static void
opc_8XY6_no_low_bit (void **state)
{
    /* Stores the least significant bit of VX in VF and then
     * shifts VX to the right by 1. */

    int i = 0x8006;

    for (; i < 0x8FFF; i += 0x100) {
        /* Set destination with a known value */
        chip8_interpret_op(0x6010 | (i & 0x0F00));
        DEBUG_PRINTF("VX: 0x%x", s_v_regs[(i & 0x0F00) >> 8]);
        chip8_interpret_op(i);
        DEBUG_PRINTF("After - Op: 0x%x, VX: 0x%x", i, s_v_regs[(i & 0x0F00) >> 8]);

        if ((i & 0x0F00) >> 8 == 0xF) {
            assert_int_equal(s_v_regs[0xF], 0);
        } else {
            assert_int_equal(s_v_regs[0xF], 0);
            assert_int_equal(s_v_regs[(i & 0x0F00) >> 8], 0x08);
        }
    }
}

static void
opc_8XY6_low_bit (void **state)
{
    /* Stores the least significant bit of VX in VF and then
     * shifts VX to the right by 1. */

    int i = 0x8006;

    for (; i < 0x8FFF; i += 0x100) {
        /* Set destination with a known value */
        chip8_interpret_op(0x6011 | (i & 0x0F00));
        DEBUG_PRINTF("VX: 0x%x", s_v_regs[(i & 0x0F00) >> 8]);
        chip8_interpret_op(i);
        DEBUG_PRINTF("After - Op: 0x%x, VX: 0x%x", i, s_v_regs[(i & 0x0F00) >> 8]);

        if ((i & 0x0F00) >> 8 == 0xF) {
            assert_int_equal(s_v_regs[0xF], 0);
        } else {
            assert_int_equal(s_v_regs[0xF], 1);
            assert_int_equal(s_v_regs[(i & 0x0F00) >> 8], 0x08);
        }
    }
}

static void
opc_8XY7 (void **state)
{
    /* Subtract VX from VY and store to VX. VF is set to 1 when Vx > Vy, and
     * 0 otherwise. */

    int i = 0x8007;
    int j = 0;

    for (; i < 0x8FFF; i += 0x100) {
        for (j = 0; j < 0xF5; j += 0x10) {
            i = (i & 0xFF0F) | (j & 0xFF);
            /* Set destination with a known value */
            chip8_interpret_op(0x6010 | (i & 0x0F00));
            DEBUG_PRINTF("VX: 0x%x", s_v_regs[(i & 0x0F00) >> 8]);
            /* Set source to test value */
            chip8_interpret_op(0x6005 | ((j & 0xF0) << 4));
            DEBUG_PRINTF("VY: 0x%x", s_v_regs[(i & 0x00F0) >> 4]);
            DEBUG_PRINTF("VX2: 0x%x", s_v_regs[(i & 0x0F00) >> 8]);
            /* Set VX to VY */
            chip8_interpret_op(i);
            DEBUG_PRINTF("After - Op: 0x%x, VX: 0x%x", i, s_v_regs[(i & 0x0F00) >> 8]);

            /* Different from the Add operation, the borrow flag is set
             * first and then subtraction is performed. This means that
             * use of the flag register will impact the result. */
            if (((i & 0x0F00) >> 8) == 0xF &&
                       (j & 0xF0) >> 4 == 0xF) {
                assert_int_equal(s_v_regs[0xF], 0);
            } else if ((j & 0xF0) == 0xF0) {
                /* 0x1 - 0x10 = 0xF */
                assert_int_equal(s_v_regs[(i & 0x0F00) >> 8], 0xF0);
            } else if ((i & 0x0F00) == 0xF00) {
                /* VF (0x1) - Vx = 0xFC */
                assert_int_equal(s_v_regs[0xF], 0x5);
            } else if (((i & 0x0F00) >> 4) == (j & 0xF0)) {
                assert_int_equal(s_v_regs[(i & 0x0F00) >> 8], 0x00);
                assert_int_equal(s_v_regs[0xF], 0);
            } else {
                assert_int_equal(s_v_regs[(i & 0x0F00) >> 8], 0xF5);
                assert_int_equal(s_v_regs[0xF], 0);
            }
        }
    }
}

static void
opc_8XYE_no_high_bit (void **state)
{
    /* Stores the most significant bit of VX in VF and then
     * shifts VX to the left by 1. */

    int i = 0x800E;

    for (; i < 0x8FFF; i += 0x100) {
        /* Set destination with a known value */
        chip8_interpret_op(0x6010 | (i & 0x0F00));
        DEBUG_PRINTF("VX: 0x%x", s_v_regs[(i & 0x0F00) >> 8]);
        chip8_interpret_op(i);
        DEBUG_PRINTF("After - Op: 0x%x, VX: 0x%x", i, s_v_regs[(i & 0x0F00) >> 8]);

        if ((i & 0x0F00) >> 8 == 0xF) {
            assert_int_equal(s_v_regs[0xF], 0);
        } else {
            assert_int_equal(s_v_regs[0xF], 0);
            assert_int_equal(s_v_regs[(i & 0x0F00) >> 8], 0x20);
        }
    }
}

static void
opc_8XYE_high_bit (void **state)
{
    /* Stores the most significant bit of VX in VF and then
     * shifts VX to the left by 1. */

    int i = 0x800E;

    for (; i < 0x8FFF; i += 0x100) {
        /* Set destination with a known value */
        chip8_interpret_op(0x6082 | (i & 0x0F00));
        DEBUG_PRINTF("VX: 0x%x", s_v_regs[(i & 0x0F00) >> 8]);
        chip8_interpret_op(i);
        DEBUG_PRINTF("After - Op: 0x%x, VX: 0x%x", i, s_v_regs[(i & 0x0F00) >> 8]);

        if ((i & 0x0F00) >> 8 == 0xF) {
            assert_int_equal(s_v_regs[0xF], 0x2);
        } else {
            assert_int_equal(s_v_regs[0xF], 1);
            assert_int_equal(s_v_regs[(i & 0x0F00) >> 8], 0x4);
        }
    }
}

static void
opc_9XY0 (void **state)
{
    /* Skip next instruction if Vx != Vy. */
    int i = 0x9000;
    int j = 0;

    for (; i < 0x9FFF; i += 0x100) {
        for (j = 0; j <= 0xF0; j += 0x10) {
            i = (i & 0xFF0F) | (j & 0xFF);
            /* Set destination with a known value */
            chip8_interpret_op(0x6010 | (i & 0x0F00));
            DEBUG_PRINTF("VX: 0x%x", s_v_regs[(i & 0x0F00) >> 8]);
            /* Set source to test value */
            chip8_interpret_op(0x6005 | ((j & 0xF0) << 4));
            DEBUG_PRINTF("VY: 0x%x", s_v_regs[(i & 0x00F0) >> 4]);
            DEBUG_PRINTF("VX2: 0x%x", s_v_regs[(i & 0x0F00) >> 8]);
            chip8_interpret_op(i);
            DEBUG_PRINTF("After - Op: 0x%x, PC: 0x%x", i, s_pc);

            if ((i & 0x0F00) >> 4 == (j & 0xF0)) {
                assert_int_equal(s_pc, PROGRAM_LOAD_ADDR);
            } else {
                assert_int_equal(s_pc, PROGRAM_LOAD_ADDR + 2);
            }

            /* Reset program counter back to the start */
            chip8_interpret_op(0x1000 | PROGRAM_LOAD_ADDR);
        }
    }
}

static void
opc_ANNN (void **state)
{
    int i = 0xA000;

    for (; i <= 0xAFFF; i++) {
        chip8_interpret_op(i);
        assert_int_equal(s_i_reg, i & 0xFFF);
    }
}

static void
opc_BNNN_v0_nop (void **state)
{
    int i = 0xB000;

    for (; i <= 0xBFFF; i++) {
        /* v0 = 0x0 */
        chip8_interpret_op(0x6000);
        chip8_interpret_op(i);
        /* v0 contents are a NOP in this test */
        assert_int_equal(s_i_reg, i & 0xFFF);
    }
}

static void
opc_BNNN_v0_overflow (void **state)
{
    /* Specifically verify 16-bit add */
    chip8_interpret_op(0x60FF);
    chip8_interpret_op(0xBFFF);
    assert_int_equal(s_i_reg, 0x10FE);
}

static void
opc_CXNN (void **state)
{
    expect_function_call(get_random_byte);
    chip8_interpret_op(0xC0FF);
    assert_int_equal(s_v_regs[0], 4);

    expect_function_call(get_random_byte);
    chip8_interpret_op(0xC000);
    assert_int_equal(s_v_regs[0], 0);
}

static void
opc_DXYN_nop (void **state)
{
    LOAD_X(0, 0);
    LOAD_I(0x300);

    chip8_interpret_op(0xD001);
    /* Empty sprite at address 0, so no pixels cleared */
    assert_int_equal(s_v_regs[0xF], 0);
    assert_int_equal(s_vram[0][0], 0);
}

static void
opc_DXYN_pixel_cleared (void **state)
{
    LOAD_X(1, 0);
    LOAD_I(0x300);

    s_memory[0x300] = 0x8A;
    /* Write some bits to be cleared to VRAM */
    s_vram[0][0] = 0xFF;
    s_vram[0][4] = 0xFF;

    chip8_interpret_op(0xD111);
    assert_int_equal(s_v_regs[0xF], 1);
    assert_int_equal(s_vram[0][0], 0x0);
}

static void
opc_DXYN_multiple_bytes (void **state)
{
    int i;

    LOAD_X(2, 0);
    LOAD_I(0x300);

    memset(&s_memory[0x300], 0x8A, 0xF);

    chip8_interpret_op(0xD22F);

    for (i = 0; i < 0xF; i++) {
        DEBUG_PRINTF("VRAM[%x][%x]: 0x%x", 0, i, s_vram[0][i]);
    }

    /* Check some pixels */
    assert_int_equal(s_vram[0][0], 0xFF);
    assert_int_equal(s_vram[0][1], 0x00);
    assert_int_equal(s_vram[0][4], 0xFF);

    /* Nothing in VRAM at the start of the test, so no pixels cleared */
    assert_int_equal(s_v_regs[0xF], 0x0);
}

static void
opc_DXYN_wraparound (void **state)
{
    int i;

    LOAD_X(3, 0);
    LOAD_X(4, 30);
    LOAD_I(0x300);

    memset(&s_memory[0x300], 0x8A, 0xF);

    chip8_interpret_op(0xD34F);

    for (i = 0; i < 0xF; i++) {
        DEBUG_PRINTF("VRAM[%x][%x]: 0x%x", 0, i, s_vram[0][(i + 30) % 32]);
    }

    /* Spot check some pixels */
    assert_int_equal(s_vram[0][0], 0xFF);
    assert_int_equal(s_vram[0][1], 0x00);
    assert_int_equal(s_vram[30][0], 0xFF);
    assert_int_equal(s_vram[30][1], 0x00);

    /* Nothing in VRAM before, so no pixels will be cleared */
    assert_int_equal(s_v_regs[0xF], 0);
}

static void
opc_EX9E_pressed (void **state)
{
    int i;

    s_test_key_is_pressed = true;
    for (i = 0; i < NUM_V_REGISTERS; i++) {
        LOAD_X(i, 0);

        expect_function_call(get_key_pressed);
        chip8_interpret_op(0xE09E | ((i & 0xF) << 8));
        assert_int_equal(s_pc, PROGRAM_LOAD_ADDR + 2);
        /* Reset the program counter back to the start for the next test */
        chip8_interpret_op(0x1000 | PROGRAM_LOAD_ADDR);
    }
}

static void
opc_EX9E_not_pressed (void **state)
{
    int i;

    s_test_key_is_pressed = false;
    for (i = 0; i < NUM_V_REGISTERS; i++) {
        LOAD_X(i, 0);

        expect_function_call(get_key_pressed);
        chip8_interpret_op(0xE09E | ((i & 0xF) << 8));
        assert_int_equal(s_pc, PROGRAM_LOAD_ADDR);
    }
}

static void
opc_EXA1_pressed (void **state)
{
    int i;

    s_test_key_is_pressed = true;
    for (i = 0; i < NUM_V_REGISTERS; i++) {
        LOAD_X(i, 0);

        expect_function_call(get_key_pressed);
        chip8_interpret_op(0xE0A1 | ((i & 0xF) << 8));
        assert_int_equal(s_pc, PROGRAM_LOAD_ADDR);
    }
}

static void
opc_EXA1_not_pressed (void **state)
{
    int i;

    s_test_key_is_pressed = false;
    for (i = 0; i < NUM_V_REGISTERS; i++) {
        LOAD_X(i, 0);

        expect_function_call(get_key_pressed);
        chip8_interpret_op(0xE0A1 | ((i & 0xF) << 8));
        assert_int_equal(s_pc, PROGRAM_LOAD_ADDR + 2);
        /* Reset the program counter back to the start for the next test */
        chip8_interpret_op(0x1000 | PROGRAM_LOAD_ADDR);
    }
}

static void
opc_FX07 (void **state)
{
    int i;

    for (i = 0; i < NUM_V_REGISTERS; i++) {
        expect_function_call(get_delay_timer_remaining);
        chip8_interpret_op(0xF007 | ((i & 0xF) << 8));
        assert_int_equal(s_v_regs[i], 42);
    }
}

static void
opc_FX0A (void **state)
{
    int i;

    for (i = 0; i < NUM_V_REGISTERS; i++) {
        U16_MEMORY_WRITE(PROGRAM_LOAD_ADDR, htons(0xF00A | ((i & 0xF) << 8)));
        chip8_step();
        DEBUG_PRINTF("RAM[%x]: 0x%x", PROGRAM_LOAD_ADDR,
                     U16_MEMORY_READ(PROGRAM_LOAD_ADDR));
        DEBUG_PRINTF("PC: 0x%x", s_pc);
        assert_int_equal(s_pc, PROGRAM_LOAD_ADDR+2);

        /* The interpreter core will be waiting for a key press
         * after the LD Vx, K instruction. It will not advance
         * execution further until a key is received. */
        chip8_step();
        assert_int_equal(s_pc, PROGRAM_LOAD_ADDR+2);

        /* Press a key to continue execution */
        chip8_notify_key_pressed(CHIP8_KEY_F);
        assert_int_equal(s_v_regs[i], CHIP8_KEY_F);
        chip8_init();
    }
}

static void
opc_FX15 (void **state)
{
    int i;

    for (i = 0; i < NUM_V_REGISTERS; i++) {
        expect_function_call(set_delay_timer);
        LOAD_X(i, 45);
        chip8_interpret_op(0xF015 | ((i & 0xF) << 8));
    }
}

static void
opc_FX18 (void **state)
{
    int i;

    for (i = 0; i < NUM_V_REGISTERS; i++) {
        expect_function_call(set_sound_timer);
        LOAD_X(i, 50);
        chip8_interpret_op(0xF018 | ((i & 0xF) << 8));
    }
}

static void
opc_FX1E (void **state)
{
    int i;

    for (i = 0; i < NUM_V_REGISTERS; i++) {
        LOAD_I(0x100);
        LOAD_X(i, 0x55);
        chip8_interpret_op(0xF01E | ((i & 0xF) << 8));
        assert_int_equal(s_i_reg, 0x155);
    }
}

static void
opc_FX29 (void **state)
{
    uint32_t i;
    uint32_t x, y;

    for (i = 0; i <= 0xF; i++) {
        LOAD_X(0, i);

        /* Loads the sprite address for the character i into I */
        chip8_interpret_op(0xF029);

        /* Hardcoded verification of the sprite address */
        assert_int_equal(i * 5, s_i_reg);
        DEBUG_PRINTF("I: 0x%x\n", s_i_reg);

        chip8_interpret_op(0xD005);

        DEBUG_PRINTF("Character 0x%x\n", i);
        for (y = 0; y < DISPLAY_HEIGHT_PIXELS; y++) {
            for (x = 0; x < DISPLAY_WIDTH_PIXELS; x++) {
                DEBUG_PRINTF("0x%x ", s_vram[y][x]);
            }
            DEBUG_PRINTF("-\n", NULL);
        }
        DEBUG_PRINTF("--\n", NULL);
        memset(s_vram, 0, sizeof(s_vram));
    }
}

static void
opc_FX33 (void **state)
{
    int i;

    for (i = 0; i <= 255; i++) {
        LOAD_X(0, i);

        chip8_interpret_op(0xF033);

        DEBUG_PRINTF("i: %d", i);
        DEBUG_PRINTF("%u%u%u",
                     s_memory[s_i_reg],
                     s_memory[s_i_reg+1],
                     s_memory[s_i_reg+2]);

        assert_int_equal(s_memory[s_i_reg], i / 100);
        assert_int_equal(s_memory[s_i_reg + 1], (i % 100) / 10);
        assert_int_equal(s_memory[s_i_reg + 2], (i % 10));
    }
}

static void
opc_FX55 (void **state)
{
    int i;
    for (i = 0; i < NUM_V_REGISTERS; i++) {
        LOAD_X(i, i);
    }

    LOAD_I(0x300);

    /* Store all registers to memory */
    chip8_interpret_op(0xFF55);

    for (i = 0; i < NUM_V_REGISTERS; i++) {
        assert_int_equal(i, s_memory[0x300 + i]);
    }
}

static void
opc_FX65 (void **state)
{
    int i;
    for (i = 0; i < NUM_V_REGISTERS; i++) {
        s_memory[0x300 + i] = i;
    }

    LOAD_I(0x300);

    /* Load all registers from memory */
    chip8_interpret_op(0xFF65);

    for (i = 0; i < NUM_V_REGISTERS; i++) {
        assert_int_equal(i, s_v_regs[i]);
    }
}

static void
chip8_step_instruction (void **state)
{
    *(uint16_t *)&s_memory[PROGRAM_LOAD_ADDR] = htons(0x1EEE);
    chip8_step();
    assert_int_equal(s_pc, 0x0EEE);
}

static int
chip8_test_init (void **state)
{
    chip8_init();
    return 0;
}

static void
parse_args (int argc, char *argv[])
{
    char opt = '\0';

    while (opt != -1) {
        opt = getopt(argc, argv, "d");

        switch (opt) {
            default:
                break;
            case 'd':
                s_debug = true;
                break;
        }
    }
}

int
main(int argc, char *argv[])
{
    const struct CMUnitTest chip8_opc[] = {
        cmocka_unit_test_setup(opc_00E0, chip8_test_init),
        cmocka_unit_test_setup(opc_00EE, chip8_test_init),
        cmocka_unit_test_setup(opc_1NNN, chip8_test_init),
        cmocka_unit_test_setup(chip8_step_instruction, chip8_test_init),
        cmocka_unit_test_setup(opc_2NNN, chip8_test_init),
        cmocka_unit_test_setup(opc_3XNN_skip, chip8_test_init),
        cmocka_unit_test_setup(opc_3XNN_noskip, chip8_test_init),
        cmocka_unit_test_setup(opc_4XNN_skip, chip8_test_init),
        cmocka_unit_test_setup(opc_4XNN_noskip, chip8_test_init),
        cmocka_unit_test_setup(opc_5XY0_skip, chip8_test_init),
        cmocka_unit_test_setup(opc_5XY0_noskip, chip8_test_init),
        cmocka_unit_test_setup(opc_6XNN, chip8_test_init),
        cmocka_unit_test_setup(opc_7XNN, chip8_test_init),
        cmocka_unit_test_setup(opc_8XY0, chip8_test_init),
        cmocka_unit_test_setup(opc_8XY1, chip8_test_init),
        cmocka_unit_test_setup(opc_8XY2, chip8_test_init),
        cmocka_unit_test_setup(opc_8XY3, chip8_test_init),
        cmocka_unit_test_setup(opc_8XY4_no_carry, chip8_test_init),
        cmocka_unit_test_setup(opc_8XY4_carry, chip8_test_init),
        cmocka_unit_test_setup(opc_8XY5, chip8_test_init),
        cmocka_unit_test_setup(opc_8XY6_no_low_bit, chip8_test_init),
        cmocka_unit_test_setup(opc_8XY6_low_bit, chip8_test_init),
        cmocka_unit_test_setup(opc_8XY7, chip8_test_init),
        cmocka_unit_test_setup(opc_8XYE_no_high_bit, chip8_test_init),
        cmocka_unit_test_setup(opc_8XYE_high_bit, chip8_test_init),
        cmocka_unit_test_setup(opc_9XY0, chip8_test_init),
        cmocka_unit_test_setup(opc_ANNN, chip8_test_init),
        cmocka_unit_test_setup(opc_BNNN_v0_nop, chip8_test_init),
        cmocka_unit_test_setup(opc_BNNN_v0_overflow, chip8_test_init),
        cmocka_unit_test_setup(opc_CXNN, chip8_test_init),
        cmocka_unit_test_setup(opc_DXYN_nop, chip8_test_init),
        cmocka_unit_test_setup(opc_DXYN_pixel_cleared, chip8_test_init),
        cmocka_unit_test_setup(opc_DXYN_multiple_bytes, chip8_test_init),
        cmocka_unit_test_setup(opc_DXYN_wraparound, chip8_test_init),
        cmocka_unit_test_setup(opc_EX9E_pressed, chip8_test_init),
        cmocka_unit_test_setup(opc_EX9E_not_pressed, chip8_test_init),
        cmocka_unit_test_setup(opc_EXA1_pressed, chip8_test_init),
        cmocka_unit_test_setup(opc_EXA1_not_pressed, chip8_test_init),
        cmocka_unit_test_setup(opc_FX07, chip8_test_init),
        cmocka_unit_test_setup(opc_FX0A, chip8_test_init),
        cmocka_unit_test_setup(opc_FX15, chip8_test_init),
        cmocka_unit_test_setup(opc_FX18, chip8_test_init),
        cmocka_unit_test_setup(opc_FX1E, chip8_test_init),
        cmocka_unit_test_setup(opc_FX29, chip8_test_init),
        cmocka_unit_test_setup(opc_FX33, chip8_test_init),
        cmocka_unit_test_setup(opc_FX55, chip8_test_init),
        cmocka_unit_test_setup(opc_FX65, chip8_test_init),
    };

    parse_args(argc, argv);

    return cmocka_run_group_tests(chip8_opc, NULL, NULL);
}
