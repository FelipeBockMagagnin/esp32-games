// Kong's own copy of ArduboyTones, timer 3 and all, gave way to the board's port: the same calls,
// with the jump's pitch slide played as one note
#pragma once
#include <ArduboyTones.h>

#define NOTE_JUMP NOTE_A5

class ArduboyTonesExt : public ArduboyTones
{
 public:
  ArduboyTonesExt() : ArduboyTones(nullptr) {}
  // The output switch is static in ArduboyTones and set by its constructor
  static void setOutputEnabled(bool (*outEn)()) { ArduboyTones{outEn}; }
};
