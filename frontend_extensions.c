#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libretro.h"
#include "libretro_extensions.h"

#include "common.h"
#include "frontend_extensions.h"
#include "charmap.h"

bool ff_enabled = false;

#define ROM_START      0x0000
#define ZEROPAGE_START 0xFF80
#define WRAM_START     0xC000

#define WRAM_PAGE_SIZE 0x1000
#define ROM_PAGE_SIZE  0x4000

void (*input_override_cb)(void) = NULL;
void (*set_PC_breakpoint_)(unsigned short bank, unsigned short offset) = NULL;
void (*clear_PC_breakpoints)(void) = NULL;
unsigned short (*get_program_counter)(void) = NULL;

unsigned char *zeropage_ptr = NULL;
unsigned char *wram_ptr = NULL;
unsigned char *rom_ptr = NULL;

void set_memory_map_pointers(struct retro_memory_map *desc) {
  int i;
  for (i = 0; i < desc->num_descriptors; i++) {
    switch (desc->descriptors[i].start) {
      case ZEROPAGE_START:
        zeropage_ptr = desc->descriptors[i].ptr;
        break;
      case WRAM_START:
        wram_ptr = desc->descriptors[i].ptr;
        break;
      case ROM_START:
        rom_ptr = desc->descriptors[i].ptr;
        break;
    }
  }
}

unsigned char* game_addr_to_real_addr(
  unsigned short bank,
  unsigned short game_addr,
  unsigned char* base_ptr,
  unsigned short section_start,
  unsigned short section_length
) {
  unsigned char *result = base_ptr + (game_addr - section_start);
  if (bank > 0) {
    result += (bank - 1) * section_length;
  }
  return result;
}

void read_zeropage(unsigned short address, unsigned short num_bytes, unsigned char* dest) {
  unsigned char *base_address = zeropage_ptr + (address - ZEROPAGE_START);
  memcpy(dest, base_address, num_bytes);
}

void read_wram(unsigned char bank, unsigned short address, unsigned short num_bytes, unsigned char* dest) {
  unsigned char *base_address = game_addr_to_real_addr(bank, address, wram_ptr, WRAM_START, WRAM_PAGE_SIZE);
  memcpy(dest, base_address, num_bytes);
}

void write_wram_byte(unsigned char bank, unsigned short address, unsigned char byte) {
  unsigned char *base_address = game_addr_to_real_addr(bank, address, wram_ptr, WRAM_START, WRAM_PAGE_SIZE);
  *base_address = byte;
}

void read_rom(unsigned char bank, unsigned short address, unsigned short num_bytes, unsigned char* dest) {
  unsigned char *base_address = game_addr_to_real_addr(bank, address, rom_ptr, ROM_START, ROM_PAGE_SIZE);
  memcpy(dest, base_address, num_bytes);
}

#define ROM_BANK_ADDR 0xFF9D

unsigned char get_rom_bank() {
  unsigned char res;
  read_zeropage(ROM_BANK_ADDR, 1, &res);
  return res;
}

#define START_BATTLE_ADDR 0x74C1
#define START_BATTLE_BANK 0x0F

#define EXIT_BATTLE_ADDR 0x769E
#define EXIT_BATTLE_BANK 0x0F

#define BATTLE_MENU_ADDR 0x6139
#define BATTLE_MENU_BANK 0x0F

#define BATTLE_MENU_NEXT_ADDR 0x6175
#define BATTLE_MENU_NEXT_BANK 0x0F

#define LIST_MOVES_ADDR 0x4D6F
#define LIST_MOVES_BANK 0x14

#define MOVESELECTION_SCREEN_USE_MOVE_NOT_B_ADDR (0x65D9 + 2)
#define MOVESELECTION_SCREEN_USE_MOVE_NOT_B_BANK 0x0F

#define set_PC_breakpoint(NAME) set_PC_breakpoint_(NAME##_BANK, NAME##_ADDR)

void setup_breakpoints() {
  set_PC_breakpoint(START_BATTLE);
  set_PC_breakpoint(EXIT_BATTLE);
  set_PC_breakpoint(BATTLE_MENU);
  set_PC_breakpoint(BATTLE_MENU_NEXT);
  set_PC_breakpoint(LIST_MOVES);
  set_PC_breakpoint(MOVESELECTION_SCREEN_USE_MOVE_NOT_B);
}

#define check_breakpoint_state()\
  unsigned short bp_curr_pc = get_program_counter();\
  unsigned short bp_curr_bank = get_rom_bank();

#define did_hit_breakpoint(NAME) ((bp_curr_pc == NAME##_ADDR) && (bp_curr_bank == NAME##_BANK))
#define breakpoint_case(NAME) if (did_hit_breakpoint(NAME))


unsigned char breakpoint_menu_option = 0;

void input_override_set_menu_option() {
  unsigned char opt = breakpoint_menu_option;
  if (glfwGetKey(g_win, GLFW_KEY_1) == GLFW_PRESS && opt != 1) {
    breakpoint_menu_option = 1;
  }
  if (glfwGetKey(g_win, GLFW_KEY_2) == GLFW_PRESS && opt != 2) {
    breakpoint_menu_option = 2;
  }
  if (glfwGetKey(g_win, GLFW_KEY_3) == GLFW_PRESS && opt != 3) {
    breakpoint_menu_option = 3;
  }
  if (glfwGetKey(g_win, GLFW_KEY_4) == GLFW_PRESS && opt != 4) {
    breakpoint_menu_option = 4;
  }

  if (breakpoint_menu_option != opt) {
    printf("Set menu option: %u\n", breakpoint_menu_option);
    g_joy[RETRO_DEVICE_ID_JOYPAD_A] = 1;
  }
}

#define W_BATTLE_MENU_CURSOR_POSITION_ADDR 0xD0D2
#define W_BATTLE_MENU_CURSOR_POSITION_BANK 0x1
#define W_LIST_MOVES_MOVE_INDICES_BUFFER_ADDR 0xD25E
#define W_LIST_MOVES_MOVE_INDICES_BUFFER_BANK 0x01
#define W_MENU_CURSOR_Y_ADDR 0xCFA9
#define W_MENU_CURSOR_Y_BANK 0
#define W_CUR_MOVE_NUM_ADDR 0xD0D5
#define W_CUR_MOVE_NUM_BANK 0x1

#define MOVE_NAMES_BANK 0x72
#define MOVE_NAMES_ADDR 0x5f29
#define MOVE_NAME_MAX_LENGTH 13
#define NUM_MOVES 251

void get_move_names(char* dest, unsigned char* move_ixs);
void game_str_to_real_str(char* dest, unsigned char* src);

void handle_breakpoint() {
  check_breakpoint_state();

  breakpoint_case (START_BATTLE) {
    printf("Starting battle!\n");
  }

  breakpoint_case (EXIT_BATTLE) {
    printf("Exiting battle!\n");
  }

  breakpoint_case (BATTLE_MENU) {
    printf("Showing battle menu!\n");
    input_override_cb = &input_override_set_menu_option;
    printf("1 - FIGHT\n");
    printf("2 - POKéMON\n");
    printf("3 - PACK\n");
    printf("4 - RUN\n");
  }

  breakpoint_case (BATTLE_MENU_NEXT) {
    printf("Option selected in battle menu!\n");
    input_override_cb = NULL;

    if (breakpoint_menu_option != 0) {
      write_wram_byte(
          W_BATTLE_MENU_CURSOR_POSITION_BANK,
          W_BATTLE_MENU_CURSOR_POSITION_ADDR,
          breakpoint_menu_option
      );
    }

    breakpoint_menu_option = 0;
  }

  breakpoint_case (LIST_MOVES) {
    printf("Listing moves...\n");

    unsigned char move_indices[4];
    read_wram(
      W_LIST_MOVES_MOVE_INDICES_BUFFER_BANK,
      W_LIST_MOVES_MOVE_INDICES_BUFFER_ADDR,
      4, move_indices
    );

    char move_names[MOVE_NAME_MAX_LENGTH * 4];
    get_move_names(move_names, move_indices);

    for (int i = 0; i < 4; i++) {
      printf("%d: %s\n", i + 1, &move_names[i * MOVE_NAME_MAX_LENGTH]);
    }

    breakpoint_menu_option = 0;
    input_override_cb = &input_override_set_menu_option;
  }

  breakpoint_case (MOVESELECTION_SCREEN_USE_MOVE_NOT_B) {
    printf("Move selected!");
    input_override_cb = NULL;

    if (breakpoint_menu_option != 0) {
      write_wram_byte(
          W_MENU_CURSOR_Y_BANK,
          W_MENU_CURSOR_Y_ADDR,
          breakpoint_menu_option - 1
      );
      write_wram_byte(
          W_CUR_MOVE_NUM_BANK,
          W_CUR_MOVE_NUM_ADDR,
          breakpoint_menu_option - 1
      );
    }

    breakpoint_menu_option = 0;
  }
}

void game_str_to_real_str(char* dest, unsigned char* src) {
  memcpy(dest, src, MOVE_NAME_MAX_LENGTH);
  for (int i = 0; i < MOVE_NAME_MAX_LENGTH; i++) {
    if (dest[i] == 0x50) {
      dest[i] = '\0';
      return;
    }
    dest[i] = charmap[(unsigned char)dest[i]];
  }
}

void get_move_names(char* dest, unsigned char* move_ixs) {
  unsigned char* moves_ptr = game_addr_to_real_addr(MOVE_NAMES_BANK, MOVE_NAMES_ADDR, rom_ptr, ROM_START, ROM_PAGE_SIZE);

  for (int i = 0; i < 4; i++) {
    unsigned char* moves_ptr_curr = moves_ptr;
    if (move_ixs[i] == 0) {
      strcpy(dest + (i * MOVE_NAME_MAX_LENGTH), "-");
      continue;
    }
    int current_name_index = 0;
    while (current_name_index + 1 != move_ixs[i]) {
      while (*(moves_ptr_curr++) != 0x50) {}
      current_name_index++;
    }
    printf("Found move index %u at %ld\n", current_name_index, moves_ptr_curr - moves_ptr);
    game_str_to_real_str(dest + (i * MOVE_NAME_MAX_LENGTH), moves_ptr_curr);
  }
}
