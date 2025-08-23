#pragma once

#include "pitches.h"

int melody[] = {
    NOTE_D3, REST, NOTE_D3, REST, NOTE_D3, REST, NOTE_D3, NOTE_D3, NOTE_D3, NOTE_A2, REST,
    NOTE_D3, REST, NOTE_D3, REST, NOTE_D3, REST, NOTE_D3, NOTE_D3, NOTE_D3, NOTE_A2, REST,
    NOTE_D3, REST, NOTE_D3, REST, NOTE_D3, REST, NOTE_D3, NOTE_D3, NOTE_D3, NOTE_A2, REST,
    NOTE_D3,
    NOTE_D4, REST, NOTE_D4, NOTE_D4, REST,
    NOTE_E3, NOTE_D3, NOTE_F3, REST, NOTE_F3,
    NOTE_D4, REST, NOTE_D4, NOTE_D4, REST,
    NOTE_D3,
    NOTE_D4, REST, NOTE_D4, NOTE_D4, REST,
    NOTE_E3, NOTE_D3, NOTE_F3, REST, NOTE_F3,
    NOTE_D4,
    REST
  };
  
  int durations[] = {
    8, 16, 8, 16, 8, 16, 10, 10, 10, 3, 2,
    8, 16, 8, 16, 8, 16, 10, 10, 10, 3, 2,
    8, 16, 8, 16, 8, 16, 10, 10, 10, 3, 2,
    4,
    4, 3, 4, 4, 3,
    6, 6, 6, 33, 6,
    4, 3, 4, 4, 3,
    4,
    4, 3, 4, 4, 3,
    6, 6, 6, 33, 6,
    3,
    1
  };