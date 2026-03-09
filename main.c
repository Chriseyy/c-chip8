#include <stdio.h>
#include <math.h>
#include "raylib.h"
#include "chip8.h"

#define SAMPLE_RATE     44100
#define BEEP_FREQUENCY  440    
#define BEEP_VOLUME     0.3f    

static float phase = 0.0f;
static bool sound_playing = false;

void audio_callback(void* buffer, unsigned int frames) {
    float* samples = (float*)buffer;
    for (unsigned int i = 0; i < frames; i++) {
        if (sound_playing) {
            samples[i] = sinf(2.0f * PI * BEEP_FREQUENCY * phase) * BEEP_VOLUME;
            phase += 1.0f / SAMPLE_RATE;
            if (phase >= 1.0f) phase -= 1.0f;
        } else {
            samples[i] = 0.0f;
            phase = 0.0f; 
        }
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Benutzung: %s <ROM-Datei>\n", argv[0]);
        return -1;
    }

    Chip8 chip8;
    chip8_init(&chip8);
    
    if (!chip8_load_rom(&chip8, argv[1])) {
        printf("Konnte ROM %s nicht laden!\n", argv[1]);
        return -1;
    }

    int scale = 15; 
    InitWindow(64 * scale, 32 * scale, "CHIP-8 Emulator (Raylib)");
    SetTargetFPS(60);

    InitAudioDevice();
    SetAudioStreamBufferSizeDefault(512);

    AudioStream beep_stream = LoadAudioStream(SAMPLE_RATE, 32, 1); 
    SetAudioStreamCallback(beep_stream, audio_callback);
    PlayAudioStream(beep_stream);

    while (!WindowShouldClose()) {

        // TASTENEINGABEN ABFANGEN 
        // Original: 1 2 3 C  ->  PC: 1 2 3 4
        //           4 5 6 D  ->      Q W E R
        //           7 8 9 E  ->      A S D F
        //           A 0 B F  ->      Z X C V
        chip8.keypad[0x1] = IsKeyDown(KEY_ONE);
        chip8.keypad[0x2] = IsKeyDown(KEY_TWO);
        chip8.keypad[0x3] = IsKeyDown(KEY_THREE);
        chip8.keypad[0xC] = IsKeyDown(KEY_FOUR);
        chip8.keypad[0x4] = IsKeyDown(KEY_Q);
        chip8.keypad[0x5] = IsKeyDown(KEY_W);
        chip8.keypad[0x6] = IsKeyDown(KEY_E);
        chip8.keypad[0xD] = IsKeyDown(KEY_R);
        chip8.keypad[0x7] = IsKeyDown(KEY_A);
        chip8.keypad[0x8] = IsKeyDown(KEY_S);
        chip8.keypad[0x9] = IsKeyDown(KEY_D);
        chip8.keypad[0xE] = IsKeyDown(KEY_F);
        chip8.keypad[0xA] = IsKeyDown(KEY_Z);
        chip8.keypad[0x0] = IsKeyDown(KEY_X);
        chip8.keypad[0xB] = IsKeyDown(KEY_C);
        chip8.keypad[0xF] = IsKeyDown(KEY_V);

        for (int i = 0; i < 10; i++) {
            chip8_cycle(&chip8);
        }

        // Sound an/aus je nach sound_timer
        sound_playing = (chip8.sound_timer > 0);

        BeginDrawing();
        ClearBackground(BLACK);

        // gehen jeden einzelnen Pixel 64x32 Array durch
        for (int y = 0; y < 32; y++) {
            for (int x = 0; x < 64; x++) {
                if (chip8.video[y * 64 + x] != 0) {
                    DrawRectangle(x * scale, y * scale, scale, scale, WHITE);
                }
            }
        }

        EndDrawing();
    }

    StopAudioStream(beep_stream);
    UnloadAudioStream(beep_stream);
    CloseAudioDevice();
    CloseWindow();

    return 0;
}