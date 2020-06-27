/*
 * chip8 - CHIP8 Interpreter
 *
 * Mike Mallin, 2020
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include "chip8.c"

//#define __DEBUG__
#ifdef __DEBUG__
	#define DEBUG_PRINTF(fmt, ...) (printf("- "fmt"\n", __VA_ARGS__))
#else /* __DEBUG__ */
	#define DEBUG_PRINTF(fmt, ...)
#endif /* __DEBUG__ */

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
		assert_int_equal(s_pc, i-0x1000);
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
		assert_int_equal(s_pc, i-0x2000);
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
			i = (i & 0xFF00) | j;
			/* Ensure that VI != VJ */
			if (((i & 0x0F00) >> 8) == j >> 8) {
				DEBUG_PRINTF("Before - Op: 0x%x, s_pc: 0x%x", i, s_pc);
				chip8_interpret_op(i);
				DEBUG_PRINTF("After - Op: 0x%x, s_pc: 0x%x", i, s_pc);
				/* X == Y so the contents of the register is guaranteed to
				 * be identical */
				assert_int_equal(s_pc, 0x202);
			} else {
				s_v_regs[(i & 0x0F00) >> 8] = 1;
				s_v_regs[j >> 8] = 2;
				DEBUG_PRINTF("Before - Op: 0x%x, s_pc: 0x%x", i, s_pc);
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

int
main(int argc, char *argv[]) {
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
    };

    return cmocka_run_group_tests(chip8_opc, NULL, NULL);
}
