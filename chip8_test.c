#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include "chip8.c"

/* A test case that does nothing and succeeds. */
static void opc_00E0(void **state) {
	static uint8_t s_zero[BITS2BYTES(DISPLAY_WIDTH*DISPLAY_HEIGHT)] = {0};

	memset(s_vram, 0xFE, sizeof(s_vram));
	chip8_interpret_op(0x00E0);
	assert_memory_equal(s_vram, s_zero, sizeof(s_vram));
}

int
main(int argc, char *argv[]) {
    const struct CMUnitTest chip8_opc_0[] = {
        cmocka_unit_test(opc_00E0),
    };
    return cmocka_run_group_tests(chip8_opc_0, NULL, NULL);
}
