#ifndef CHIP8_H
#define CHIP8_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t memory[4096];       // 4 KB RAM
    uint8_t V[16];              // 16 Register (V0 bis VF)
    uint16_t I;                 // IndexRegister (für Speicheradressen)
    uint16_t pc;                // program Counter (Woim Code?)
    
    uint16_t stack[16];         // Stack Unterprogramme
    uint8_t sp;                 // Stack Pointer (Wo im Stack?)
    
    uint8_t delay_timer;        // Timer Spiel geschwindigkeit
    uint8_t sound_timer;        // timer Töne
    
    uint8_t keypad[16];         // 16 Tasten (gedrückt = 1, nicht = 0)
    uint32_t video[64 * 32];    // Bildschirm (2048 Pixel)
    
    uint16_t opcode;            // aktuell geladene bbefehl
} Chip8;

// main funct
void chip8_init(Chip8* chip8);
bool chip8_load_rom(Chip8* chip8, const char* filename);
void chip8_cycle(Chip8* chip8);

// 34 CHIP-8 
void clear_screen(Chip8* chip8);                          // 00E0
void return_from_subroutine(Chip8* chip8);                // 00EE
void jump_to_address(Chip8* chip8);                       // 1NNN
void call_subroutine(Chip8* chip8);                       // 2NNN
void skip_if_vx_equals_byte(Chip8* chip8);                // 3XNN
void skip_if_vx_not_equals_byte(Chip8* chip8);            // 4XNN
void skip_if_vx_equals_vy(Chip8* chip8);                  // 5XY0
void set_vx_to_byte(Chip8* chip8);                        // 6XNN
void add_byte_to_vx(Chip8* chip8);                        // 7XNN
void set_vx_to_vy(Chip8* chip8);                          // 8XY0
void or_vx_vy(Chip8* chip8);                              // 8XY1
void and_vx_vy(Chip8* chip8);                             // 8XY2
void xor_vx_vy(Chip8* chip8);                             // 8XY3
void add_vy_to_vx_with_carry(Chip8* chip8);               // 8XY4
void subtract_vy_from_vx(Chip8* chip8);                   // 8XY5
void shift_vx_right(Chip8* chip8);                        // 8XY6
void subtract_vx_from_vy(Chip8* chip8);                   // 8XY7
void shift_vx_left(Chip8* chip8);                         // 8XYE
void skip_if_vx_not_equals_vy(Chip8* chip8);              // 9XY0
void set_index_to_address(Chip8* chip8);                  // ANNN
void jump_to_address_plus_v0(Chip8* chip8);               // BNNN
void set_vx_to_random_and_byte(Chip8* chip8);             // CXNN
void draw_sprite(Chip8* chip8);                           // DXYN
void skip_if_key_pressed(Chip8* chip8);                   // EX9E
void skip_if_key_not_pressed(Chip8* chip8);               // EXA1
void set_vx_to_delay_timer(Chip8* chip8);                 // FX07
void wait_for_key_press(Chip8* chip8);                    // FX0A
void set_delay_timer_to_vx(Chip8* chip8);                 // FX15
void set_sound_timer_to_vx(Chip8* chip8);                 // FX18
void add_vx_to_index(Chip8* chip8);                       // FX1E
void set_index_to_sprite_char(Chip8* chip8);              // FX29
void store_bcd_of_vx(Chip8* chip8);                       // FX33
void store_registers_up_to_vx(Chip8* chip8);              // FX55
void load_registers_up_to_vx(Chip8* chip8);               // FX65

#endif