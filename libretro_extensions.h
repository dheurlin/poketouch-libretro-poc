#ifndef LIBRETRO_EXT_H__
#define LIBRETRO_EXT_H__

#define BREAKPOINT_HIT -2

#ifdef __cplusplus
extern "C" {
#endif

void ext_set_PC_breakpoint(unsigned short bank, unsigned short offset);
void ext_clear_PC_breakpoints();
unsigned short ext_get_program_counter();

#ifdef __cplusplus
}
#endif

#endif
