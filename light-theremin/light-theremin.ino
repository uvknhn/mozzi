// Yes I'm aware I should have
// made the timer thing a function,
// maybe I'll clean it up later.
//
// BUILD AT YOUR OWN RISK, USE
// PROPER INPUT PROTECTION
// CIRCUITRY WHEN NECESSARY.
//
// LOESS-LABS.NET
//
// Dylan Barry, 2024, GNU GPLv3
#include <MozziGuts.h>
#include <Oscil.h>
#include <tables/triangle_valve_2048_int8.h>
#include <ResonantFilter.h> // subsonic filter
#include <RollingAverage.h>
#define CONTROL_RATE 256 // sets CV resolution

Oscil <TRIANGLE_VALVE_2048_NUM_CELLS, AUDIO_RATE> aOsc(TRIANGLE_VALVE_2048_DATA);
Oscil <TRIANGLE_VALVE_2048_NUM_CELLS, AUDIO_RATE> bOsc(TRIANGLE_VALVE_2048_DATA);
LowPassFilter aFilt;
LowPassFilter bFilt;
RollingAverage <int, 64> aAveragePitch;
RollingAverage <int, 64> bAveragePitch;
RollingAverage <int, 32> avgHarm;


// Pin Mappings.
const int aLEDPin = 2;
const int bLEDPin = 3;
const int aLDRPin = 6;
const int bLDRPin = 7;
const int aOscFreqPin = 1;
const int bOscFreqPin = 5;
const int filterFreqPin = 4;
const int fmPin = 0;
const int bendPin = 3;
const int harmonicSelectPin = 2;

// Create global variables.
int aGain, bGain;
int aLDRPrevious, bLDRPrevious;
unsigned long aMillisPrevious, bMillisPrevious;
int aCount, bCount;
int aHarm = 2;
int bHarm = 2;
int fm_intensity;
int aBoom, bBoom;
int aIndex, bIndex;
int bendFactor;

int table0[] = {2, 3};
int table1[] = {2, 4};
int table2[] = {2, 4};
int table3[] = {2, 5};

int table4[] = {2, 3};
int table5[] = {2, 3};
int table6[] = {2, 4};
int table7[] = {2, 4};
int table8[] = {2, 4};

void setup() {
  startMozzi(CONTROL_RATE);
  pinMode(aLEDPin, OUTPUT);
  pinMode(bLEDPin, OUTPUT);
}

void updateControl() {
  // aIndex/bIndex selects the value from the table
  // tableSelect selects the table
  int tableSelect = map(avgHarm.next(mozziAnalogRead(harmonicSelectPin)), 0, 1023, 0, 3);
  // Array of pointers to the tables in order to efficiently assign values
  int* tables[] = {table0, table1, table2, table3, table4, table5, table6, table7, table8};
  int aHarm = tables[tableSelect][aIndex];
  int bHarm = tables[tableSelect + 4][bIndex];

  // Map various variables.
  int aLDR = map(mozziAnalogRead(aLDRPin), 0, 512, 255, 0);
  int bLDR = map(mozziAnalogRead(bLDRPin), 0, 512, 255, 0);
  int aL = map(aLDR, 0, 255, 6, 0);
  int bL = map(bLDR, 0, 255, 6, 0);
  int filterFreq = map(mozziAnalogRead(filterFreqPin), 0, 1023, 0, 20);
  
  // Check for rapid change in LDR reading, flip flop accordingly.
  if (millis() - aMillisPrevious >= 300) {
    if (aLDRPrevious - aLDR>100){
      aCount = aCount + 1;
      if ((aCount % 2) == 0){aBoom = 1; aIndex = 0; digitalWrite(aLEDPin, LOW);}
      else{aBoom = 1; aIndex = 1; digitalWrite(aLEDPin, HIGH);}
    }
    else {aBoom = 0;}
    aLDRPrevious = aLDR;
    aMillisPrevious = millis();
  }
  if (millis() - bMillisPrevious >= 300) {
    if (bLDRPrevious - bLDR>100){
      bCount = bCount + 1;
      if ((bCount % 2) == 0){bBoom = 1; bIndex = 0; digitalWrite(bLEDPin, LOW);}
      else{bBoom = 1; bIndex = 1; digitalWrite(bLEDPin, HIGH);}
    }
    else {bBoom = 0;}
    bLDRPrevious = bLDR;
    bMillisPrevious = millis();
  }

  int bendRead = map(mozziAnalogRead(bendPin), 0, 1023, 0, 512);
  
  if (bendRead < 25){bendFactor = 0;}
  else {bendFactor = bendRead;}
  int aBend = map(aLDR, 0, 255, 0, bendFactor);
  int bBend = map(bLDR, 0, 255, 0, bendFactor);
  // Set Oscillator Frequencies.
  aOsc.setFreq((aAveragePitch.next((mozziAnalogRead(aOscFreqPin) >> 1)+aBend))*aHarm);
  bOsc.setFreq((bAveragePitch.next((mozziAnalogRead(bOscFreqPin) >> 1)+bBend))*bHarm);

  // Set Subsonic Filter Frequency and Resonance.
  aFilt.setCutoffFreqAndResonance((filterFreq + aL), 480);
  bFilt.setCutoffFreqAndResonance((filterFreq + bL + 1), 480);

  // Set FM Intensity.
  fm_intensity = map(mozziAnalogRead(fmPin), 0, 1023, 0, 63);

  // Input signal into Subsonic Filters.
  if (aBoom == 1) {aGain = (int) (aFilt.next(1023));}
  else {aGain = (int) (aFilt.next(aLDR));}
  if (bBoom == 1) {bGain = (int) (bFilt.next(1023));}
  else {bGain = (int) (bFilt.next(bLDR));}

}

AudioOutput_t updateAudio() {
  // Gate and limiters, to avoid clipping.
  if (aGain < 1){aGain = 0;}
  if (aGain > 126){aGain = 126;}
  if (bGain < 1){bGain = 0;}
  if (bGain > 126){bGain = 126;}

  // FM Stuff.
  int aVoice = (aGain * (aOsc.phMod(fm_intensity * ((bGain * bOsc.next())>>5))));
  int bVoice = (bGain * (bOsc.phMod(fm_intensity * (aVoice>>4))));
  
  return MonoOutput::from16Bit(aVoice + bVoice);
}
void loop() {
  audioHook();
}
