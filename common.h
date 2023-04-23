#ifndef COMMON_H
#define COMMON_H

#include "libretro.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define load_sym(V, S) do {\
	if (!((*(void**)&V) = dlsym(g_retro.handle, #S))) \
		die("Failed to load symbol '" #S "'': %s", dlerror()); \
	} while (0)
#define load_retro_sym(S) load_sym(g_retro.S, S)

extern unsigned g_joy[RETRO_DEVICE_ID_JOYPAD_R3+1];
extern GLFWwindow *g_win;

#endif
