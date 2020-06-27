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

static void
opc_00E0 (void **state)
{
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
	chip8_interpret_op(0x0EE);
	assert_int_equal(s_pc, 0xDEAD);
	assert_int_equal(s_stack_ptr, 0xE02);
}

static void
opc_1NNN (void **state)
{
	uint16_t i = 0x1000;

	for (; i <= 0x1FFF; i++) {
		chip8_interpret_op(i);
		assert_int_equal(s_pc, i-0x1000);
	}
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
    };

    return cmocka_run_group_tests(chip8_opc, NULL, NULL);
}
