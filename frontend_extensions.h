#ifndef FRONTEND_H
#define FRONTEND_H

#include "libretro.h"
#include "libretro_extensions.h"

extern void (*set_PC_breakpoint_)(unsigned short bank, unsigned short offset);
extern void (*clear_PC_breakpoints)(void);
extern unsigned short (*get_program_counter)(void);

extern void (*input_override_cb)(void);

void set_memory_map_pointers(struct retro_memory_map *desc);
void setup_breakpoints();
void handle_breakpoint();

#endif
