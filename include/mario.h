#pragma once

#include <Arduino.h>

constexpr int REST{-1};               // sentinel for silence

struct Note {
  int note;          // int so we can store REST = -1
  uint8_t octave;
  uint16_t duration; // milliseconds
};

// Super Mario Bros. theme - full conversion with pauses
const Note marioSong[] = {
  {NOTE_E, 5, 8},
  {NOTE_E, 5, 8},
  {REST, 0, 8},
  {NOTE_E, 5, 8},
  {REST, 0, 8},
  {NOTE_C, 5, 8},
  {NOTE_E, 5, 8},
  {NOTE_G, 5, 4},
  {REST, 0, 4},
  {NOTE_G, 4, 8},
  {REST, 0, 4},
  {NOTE_C, 5, 4},
  {NOTE_G, 4, 8},
  {REST, 0, 4},
  {NOTE_E, 4, 4},
  {NOTE_A, 4, 4},
  {NOTE_B, 4, 4},
  {NOTE_Bb, 4, 8},
  {NOTE_A, 4, 4},
  {NOTE_G, 4, 8},
  {NOTE_E, 5, 8},
  {NOTE_G, 5, 8},
  {NOTE_A, 5, 4},
  {NOTE_F, 5, 8},
  {NOTE_G, 5, 8},
  {NOTE_E, 5, 8},
  {NOTE_C, 5, 8},
  {NOTE_D, 5, 8},
  {NOTE_B, 4, 4},
  {NOTE_C, 5, 4},
  {NOTE_G, 4, 4},
  {NOTE_E, 4, 4},
  {NOTE_A, 4, 4},
  {NOTE_B, 4, 4},
  {NOTE_Bb, 4, 8},
  {NOTE_A, 4, 4},
  {NOTE_G, 4, 8},
  {NOTE_E, 5, 8},
  {NOTE_G, 5, 8},
  {NOTE_A, 5, 4},
  {NOTE_F, 5, 8},
  {NOTE_G, 5, 8},
  {NOTE_E, 5, 8},
  {NOTE_C, 5, 8},
  {NOTE_D, 5, 8},
  {NOTE_B, 4, 4},
  {REST, 0, 4},
  {NOTE_G, 5, 8},
  {NOTE_Fs, 5, 8},
  {NOTE_F, 5, 8},
  {NOTE_Eb, 5, 8},
  {NOTE_E, 5, 4},
  {REST, 0, 8},
  {NOTE_Gs, 4, 8},
  {NOTE_A, 4, 8},
  {NOTE_C, 4, 8},
  {REST, 0, 8},
  {NOTE_A, 4, 8},
  {NOTE_C, 5, 8},
  {NOTE_D, 5, 8},
  {REST, 0, 4},
};