#include "chip8.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

const uint8_t fontset[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};  // gesamt 80 Byte von 0x050 bis 0x09F im Speicher

void chip8_init(Chip8* chip8) {
    memset(chip8, 0, sizeof(Chip8));
    chip8->pc = 0x200;
    for (int i = 0; i < 80; ++i) {
        chip8->memory[0x050 + i] = fontset[i];
    }
}

bool chip8_load_rom(Chip8* chip8, const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) return false;

    // Dateigröße prüfen
    fseek(file, 0, SEEK_END);
    long rom_size = ftell(file);
    rewind(file);

    if (rom_size <= 0 || rom_size > (4096 - 0x200)) {
        printf("ROM-Datei zu groß oder leer: %ld Bytes (max %d)\n", rom_size, 4096 - 0x200);
        fclose(file);
        return false;
    }

    fread(&chip8->memory[0x200], 1, rom_size, file);
    fclose(file);
    return true;
}


void chip8_cycle(Chip8* chip8) {
    chip8->opcode = (chip8->memory[chip8->pc] << 8) | chip8->memory[chip8->pc + 1];
    chip8->pc += 2;

    switch (chip8->opcode & 0xF000) {
        case 0x0000:
            if (chip8->opcode == 0x00E0) clear_screen(chip8);
            else if (chip8->opcode == 0x00EE) return_from_subroutine(chip8);
            break;
        case 0x1000: jump_to_address(chip8); break;
        case 0x2000: call_subroutine(chip8); break;
        case 0x3000: skip_if_vx_equals_byte(chip8); break;
        case 0x4000: skip_if_vx_not_equals_byte(chip8); break;
        case 0x5000: skip_if_vx_equals_vy(chip8); break;
        case 0x6000: set_vx_to_byte(chip8); break;
        case 0x7000: add_byte_to_vx(chip8); break;
        case 0x8000:
            switch (chip8->opcode & 0x000F) {
                case 0x0: set_vx_to_vy(chip8); break;
                case 0x1: or_vx_vy(chip8); break;
                case 0x2: and_vx_vy(chip8); break;
                case 0x3: xor_vx_vy(chip8); break;
                case 0x4: add_vy_to_vx_with_carry(chip8); break;
                case 0x5: subtract_vy_from_vx(chip8); break;
                case 0x6: shift_vx_right(chip8); break;
                case 0x7: subtract_vx_from_vy(chip8); break;
                case 0xE: shift_vx_left(chip8); break;
            }
            break;
        case 0x9000: skip_if_vx_not_equals_vy(chip8); break;
        case 0xA000: set_index_to_address(chip8); break;
        case 0xB000: jump_to_address_plus_v0(chip8); break;
        case 0xC000: set_vx_to_random_and_byte(chip8); break;
        case 0xD000: draw_sprite(chip8); break;
        case 0xE000:
            if ((chip8->opcode & 0x00FF) == 0x009E) skip_if_key_pressed(chip8);
            else if ((chip8->opcode & 0x00FF) == 0x00A1) skip_if_key_not_pressed(chip8);
            break;
        case 0xF000:
            switch (chip8->opcode & 0x00FF) {
                case 0x07: set_vx_to_delay_timer(chip8); break;
                case 0x0A: wait_for_key_press(chip8); break;
                case 0x15: set_delay_timer_to_vx(chip8); break;
                case 0x18: set_sound_timer_to_vx(chip8); break;
                case 0x1E: add_vx_to_index(chip8); break;
                case 0x29: set_index_to_sprite_char(chip8); break;
                case 0x33: store_bcd_of_vx(chip8); break;
                case 0x55: store_registers_up_to_vx(chip8); break;
                case 0x65: load_registers_up_to_vx(chip8); break;
            }
            break;
        default: printf("Unbekannter Opcode: 0x%X\n", chip8->opcode); break;
    }
}




// 00E0: Löscht kompletten Bildschirm (setzt Pixel 0)
void clear_screen(Chip8* chip8) {
    memset(chip8->video, 0, sizeof(chip8->video));
}

// 00EE: Springt aus Unterprogramm zurück an Stelle, wo aufgerufen
void return_from_subroutine(Chip8* chip8) {
    chip8->sp--;
    chip8->pc = chip8->stack[chip8->sp];
}

// 1NNN: springt Speicheradresse NNN
void jump_to_address(Chip8* chip8) {
    uint16_t nnn = chip8->opcode & 0x0FFF;
    chip8->pc = nnn;
}

// 2NNN: Ruft Unterprogramm an Adresse NNN auf (merkt aktuelle Position im Stack)
void call_subroutine(Chip8* chip8) {
    uint16_t nnn = chip8->opcode & 0x0FFF;
    if (chip8->sp >= 16) {
        printf("Stack Overflow! sp=%d\n", chip8->sp);
        return;
    }
    chip8->stack[chip8->sp] = chip8->pc;
    chip8->sp++;
    chip8->pc = nnn;
}

// 3XNN: skip nächsten Befehl wenn Wert Register VX gleich NN ist
void skip_if_vx_equals_byte(Chip8* chip8) {
    uint8_t vx = (chip8->opcode & 0x0F00) >> 8;
    uint8_t nn = chip8->opcode & 0x00FF;
    if (chip8->V[vx] == nn) chip8->pc += 2;
}

// 4XNN: skip nächsten Befehl wenn Wert in Register VX ungleich NN ist
void skip_if_vx_not_equals_byte(Chip8* chip8) {
    uint8_t vx = (chip8->opcode & 0x0F00) >> 8;
    uint8_t nn = chip8->opcode & 0x00FF;
    if (chip8->V[vx] != nn) chip8->pc += 2;
}

// 5XY0: skip nächsten Befehl wenn Register VX gleich Register VY ist
void skip_if_vx_equals_vy(Chip8* chip8) {
    uint8_t vx = (chip8->opcode & 0x0F00) >> 8;
    uint8_t vy = (chip8->opcode & 0x00F0) >> 4;
    if (chip8->V[vx] == chip8->V[vy]) chip8->pc += 2;
}

// 6XNN: setzt Wert von Register VX auf Zahl NN
void set_vx_to_byte(Chip8* chip8) {
    uint8_t vx = (chip8->opcode & 0x0F00) >> 8;
    uint8_t nn = chip8->opcode & 0x00FF;
    chip8->V[vx] = nn;
}

// 7XNN: add Zahl NN zum Register VX (ohne das Carry-Flag verändern)
void add_byte_to_vx(Chip8* chip8) {
    uint8_t vx = (chip8->opcode & 0x0F00) >> 8;
    uint8_t nn = chip8->opcode & 0x00FF;
    chip8->V[vx] += nn;
}

// 8XY0: copy Wert Register VY in das Register VX
void set_vx_to_vy(Chip8* chip8) {
    uint8_t vx = (chip8->opcode & 0x0F00) >> 8;
    uint8_t vy = (chip8->opcode & 0x00F0) >> 4;
    chip8->V[vx] = chip8->V[vy];
}

// 8XY1: führt bitweises ODER (OR) zwischen VX und VY aus + speichert es in VX
void or_vx_vy(Chip8* chip8) {
    uint8_t vx = (chip8->opcode & 0x0F00) >> 8;
    uint8_t vy = (chip8->opcode & 0x00F0) >> 4;
    chip8->V[vx] |= chip8->V[vy];
    chip8->V[0xF] = 0;
}

// 8XY2: Führt bitweises UND (AND) zwischen VX und VY aus  speichert es in VX
void and_vx_vy(Chip8* chip8) {
    uint8_t vx = (chip8->opcode & 0x0F00) >> 8;
    uint8_t vy = (chip8->opcode & 0x00F0) >> 4;
    chip8->V[vx] &= chip8->V[vy];
    chip8->V[0xF] = 0;
}

// 8XY3: Führt bitweises Exklusiv-ODER (XOR) zwischen VX und VY aus speichert in VX
void xor_vx_vy(Chip8* chip8) {
    uint8_t vx = (chip8->opcode & 0x0F00) >> 8;
    uint8_t vy = (chip8->opcode & 0x00F0) >> 4;
    chip8->V[vx] ^= chip8->V[vy];
    chip8->V[0xF] = 0;
}

// 8XY4: add VY zu VX wenn Ergebnis größer als 255 - wird das Overflow-Flag (VF) 1 gesetzt
void add_vy_to_vx_with_carry(Chip8* chip8) {
    uint8_t vx = (chip8->opcode & 0x0F00) >> 8;
    uint8_t vy = (chip8->opcode & 0x00F0) >> 4;
    uint16_t sum = chip8->V[vx] + chip8->V[vy];
    uint8_t flag = (sum > 255) ? 1 : 0;
    chip8->V[vx] = sum & 0xFF;
    chip8->V[0xF] = flag;
}

// 8XY5: minus(sub) VY von VX if VX größer als VY war wird VF auf 1 gesetzt (kein Underflow)
void subtract_vy_from_vx(Chip8* chip8) {
    uint8_t vx = (chip8->opcode & 0x0F00) >> 8;
    uint8_t vy = (chip8->opcode & 0x00F0) >> 4;
    chip8->V[0xF] = (chip8->V[vx] >= chip8->V[vy]) ? 1 : 0;
    chip8->V[vx] -= chip8->V[vy];
}

// 8XY6: move Bits in VX um 1 rechts (halbiert Wert) das herausfallende bit gespeichert in VF
// Original CHIP-8: VX = VY >> 1
void shift_vx_right(Chip8* chip8) {
    uint8_t vx = (chip8->opcode & 0x0F00) >> 8;
    uint8_t vy = (chip8->opcode & 0x00F0) >> 4;
    uint8_t flag = chip8->V[vy] & 0x1;
    chip8->V[vx] = chip8->V[vy] >> 1;
    chip8->V[0xF] = flag;
}

// 8XY7: subtrahiert VX von VY speichert das Ergebnis in VX and VF = 1 if VY größer/gleich VX war
void subtract_vx_from_vy(Chip8* chip8) {
    uint8_t vx = (chip8->opcode & 0x0F00) >> 8;
    uint8_t vy = (chip8->opcode & 0x00F0) >> 4;
    chip8->V[0xF] = (chip8->V[vy] >= chip8->V[vx]) ? 1 : 0;
    chip8->V[vx] = chip8->V[vy] - chip8->V[vx];
}

// 8XYE: move Bits in VX um 1 nach links (verdoppelt den Wert) das höchste Bit landet in VF
// Original CHIP-8: VX = VY << 1
void shift_vx_left(Chip8* chip8) {
    uint8_t vx = (chip8->opcode & 0x0F00) >> 8;
    uint8_t vy = (chip8->opcode & 0x00F0) >> 4;
    uint8_t flag = (chip8->V[vy] & 0x80) >> 7;
    chip8->V[vx] = chip8->V[vy] << 1;
    chip8->V[0xF] = flag;
}

// 9XY0: Überspringt nächsten Befehl wenn Register VX ungleich Register VY ist
void skip_if_vx_not_equals_vy(Chip8* chip8) {
    uint8_t vx = (chip8->opcode & 0x0F00) >> 8;
    uint8_t vy = (chip8->opcode & 0x00F0) >> 4;
    if (chip8->V[vx] != chip8->V[vy]) chip8->pc += 2;
}

// ANNN: Setzt das Index-Register (I) auf die Speicheradresse NNN
void set_index_to_address(Chip8* chip8) {
    uint16_t nnn = chip8->opcode & 0x0FFF;
    chip8->I = nnn;
}

// BNNN: springt zu der Adresse die sich aus NNN plus dem Wert in Register V0 ergibt
void jump_to_address_plus_v0(Chip8* chip8) {
    uint16_t nnn = chip8->opcode & 0x0FFF;
    chip8->pc = nnn + chip8->V[0];
}

// CXNN: Generiert Zufallszahl (0-255) verknüpft sie per UND (AND) mit NN und speichert in VX
void set_vx_to_random_and_byte(Chip8* chip8) {
    uint8_t vx = (chip8->opcode & 0x0F00) >> 8;
    uint8_t nn = chip8->opcode & 0x00FF;
    chip8->V[vx] = (rand() % 256) & nn;
}

// DXYN: draw Sprite der Höhe N ab der Speicheradresse I an die Bildschirmkoordinaten (VX, VY)
// Setzt VF bei Kollision
void draw_sprite(Chip8* chip8) {
    uint8_t vx = (chip8->opcode & 0x0F00) >> 8;
    uint8_t vy = (chip8->opcode & 0x00F0) >> 4;
    uint8_t n  = chip8->opcode & 0x000F;

    uint8_t x_pos = chip8->V[vx] % 64;
    uint8_t y_pos = chip8->V[vy] % 32;
    uint8_t height = n;

    chip8->V[0xF] = 0; // KollisionsFlag zurücksetzen

    for (int row = 0; row < height; row++) {
        uint8_t sprite_byte = chip8->memory[chip8->I + row];

        for (int col = 0; col < 8; col++) {
            uint8_t sprite_pixel = sprite_byte & (0x80 >> col);
            if ((x_pos + col) >= 64 || (y_pos + row) >= 32) continue;
            uint32_t* screen_pixel = &chip8->video[(y_pos + row) * 64 + (x_pos + col)];

            if (sprite_pixel) {
                if (*screen_pixel == 0xFFFFFFFF) { 
                    chip8->V[0xF] = 1; // kollision
                }
                *screen_pixel ^= 0xFFFFFFFF; // XOR (Pixel umschalten)
            }
        }
    }
}

// EX9E: skip next Befehl wenn die Taste mit dem Wert aus VX gerade gedrückt wird
void skip_if_key_pressed(Chip8* chip8) {
    uint8_t vx = (chip8->opcode & 0x0F00) >> 8;
    uint8_t key = chip8->V[vx];
    if (chip8->keypad[key]) chip8->pc += 2;
}

// EXA1: skip next Befehl wenn die Taste mit Wert aus VX gerade NICHT gedrückt wird
void skip_if_key_not_pressed(Chip8* chip8) {
    uint8_t vx = (chip8->opcode & 0x0F00) >> 8;
    uint8_t key = chip8->V[vx];
    if (!chip8->keypad[key]) chip8->pc += 2;
}

// FX07: setzt den Wert Register VX auf aktuellen Stand des DelayTimers
void set_vx_to_delay_timer(Chip8* chip8) {
    uint8_t vx = (chip8->opcode & 0x0F00) >> 8;
    chip8->V[vx] = chip8->delay_timer;
}

// FX0A: pauseee Emulator bis eine Taste gedrückt wird und speichert diese Taste dann in VX
void wait_for_key_press(Chip8* chip8) {
    uint8_t vx = (chip8->opcode & 0x0F00) >> 8;
    bool key_pressed = false;
    for (int i = 0; i < 16; ++i) {
        if (chip8->keypad[i]) {
            chip8->V[vx] = i;
            key_pressed = true;
            break;
        }
    }
    if (!key_pressed) chip8->pc -= 2; 
}

// FX15: Setzt den Delay-Timer auf den Wert von Register VX
void set_delay_timer_to_vx(Chip8* chip8) {
    uint8_t vx = (chip8->opcode & 0x0F00) >> 8;
    chip8->delay_timer = chip8->V[vx];
}

// FX18: Setzt Sound-Timer auf Wert von Register VX
void set_sound_timer_to_vx(Chip8* chip8) {
    uint8_t vx = (chip8->opcode & 0x0F00) >> 8;
    chip8->sound_timer = chip8->V[vx];
}

// FX1E: addiert Wert von Register VX zum Index-Register I
void add_vx_to_index(Chip8* chip8) {
    uint8_t vx = (chip8->opcode & 0x0F00) >> 8;
    chip8->I += chip8->V[vx];
}

// FX29: setzt das Index-Register I die Speicheradresse des Font-Sprites für das Zeichen in VX
void set_index_to_sprite_char(Chip8* chip8) {
    uint8_t vx = (chip8->opcode & 0x0F00) >> 8;
    chip8->I = 0x50 + (chip8->V[vx] * 5); 
}

// FX33: Nimmt Zahl in VX (z.B.156) zerlegt in Hunderter (1), zehner (5), einer (6) 
// und speichert es RAM ab Adresse I. (BCD = Binary-Coded Decimal)
void store_bcd_of_vx(Chip8* chip8) {
    uint8_t vx = (chip8->opcode & 0x0F00) >> 8;
    uint8_t value = chip8->V[vx];
    chip8->memory[chip8->I + 2] = value % 10;
    value /= 10;
    chip8->memory[chip8->I + 1] = value % 10;
    value /= 10;
    chip8->memory[chip8->I] = value % 10;
}

// FX55: copy nacheinander all Register von V0 bis VX in den Arbeitsspeicher ab Adresse I
void store_registers_up_to_vx(Chip8* chip8) {
    uint8_t vx = (chip8->opcode & 0x0F00) >> 8;
    for (int i = 0; i <= vx; ++i) {
        chip8->memory[chip8->I + i] = chip8->V[i];
    }
}

// FX65: loadet nacheinander Werte aus Arbeitsspeicher (ab Adresse I) in die Register V0 bis VX
void load_registers_up_to_vx(Chip8* chip8) {
    uint8_t vx = (chip8->opcode & 0x0F00) >> 8;
    for (int i = 0; i <= vx; ++i) {
        chip8->V[i] = chip8->memory[chip8->I + i];
    }
}