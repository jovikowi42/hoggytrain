// Joviko WI, (C) 2022-2023
//
// Give a Hogwarts Express model train some sounds and lights: station announcements, steam train chuff-chuff sounds,
// plus a snippet of familiar melodies.
//
// The basic idea is that most of the time, the train lights are slightly flickering (and the coal box is glowering), but
// every now and then, either a melody is played, the steam engine chuff-chuffs, the whistle plays, or a station announcement is made.
//
// Also, this is explicitly a challenge to see how much can be done with a standard Arduino Nano without any extra hardware (besides
// the piezo speakers, WS2812s, and resistor and capacitor to protect / filter the WS2812s). Much more could be done with additional
// storage (such as playback of digitized sound clips from, say, ScotRail station announcements or HP movie clips). A beefier processor
// such as a Raspberry Pi Zero could easily handle making sounds AND flickering the lights simultaneously. But what's the fun in making
// it easy like that?
//
// The station announcements are via the Talkie speech synthesis library, which is hard-coded to Digital Pin 3. A dedicated
// piezo speaker is connected to this pin and to Digital Pin 11 (inverted signal, used to boost volume). Talkie is hard coded
// to pins 3 and 11 because it uses Timer 2 for its PWM fiddling, and Timer 2 is only supported on those two pins.
//
// https://reference.arduino.cc/reference/en/libraries/talkie/
//
// All other sounds (melodies, chuff-chuff sounds, train whistle) are via the Mozzi library, which is hard-coded to
// Digital Pin 9. A second dedicated piezo speaker is connected to this pin and to GND.
//
// https://sensorium.github.io/Mozzi/
//
// In addition, it uses 3 tiny addressable WS2812 RGB LEDs to provide the engine's front light, glowing coal fire, and the
// passenger cabin's overhead light. The LEDs are in series and are connected to 5V, GND, and Digital Pin 6 for signal. The WS2812s
// are controlled via the Adafruit NeoPixel library.
//
// https://learn.adafruit.com/adafruit-neopixel-uberguide/arduino-library-use
//
// As noted above, the train lights and coal box flicker most of the time, but hold at their current values while sounds are made. This
// is done to avoid figuring out how to have both Talkie / Mozzi making sounds at the same time the NeoPixel library is messing with the
// WS2812s. Given how much processor bandwith these all use, it's not clear that simultaneous flickering and sound generation is
// even possible.
//
// Intended for an Arduino Nano plus two small piezo speakers and 3 WS2812 LEDs. Powered by a small USB 2200 mAh power bank.
// To be installed in a medium-sized (2-3 foot) Hogwarts Express train model, such as:
//   https://www.4dmodelkit.com/products/harry-potter-hogwarts-express
//   https://www.amazon.com/dp/B08LCK7H27
//
// Note that this sketch uses 26+K of program storage space, which is around 87% available of 30K max, plus 1.3+K (67+%) of dynamic
// memory from 2K max. It was hoped to include playback of some short audio samples (such as "Allll Aboooard!"), but they just won't fit.
//
// NOTE: To make Talkie louder, I hacked the Talkie.cpp file in in the Talkie library.
//  https://github.com/ArminJo/Talkie/blob/master/src/Talkie.cpp
//  On my Mac: ~/Documents/Arduino/libraries/Talkie/src/Talkie.cpp
//  Overriding line ~856 from:
//     nextPwm = (u0 >> 2) + 0x80;
//  With these lines added below the #endif on line ~857:
//     int tPwm = ((int) u0 * 6) / 8;
//     nextPwm = tPwm + 0x80;
//  This does cause slight distortion of some words (notably sp5_DEPARTURE), but increases the volume of Talkie overall
//  greatly. For this use, the tradeoff is worth it. Adjust the 6/8 ratio as needed. 5/8 was a little too quiet but
//  distortion-free. 7/8 was slightly louder but much more distorted.
//
// NOTES/TODOS: 
// - Train whistle sounds sucky. Needs more tweaking or a rewrite into a single call with different volume envelopes for each "chime".
//   Make the whistle call just one long call with ramp-up/-down on the various whistle parts. Basically, start with WHISTLE_A quickly 
//   ramping up in volume by itself, then all 4 whistles ramp up together, then ramp down together, then finally just WHISTLE_A ramps
//   slowly down to silence.
//
// - Music uses simple sinewaves and is OK but a bit harsh on the ear. Mozzi can for sure do better, but my synth-fu skills are weak.
//   Tunes are familiar but hacked-down snippets from Hedwig's Theme and the Harry Potter Main Theme. They're not perfect, but
//   you can recognize them.
//
// - It would be fun to add a "dementor mode" where the all 3 lights turn sickly, slowly changing green, and the "dementors music"
//   from the HP movies plays, then someone calls out "Expecto Patronum!", all the lights flash white, then things return to normal. 
//   BUT ends up there isn't really any dementor-specific music (besides some bass viol bowing) during the dementor scenes, and, yeah,
//   "expecto..." isn't exactly in the Talkie vocabulary. Also, there wouldn't be enough storage to wedge all that into the code.
//
// + Chuff-chuff had a very distinct whine ever since I turned on the NeoPixel code. Which is weird given that the NeoPixel library
//   does not use timers or any of the pins that Mozzi uses. The whine seems to change frequency and is sometimes a warble. It
//   doesn't seem to have anything to do with AC line leakage, since running from a battery doesn't make it go away. Turning the
//   brightness of the WS2812 LEDs all down to 0 helps, but doesn't make it go away. It doesn't appear until the NeoPixel library
//   .begin() or maybe .show() routine is called.
//   SOLUTION: Adding the recommended NeoPixel 470 uF capacitor from 5V to GND and inline 470 Ohm resistor on the WS2812 signal line
//   made the whine almost totally go away. It's still there, but faint, and can only be heard during chuff-chuffs even though it is
//   present during the tunes and whistle sounds. The WS2812s still seem happy with the added components.
//   Note: Adding the recommended Mozzi cap and resistor didn't help the whine or Mozzi output quality, but did make the Mozzi output
//   volume much quieter. So I removed them.
//
// + Station announcement is currently "Express 9572 departure from 9 and 3 4...All abort". 
//   Yeah, "abort" sounds a lot like "aboard", right? Sure it does.

//
// PINS:
//  NOT USED: VIN: 4.5V battery +, ended up using USB power bank instead of batteries.
//  GND: Next to VIN. To breadboard for WS2812 string -
//  +5V: To breadboard for WS2812 string +
//  GND: Mozzi speaker -   GND on 6-pin header, pin closest to TX1.
//  D3:  Talkie speaker +
//  D6:  To breadboard for WS2812 string signal
//  D9:  Mozzi speaker +
//  D11: Talkie speaker inverted / -

// Electrical filters:
//
// Connect GND and +5V from Arduino to breadboard.
//
// W2812 Neopixels:
// - From https://learn.adafruit.com/adafruit-neopixel-uberguide/basic-connections
// - D6 to breadboard point 3.
// - 470 Ohm resistor breadboard point 2 and breadboard point 3.
// - Breadboard point 2 to first WS2812's signal line.
// - Resistor protects WS2812s, prevents spikes on data line
// - 470 uF (100 to 1000 uF) cap between GND and +5V on breadboard, prevents onrush current damage
// - Breadboard GND to first WS2812's GND line
// - Breadboard +5V to first WS2812's +5V line
//
// Mozzi:
// - NOT USED. Left here for reference.
// - From https://sensorium.github.io/Mozzi/learn/output/
// - D9 to breadboard point 7.
// - 270 Ohm resistor between breadboard point 7 and breadboard point 6.
// - Breadboard point 6 connects to Mozzi speaker's + wire
// - 100 nF cap between breadboard point 6 and GND
// - Breadboard GND to Mozzi speaker GND
//
// Talkie:
// - From https://github.com/ArminJo/Talkie#hints
// - No electrical components needed, since a piezo speaker is used?
// - There is a bit of distortion and whine but those seem appropriate for a train station PA system


// Initial code came from the Mozzi EAD example.
// Talkie speech, Mozzi music, and Mozzi whistle sounds were added,
// along with code for controlling the WS2812 LEDs via Adafruit NeoPixel library.

/*  Example playing an enveloped noise source
		using Mozzi sonification library.

		Demonstrates Ead (exponential attack decay).

		Circuit: Audio output on digital pin 9 on a Uno or similar, or
		DAC/A14 on Teensy 3.1, or
		check the README or http://sensorium.github.io/Mozzi/

		Mozzi help/discussion/announcements:
		https://groups.google.com/forum/#!forum/mozzi-users

		Tim Barrass 2012, CC by-nc-sa
 */

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>


#include <MozziGuts.h>
#include <Oscil.h> // oscillator template
#include <Sample.h> // sample template
#include <tables/brownnoise8192_int8.h> // recorded audio wavetable
#include <tables/sin2048_int8.h> // sine table for oscillator
#include <tables/square_no_alias_2048_int8.h> // sine table for oscillator
#include <Ead.h> // exponential attack decay
#include <EventDelay.h>
#include <mozzi_rand.h>

#include "Talkie.h"
#include "Vocab_US_Large.h"

// The WS2812 LEDs need 5V, GND, and a signal pin. 3 is used by Talkie, 9 is used
// by Mozzi, so why not good old Digital Pin 6 for controlling the lights.
#define LED_PIN    6
#define LED_COUNT  3

#define CONTROL_RATE 256 // Hz, powers of 2 are most reliable

#define MOZZI_PIN       9

#define MOZZI_CHUFF     1 // Mozzi mode to make a chuff sound
#define MOZZI_NOTE      2 // Mozzi mode to make a note
#define MOZZI_WHISTLE   3 // Mozzi mode to play a steam whistle sound

#define CHUFF_DELAY 1100 // Milliseconds between first and second chuffs
#define NUM_CHUFFS  15   // Number of chuffs to chuff in a sequence

#define ANNOUNCEMENT_DELAY  100   // Milliseconds pause between announcement words

#define FLICKERS_PER_SOUND  300   // Number of LED flickers between sounds. Flickers = 10 - 100ms


// 
// Frequencies of notes used for both melodies and for train whistle.

#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_CS6 1109
#define NOTE_D6  1175
#define NOTE_DS6 1245
#define NOTE_E6  1319
#define NOTE_F6  1397
#define NOTE_C7  2093
#define NOTE_D7  2349
#define NOTE_E7  2637
#define NOTE_F7  2794

/*
The train whistle sound isn't great. But it fits in storage and isn't terrible.

NOTE: This is the "movie whistle sound", NOT the actual Crosby 4" 3-chime from the Orton Hall train.
They do sound a bit different from each other.

NOTE: There is a better implementation of a train whistle in the Mozzi forum. But sadly it doesn't
quite fit in the Nano's program storage space along with all of the other stuff going on.

  https://groups.google.com/g/mozzi-users/c/l8l8KbMoXXk/m/0FHmhFfgAYsJ

To create the whistle:

Took the clip "Harry Potter and the Chamber of Secrets (2/5) Movie CLIP - Reckless Flying (2002) HD"

  https://www.youtube.com/watch?v=cPsIU9BTbcQ

Extracted MP3 from it.

Loaded into Audacity.

Clipped down to only train whistle parts.

Used Graphic EQ effect to reduce sound under 200 Hz and over 6000 Hz.

Displayed in Frequency Analysis.

3 peaks show up:
  ~580 Hz (D5)
  ~775 Hz (G5)
  ~925 Hz (B5)
  Also wide mound centered on ~325 Hz (E4)

Used square waves plus a bit of brown noise (which was conveniently already loaded for the
chuff-chuff sound) to get a hoarse whistle-y sound. Play the lowest note solo to simulate
the engineer just starting to pull on the whistle cord, then play all 4 notes, then drop
back to one note to finish things.

 */

#define WHISTLE_INITIAL           0
#define WHISTLE_TRITONE           1
#define WHISTLE_FADE              2
#define WHISTLE_DUD               3

#define WHISTLE_INITIAL_DURATION  900
#define WHISTLE_TRITONE_DURATION  900
#define WHISTLE_FADE_DURATION     0
#define WHISTLE_DUD_DURATION      0

#define WHISTLE_A NOTE_B5
#define WHISTLE_B NOTE_G5
#define WHISTLE_C NOTE_G5
#define WHISTLE_D NOTE_D5


// Voice structure that Talkie uses
Talkie voice;

// NeoPixel structure for controlling the WS2812 LED strip
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// Mozzi stuff.
// Brown noise oscillator is for the steam train chuff-chuff sound
Oscil<BROWNNOISE8192_NUM_CELLS, AUDIO_RATE> aNoise(BROWNNOISE8192_DATA);
// Sinewave oscillators are for playing the melody and train whistle
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> aSin(SIN2048_DATA);
Oscil <SQUARE_NO_ALIAS_2048_NUM_CELLS, AUDIO_RATE> aSquare(SQUARE_NO_ALIAS_2048_DATA);
Oscil <SQUARE_NO_ALIAS_2048_NUM_CELLS, AUDIO_RATE> bSquare(SQUARE_NO_ALIAS_2048_DATA);
Oscil <SQUARE_NO_ALIAS_2048_NUM_CELLS, AUDIO_RATE> cSquare(SQUARE_NO_ALIAS_2048_DATA);
Oscil <SQUARE_NO_ALIAS_2048_NUM_CELLS, AUDIO_RATE> dSquare(SQUARE_NO_ALIAS_2048_DATA);
// Event delay is for counting to the end of the current note or chuff-chuff sound and stopping Mozzi when it is done.
// This returns the timers to normal and allows the NeoPixel and Talkie libraries to use them.
EventDelay kDelay; // for triggering envelope start
// EAD envelope gives the chuff-chuff sound a nice hard start and fade over time so it sounds more realistic
Ead kEnvelope(CONTROL_RATE); // resolution will be CONTROL_RATE

int gain;
int inMozzi = 0;
int mozziMode = MOZZI_CHUFF;
int inMelody = 0, curMelodyNote = 0, whichMelody = 0;
int inChuffing = 0, curChuff = 0, chuffDelay = CHUFF_DELAY;
int inWhistle = 0, curWhistle = WHISTLE_INITIAL;
int whichSound = 0;


/* 
  Hedwig's theme - Harry Potter 
  More songs available at https://github.com/robsoncouto/arduino-songs                                            
                                              
                                              Robson Couto, 2019
*/

int melody[] = {NOTE_B4,
  NOTE_E5, NOTE_G5, NOTE_FS5,
  NOTE_E5, NOTE_B5,
  NOTE_A5, 
  NOTE_FS5,
  NOTE_E5, NOTE_G5, NOTE_FS5,
  NOTE_DS5, NOTE_F5,
  NOTE_B4,
  
  NOTE_B4,
  NOTE_E5, NOTE_G5, NOTE_FS5, //10
  NOTE_E5, NOTE_B5,
  NOTE_D6,
  NOTE_CS6, NOTE_C6,
  NOTE_GS5, NOTE_C6, NOTE_B5,
  NOTE_AS5, NOTE_AS4, NOTE_G5,
  NOTE_E5};

int melodyDurations[] = {4,
  -4, 8, 4,
  2, 4,
  -2, 
  -2,
  -4, 8, 4,
  2, 4,
  -1, 
  4,

  -4, 8, 4, //10
  2, 4,
  2, 4,
  2, 4,
  -4, 8, 4,
  2, 4,
  -1};

#define MELODY_LENGTH 30


// Extracted second melody, snippet of "Harry Potter Main Theme", from 
//   https://freemidi.org/download3-22659-chamber-of-secrets-harrys-wonderous-world-harry-potter
// Skipped the upper-register part since it sounds weird and squeaky-er, and just repeat the first part
// Did a lot of tweaking to get this to sound right, and it's still not quite there, but close enough for now.
// Some corrected note values deduced using https://onlinesequencer.net/309085
// Added values for second melody based on https://mixbutton.com/mixing-articles/music-note-to-frequency-chart/

int melody2[] = {
  NOTE_E5,
  NOTE_E6,
  NOTE_C6,
  NOTE_E6,
  NOTE_C6,
  NOTE_F6,
  NOTE_E6,
  NOTE_DS6,
  NOTE_DS6,
  NOTE_E6,
  NOTE_C6,
  NOTE_A5,
  NOTE_DS5,
  NOTE_C6,
  NOTE_A5, // long note

  NOTE_A4,
  NOTE_B4,
  NOTE_C5,
  NOTE_G5, NOTE_G5, NOTE_G5,
  NOTE_F5,
  NOTE_C6,
  NOTE_E6,
  NOTE_C6,
  NOTE_F5, NOTE_F5,
  NOTE_D5,

  NOTE_E5,
  NOTE_E6,
  NOTE_C6,
  NOTE_E6,
  NOTE_C6,
  NOTE_F6,
  NOTE_E6,
  NOTE_DS6,
  NOTE_DS6,
  NOTE_E6,
  NOTE_C6,
  NOTE_A5,
  NOTE_DS5,
  NOTE_C6,
  NOTE_A5, // long note
};

int melody2Durations [] = {
  4,
  2,
  4,
  2,
  4,
  2,
  4,
  2,
  4,
  -4,
  8,
  -8,
  2,
  4,
  1, // long note

  -4,
  -4,
  4,
  4, 4, 4,
  4,
  4,
  4,
  4,
  4, 4,
  1,

  4,
  2,
  4,
  2,
  4,
  2,
  4,
  2,
  4,
  -4,
  8,
  -8,
  2,
  4,
  -1, // long note
};

#define MELODY2_LENGTH  43



void cutTheLights() {
  strip.setPixelColor(0, 0, 0, 0);
  strip.setPixelColor(1, 0, 0, 0);
  strip.setPixelColor(2, 0, 0, 0);
  strip.show();  
}


void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  // Set up the WS2812 LEDs
  strip.begin();
  strip.setBrightness(255);
  cutTheLights();
  // strip.setPixelColor(0, 255,240,240); // Test to see if LEDs are working
  // strip.setPixelColor(1, 255,0,0);     // Coalbox in engine
  // strip.setPixelColor(2, 255,200,200);   // Passenger car overhead light
  // strip.show();
  // delay(2000);
  

  // use float to set freq because it will be small and fractional
  aNoise.setFreq((float)AUDIO_RATE/BROWNNOISE8192_SAMPLERATE);
  aSin.setFreq(440);
  aSquare.setFreq(WHISTLE_A);
  bSquare.setFreq(WHISTLE_B);
  cSquare.setFreq(WHISTLE_C);
  dSquare.setFreq(WHISTLE_D);
  randSeed(); // fresh random, MUST be called before startMozzi - wierd bug
  // startMozzi(CONTROL_RATE);
  // Wait 1 second before starting chuffs
  inMozzi = 0;
  // kDelay.start(1000);
  delay(1000);
  startMelody();  // Kick things off by playing one of the melodies
  whichSound++;
}


void playNote(int freq, int duration) {
  startMozzi(CONTROL_RATE);
  aSin.setFreq(freq);
  digitalWrite(LED_BUILTIN, HIGH);
  inMozzi = 1;
  mozziMode = MOZZI_NOTE;
  kDelay.start(duration);  
}


void playNextNoteInMelody() {
  int duration, note, melodyLength;
  if (whichMelody == 0) {
    duration = melodyDurations[curMelodyNote];
    note = melody[curMelodyNote];
    melodyLength = MELODY_LENGTH;
  } else {
    duration = melody2Durations[curMelodyNote];
    note = melody2[curMelodyNote];
    melodyLength = MELODY2_LENGTH;
  }
  if (duration == 0) {
    duration = 1;
  }
  if (duration > 0) {
    duration = 1024 / duration;
  } else {
    duration = -duration;
    duration = (1024 * 3) / (duration * 2);
  }
  playNote(note, duration);
  curMelodyNote++;
  if (curMelodyNote >= melodyLength) {
    curMelodyNote = 0;
    inMelody = 0;
  }
}


void startMelody() {
  inMelody = 1;
  curMelodyNote = 0;
  // whichMelody = random(3);
  whichMelody = whichSound;
  if (whichMelody > 0) {
    whichMelody = 1;
  }
  // whichMelody = 0;
  playNextNoteInMelody();
}


void whistle(int whichWhistle) {
  startMozzi(CONTROL_RATE);
  digitalWrite(LED_BUILTIN, HIGH);
  inMozzi = 1;
  mozziMode = MOZZI_WHISTLE;
  if (whichWhistle == WHISTLE_TRITONE) {
    // Play all the whistle "chimes"
    aSquare.setFreq(WHISTLE_A);
    bSquare.setFreq(WHISTLE_B);
    cSquare.setFreq(WHISTLE_C);
    dSquare.setFreq(WHISTLE_D);
    kDelay.start(WHISTLE_TRITONE_DURATION);  
  } else if (whichWhistle == WHISTLE_INITIAL) {
    // Initial single whistle
    aSquare.setFreq(WHISTLE_A);
    kDelay.start(WHISTLE_INITIAL_DURATION);  
  } else {
    // Fade
    aSquare.setFreq(WHISTLE_A);
    kDelay.start(WHISTLE_FADE_DURATION);  
  }
}


void nextWhistle() {

  whistle(curWhistle);
  curWhistle++;
  if (curWhistle > WHISTLE_FADE) {
    curWhistle = 0;
    inWhistle = 0;
  }
}


void startWhistle() {
  inWhistle = 1;
  curWhistle = 0;
  nextWhistle();
}



void chuff() {
  // cutTheLights();
  randSeed();
  startMozzi(CONTROL_RATE);
  // set random parameters
  unsigned int duration = rand(500u)+200;
  unsigned int attack = rand(75)+5; // +5 so the internal step size is more likely to be >0
  unsigned int decay = duration - attack;
  duration = 1000u;
  attack = 75u;
  decay = duration - attack;
  digitalWrite(LED_BUILTIN, HIGH);
  inMozzi = 1;
  mozziMode = MOZZI_CHUFF;
  kEnvelope.start(attack,decay);
  kDelay.start(duration + chuffDelay);  
}


void chuffNextChuff() {
  chuff();
  // delay(chuffDelay);
  chuffDelay -= 150;
  if (chuffDelay < 50) {
    chuffDelay = 50;
  }
  curChuff++;
  if (curChuff >= NUM_CHUFFS) {
    curChuff = 0;
    inChuffing = 0;
  }  
}


void startChuffing() {
  inChuffing = 1;
  curChuff = 0;
  chuffDelay = CHUFF_DELAY;
  chuffNextChuff();  
}




// Mozzi callback to update values based on current situation
void updateControl(){
  // Use the Mozzi delay call to determine when a tone or chuff noise is done playing.
  if(kDelay.ready()){
    gain = 0;
    inMozzi = 0;
    digitalWrite(LED_BUILTIN, LOW);
    return;
  }

  if ((mozziMode == MOZZI_CHUFF) || (mozziMode == MOZZI_WHISTLE)) {
    // jump around in audio noise table to disrupt obvious looping
    aNoise.setPhase(rand((unsigned int)BROWNNOISE8192_NUM_CELLS));
 
    gain = (int) kEnvelope.next();
    // gain = 1;
  } else if ((mozziMode == MOZZI_NOTE) || (mozziMode == MOZZI_WHISTLE)) {
    // Nothing to do for simple sine or square wave.
  }
}


AudioOutput_t updateAudio(){
  if (inMozzi == 0) {
    return 0;
  }
  if (mozziMode == MOZZI_CHUFF) {
    return MonoOutput::from16Bit(gain * aNoise.next());
  }
  if (mozziMode == MOZZI_NOTE) {
    return MonoOutput::from16Bit(aSin.next() * 257);
  }
  if (mozziMode == MOZZI_WHISTLE) {
    if (curWhistle != WHISTLE_TRITONE) {
      // Gains are hand-selected and still aren't quite right...
      return MonoOutput::from16Bit(
        ((int) aSquare.next() * 100) + ((int) bSquare.next() + (int) cSquare.next() + (int) dSquare.next() * 64) + (aNoise.next() * 85));
    } else {
        return MonoOutput::from16Bit((aSquare.next() * 100) + (aNoise.next() * 85));
    }
  }
}


void sayAWord(const uint8_t *theWord) {
  // Using the queued version of voice.sayQ() makes the announcement
  // a little smoother than speaking word by word with voice.say().
  voice.sayQ(theWord);
}


void stationAnnouncement() {
  // Sadly, the Talkie library doesn't include "train", "track",
  // "express", "quarters", or of course "hog" or "warts". So have
  // it say something close-ish. It does have "all", but not "aboard".
  digitalWrite(LED_BUILTIN, HIGH);
  sayAWord(sp2_X);
  sayAWord(sp2_PRESS);
  sayAWord(sp2_NINE);
  sayAWord(sp2_FIVE);
  sayAWord(sp2_SEVEN);
  sayAWord(sp2_TWO);
  sayAWord(sp5_DEPARTURE);
  sayAWord(sp2_FROM);
  sayAWord(sp2_NINE);
  sayAWord(sp2_AND);
  sayAWord(sp2_THREE);
  sayAWord(sp2_FOUR);
  voice.wait();
  delay(700);
  sayAWord(sp2_ALL);
  sayAWord(sp2_ABORT); // Ya know, "aboard" and "abort" sound almost the same, right? Right?
  voice.wait();
  digitalWrite(LED_BUILTIN, LOW);
  // delay(500);
  // digitalWrite(LED_BUILTIN, HIGH);
  // sayAWord(AllAboard);
  // voice.wait();
  // digitalWrite(LED_BUILTIN, LOW);
  delay(500);
  // Put all the hardware back the way we found it.
  voice.terminate();
}




void trainLights() {
  int whichLight = random(3);
  int r, g, b;

  digitalWrite(LED_BUILTIN, LOW);
  delay(10 + random(90));
  digitalWrite(LED_BUILTIN, HIGH);

  if (whichLight == 2) {
    // Bright yellow-white engine front light.
    r = 200 + random(10);
    g = r - 120 + random(5);
    b = g;
  } else if (whichLight == 0) {
    // Yellow-white "candle light" in passenger compartment ceiling.
    // r = 60 + random(65);
    g = 95 + random(5);
    r = 195 + random(10);
    b = 0; // g - 40;
  } else {
    if (random(10) == 1) {
      // Make an occasional bright flash of a primary RGB or secondary VYO
      // color, because this is a special train and it runs on coal plus magic!
      r = 255 * random(2);
      g = 255 * random(2);
      b = 255 * random(2);     
    } else {
      // Reddish cast-iron stove "logs" in second floor stove.
      r = 150 + random(50);
      g = random(20);
      b = 0;
    }
  }

  strip.setPixelColor(whichLight, r, g, b);
  strip.show();

}


void loop(){
  static long unsigned int counter = 1;
  
  if (inMozzi == 1) {
    audioHook(); // required here
    return;
  } else {
    stopMozzi();
    if (inMelody == 1) {
      playNextNoteInMelody();
      return;
    } else if (inChuffing == 1) {
      chuffNextChuff();
      return;
    } else if (inWhistle == 1) {
      nextWhistle();
    }
    trainLights();
    if ((counter % FLICKERS_PER_SOUND) == 0) {
      // int chooseNoise = random(4);
      // chooseNoise = 0;
      if (whichSound < 2) {
        startMelody();
      } else if (whichSound == 2) {
        stationAnnouncement();
      } else if (whichSound == 3) {
        startChuffing();
      } else {
        startWhistle();
      }
      whichSound++;
      if (whichSound > 4) {
        whichSound = 0;
      }
    }
    counter++;
    if (counter > 32000) {
      counter = 0;
    }
  }
}
