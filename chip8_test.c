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

#include "chip8.c"

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

#define DEBUG_PRINTF(fmt, ...) (debug_printf("- "fmt"\n", __VA_ARGS__))

static void
opc_00E0 (void **state)
{
	/* Clears the display */
	static uint8_t s_zero[BITS2BYTES(DISPLAY_WIDTH * DISPLAY_HEIGHT)] = {0};

	memset(s_vram, 0xFE, sizeof(s_vram));
	chip8_interpret_op(0x00E0);
	assert_memory_equal(s_vram, s_zero, sizeof(s_vram));
}

static void
opc_00EE (void **state)
{
	/* Returns from a subroutine */
	s_stack_ptr = 0xE00;
	*(uint16_t *)&s_memory[s_stack_ptr] = 0xDEAD;
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

	uint16_t i = 0x3000;
	memset(&s_v_regs, 0, sizeof(s_v_regs));

	/* Test each register behaves the same */
	for (; i <= 0x3F00; i += 0x100) {
		DEBUG_PRINTF("Before - Op: 0x%x, s_pc: 0x%x", i, s_pc);
		chip8_interpret_op(i);
		DEBUG_PRINTF("After - Op: 0x%x, s_pc: 0x%x", i, s_pc);
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

	uint16_t i = 0x3000;
	memset(&s_v_regs, 0xDE, sizeof(s_v_regs));

	/* Test each register behaves the same */
	for (; i <= 0x3F00; i += 0x100) {
		DEBUG_PRINTF("Before - Op: 0x%x, s_pc: 0x%x", i, s_pc);
		chip8_interpret_op(i);
		DEBUG_PRINTF("After - Op: 0x%x, s_pc: 0x%x", i, s_pc);
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

	uint16_t i = 0x4000;
	memset(&s_v_regs, 0, sizeof(s_v_regs));

	/* Test each register behaves the same */
	for (; i <= 0x4F00; i += 0x100) {
		DEBUG_PRINTF("Before - Op: 0x%x, s_pc: 0x%x", i, s_pc);
		chip8_interpret_op(i);
		DEBUG_PRINTF("After - Op: 0x%x, s_pc: 0x%x", i, s_pc);
		/* Doesn't match, so PC should not be incremented again. */
		assert_int_equal(s_pc, 0x200);
		/* Reset for next instruction */
		s_pc = PROGRAM_LOAD_ADDR;
	}
}

static void
opc_4XNN_noskip (void **state)
{
	/* Skips the next instruction if VX doesn't equal NN.
	 * (Usually the next instruction is a jump to skip a code block)  */

	uint16_t i = 0x4011;
	memset(&s_v_regs, 0, sizeof(s_v_regs));

	/* Test each register behaves the same */
	for (; i <= 0x4F11; i += 0x100) {
		DEBUG_PRINTF("Before - Op: 0x%x, s_pc: 0x%x", i, s_pc);
		chip8_interpret_op(i);
		DEBUG_PRINTF("After - Op: 0x%x, s_pc: 0x%x", i, s_pc);
		/* Match, so PC should be incremented again. */
		assert_int_equal(s_pc, 0x202);
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
chip8_step_instruction (void **state)
{
	*(uint16_t *)&s_memory[PROGRAM_LOAD_ADDR] = 0x1EEE;
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
    };

    parse_args(argc, argv);

    return cmocka_run_group_tests(chip8_opc, NULL, NULL);
}
