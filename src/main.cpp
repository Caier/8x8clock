#include <Arduino.h>
#include <TimeLib.h>
#include <EnableInterrupt.h>
#include "task.hpp"
#include "progmem.hpp"
#include "clock.hpp"

const int row[8] = {
  A0, 11, A2, 10, 5, A3, 7, 2
};

const int col[8] = {
  9, 8, 3, A1, 4, 13, 12, 6
};

uint64_t volatile selfSeconds = 0;
uint16_t volatile hr1 = 0;
bool volatile shouldhr2 = false;
Clock clock;

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(50);
  pinMode(A5, INPUT_PULLUP);
  
  for (int i = 0; i < 8; i++) {
    pinMode(col[i], OUTPUT);
    pinMode(row[i], OUTPUT);
    digitalWrite(col[i], 1);
    digitalWrite(row[i], 0);
  }

  delay(2000);
  noInterrupts();
  TIMSK2 = 0;
  ASSR |= (1 << AS2);
  TCCR2A = 0;
  TCCR2B = 0;
  TCNT2 = 0;
  while(ASSR & 0x1F) {}
  TCCR2B |= (1 << CS20) | (1 << CS22);
  TIFR2 = 0x07;
  TIMSK2 |= (1 << TOIE2);
  interrupts();

  enableInterrupt(A5, [](){ 
    static uint32_t lit = 0;
    uint32_t it = millis();
    if(it - lit > 200)
      clock.press();
    lit = it;
  }, FALLING);
}

ISR(TIMER2_OVF_vect) { //the crystal is still shitty and going slow
  selfSeconds++;
  hr1++;
  if(hr1 == 3600) {
    selfSeconds += (shouldhr2 ? 5 : 4);
    shouldhr2 = !shouldhr2;
    hr1 = 0;
  }
}