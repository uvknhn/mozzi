// PINOUTS:
// A0 - aDog frequency
// A1 - aDog filter reso
// A2 - aDog filter freq
// A3 - aDog gate input
// A4 - bDog frequency
// A5 - bDog filter reso
// A6 - bDog filter freq
// A7 - bDog gate input
// D9 - aDog audio out
// D10- bDog audio out
//
// These last 4 outputs are not working
// as intended in this version.
// Will fix soon - otherwise functional.
//
// D2 - aDog beat
// D3 - aDog offBeat
// D4 - bDog beat
// D5 - bDog offBeat
//
// BUILD AT YOUR OWN RISK, USE
// PROPER INPUT PROTECTION
// CIRCUITRY WHEN NECESSARY.
//
// Dylan Barry, 2024, GNU GPLv3
#include <MozziGuts.h>
#include <Oscil.h> // oscillator template
#include <tables/triangle_valve_2048_int8.h> // triangle table
#include <ResonantFilter.h> // subsonic filter

#define CONTROL_RATE 512 // sets CV resolution

Oscil <TRIANGLE_VALVE_2048_NUM_CELLS, AUDIO_RATE> aDog(TRIANGLE_VALVE_2048_DATA);
Oscil <TRIANGLE_VALVE_2048_NUM_CELLS, AUDIO_RATE> bDog(TRIANGLE_VALVE_2048_DATA);
LowPassFilter aFilt;
LowPassFilter bFilt;

int aGain, bGain;

void setup() {
  startMozzi(CONTROL_RATE);
}

void updateControl() {

  digitalWrite(2, LOW);digitalWrite(3, HIGH);
  digitalWrite(4, LOW);digitalWrite(5, HIGH);
  if (aGain > 250){digitalWrite(2, HIGH);digitalWrite(3, LOW);}
  if (bGain > 250){digitalWrite(4, HIGH);digitalWrite(5, LOW);}
  int aButton = mozziAnalogRead(3);
  int bButton = mozziAnalogRead(7);
  // convert digital button input to analog audio
  int aValue = map(aButton, 0, 1023, 0, 255);
  int bValue = map(bButton, 0, 1023, 0, 255);

  // tune dogs with knobs
  aDog.setFreq(mozziAnalogRead(0) << 1); bDog.setFreq(mozziAnalogRead(4) << 1);
  
  // scale filter tuning knobs to be subsonic range only
  int aFreq = map(mozziAnalogRead(1), 0, 255, 1, 10);
  int bFreq = map(mozziAnalogRead(5), 0, 255, 1, 10);  
  
  // control filters
  aFilt.setCutoffFreqAndResonance(aFreq, (mozziAnalogRead(2) + 255));
  bFilt.setCutoffFreqAndResonance(bFreq, (mozziAnalogRead(6) + 255));

  // vca stuff
  int aa = (aFilt.next(aValue));
  int bb = (bFilt.next(bValue));
  aGain = (int) aa;
  bGain = (int) bb;
}

AudioOutput_t updateAudio() {
  // crude gate and limiters, it sounds awful without this
  if (aGain > 255){aGain = 255;}
  if (aGain < 1){aGain = 0;}
  if (bGain > 255){bGain = 255;}
  if (bGain < 1){bGain = 0;}
  
  return StereoOutput::from16Bit(aGain * aDog.next(), bGain * bDog.next());
}
void loop() {
  audioHook(); // required here
}
