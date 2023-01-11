# hoggytrain
Arduino Nano code using Mozzi, Talkie, and Adafruit NeoPixel libraries to provide music, sound effects, and flickering lights for a Hogwarts Express model train

Give a Hogwarts Express model train some sounds and lights: station announcements, steam train chuff-chuff sounds,
plus a snippet of familiar melodies.

The basic idea is that most of the time, the train lights are slightly flickering (and the coal box is glowering), but
every now and then, either a melody is played, the steam engine chuff-chuffs, the whistle plays, or a station announcement is made.

Also, this is explicitly a challenge to see how much can be done with a standard Arduino Nano without any extra hardware (besides
the piezo speakers, WS2812s, and resistor and capacitor to protect / filter the WS2812s). Much more could be done with additional
storage (such as playback of digitized sound clips from, say, ScotRail station announcements or HP movie clips). A beefier processor
such as a Raspberry Pi Zero could easily handle making sounds AND flickering the lights simultaneously. But what's the fun in making
it easy like that?

The station announcements are via the Talkie speech synthesis library, which is hard-coded to Digital Pin 3. A dedicated
piezo speaker is connected to this pin and to Digital Pin 11 (inverted signal, used to boost volume). Talkie is hard coded
to pins 3 and 11 because it uses Timer 2 for its PWM fiddling, and Timer 2 is only supported on those two pins.

  https://reference.arduino.cc/reference/en/libraries/talkie/

All other sounds (melodies, chuff-chuff sounds, train whistle) are via the Mozzi library, which is hard-coded to
Digital Pin 9. A second dedicated piezo speaker is connected to this pin and to GND.

  https://sensorium.github.io/Mozzi/

In addition, it uses 3 tiny addressable WS2812 RGB LEDs to provide the engine's front light, glowing coal fire, and the
passenger cabin's overhead light. The LEDs are in series and are connected to 5V, GND, and Digital Pin 6 for signal. The WS2812s
are controlled via the Adafruit NeoPixel library.

  https://learn.adafruit.com/adafruit-neopixel-uberguide/arduino-library-use

As noted above, the train lights and coal box flicker most of the time, but hold at their current values while sounds are made. This
is done to avoid figuring out how to have both Talkie / Mozzi making sounds at the same time the NeoPixel library is messing with the
WS2812s. Given how much processor bandwith these all use, it's not clear that simultaneous flickering and sound generation is
even possible.

Intended for an Arduino Nano plus two small piezo speakers and 3 WS2812 LEDs. Powered by a small USB 2200 mAh power bank.
To be installed in a medium-sized (2-3 foot) Hogwarts Express train model, such as:
  https://www.4dmodelkit.com/products/harry-potter-hogwarts-express
  https://www.amazon.com/dp/B08LCK7H27

Note that this sketch uses 26+K of program storage space, which is around 87% available of 30K max, plus 1.3+K (67+%) of dynamic
memory from 2K max. It was hoped to include playback of some short audio samples (such as "Allll Aboooard!"), but they just won't fit.

NOTE: To make Talkie louder, I hacked the Talkie.cpp file in in the Talkie library.
  https://github.com/ArminJo/Talkie/blob/master/src/Talkie.cpp
On my Mac: ~/Documents/Arduino/libraries/Talkie/src/Talkie.cpp
Overriding line ~856 from:
  nextPwm = (u0 >> 2) + 0x80;
With these lines added below the #endif on line ~857:
  int tPwm = ((int) u0 * 6) / 8;
  nextPwm = tPwm + 0x80;
This does cause slight distortion of some words (notably sp5_DEPARTURE), but increases the volume of Talkie overall
greatly. For this use, the tradeoff is worth it. Adjust the 6/8 ratio as needed. 5/8 was a little too quiet but
distortion-free. 7/8 was slightly louder but much more distorted.

NOTES/TODOS: 
- Train whistle sounds sucky. Needs more tweaking or a rewrite into a single call with different volume envelopes for each "chime".
  Make the whistle call just one long call with ramp-up/-down on the various whistle parts. Basically, start with WHISTLE_A quickly 
  ramping up in volume by itself, then all 4 whistles ramp up together, then ramp down together, then finally just WHISTLE_A ramps
  slowly down to silence.

- Music uses simple sinewaves and is OK but a bit harsh on the ear. Mozzi can for sure do better, but my synth-fu skills are weak.
  Tunes are familiar but hacked-down snippets from Hedwig's Theme and the Harry Potter Main Theme. They're not perfect, but
  you can recognize them.

- It would be fun to add a "dementor mode" where the all 3 lights turn sickly, slowly changing green, and the "dementors music"
  from the HP movies plays, then someone calls out "Expecto Patronum!", all the lights flash white, then things return to normal. 
  BUT ends up there isn't really any dementor-specific music (besides some bass viol bowing) during the dementor scenes, and, yeah,
  "expecto..." isn't exactly in the Talkie vocabulary. Also, there wouldn't be enough storage to wedge all that into the code.

+ Chuff-chuff had a very distinct whine ever since I turned on the NeoPixel code. Which is weird given that the NeoPixel library
  does not use timers or any of the pins that Mozzi uses. The whine seems to change frequency and is sometimes a warble. It
  doesn't seem to have anything to do with AC line leakage, since running from a battery doesn't make it go away. Turning the
  brightness of the WS2812 LEDs all down to 0 helps, but doesn't make it go away. It doesn't appear until the NeoPixel library
  .begin() or maybe .show() routine is called.
  SOLUTION: Adding the recommended NeoPixel 470 uF capacitor from 5V to GND and inline 470 Ohm resistor on the WS2812 signal line
    made the whine almost totally go away. It's still there, but faint, and can only be heard during chuff-chuffs even though it is
    present during the tunes and whistle sounds. The WS2812s still seem happy with the added components.
  Note: Adding the recommended Mozzi cap and resistor didn't help the whine or Mozzi output quality, but did make the Mozzi output
    volume much quieter. So I removed them.

+ Station announcement is currently "Express 9572 departure from 9 and 3 4...All abort". 
  Yeah, "abort" sounds a lot like "aboard", right? Sure it does.

PINS on Arduino Nano:
  NOT USED: VIN: 4.5V battery +, ended up using USB power bank instead of batteries.
  GND: Next to VIN. To breadboard for WS2812 string -
  +5V: To breadboard for WS2812 string +
  GND: Mozzi speaker -   GND on 6-pin header, pin closest to TX1.
  D3:  Talkie speaker +
  D6:  To breadboard for WS2812 string signal
  D9:  Mozzi speaker +
  D11: Talkie speaker inverted / -

Electrical filters:

Connect GND and +5V from Arduino to breadboard.

W2812 Neopixels:
  - From https://learn.adafruit.com/adafruit-neopixel-uberguide/basic-connections
  - D6 to breadboard point 3.
  - 470 Ohm resistor breadboard point 2 and breadboard point 3.
  - Breadboard point 2 to first WS2812's signal line.
  - Resistor protects WS2812s, prevents spikes on data line
  - 470 uF (100 to 1000 uF) cap between GND and +5V on breadboard, prevents onrush current damage
  - Breadboard GND to first WS2812's GND line
  - Breadboard +5V to first WS2812's +5V line

Mozzi:
  - NOT USED. Left here for reference.
  - From https://sensorium.github.io/Mozzi/learn/output/
  - D9 to breadboard point 7.
  - 270 Ohm resistor between breadboard point 7 and breadboard point 6.
  - Breadboard point 6 connects to Mozzi speaker's + wire
  - 100 nF cap between breadboard point 6 and GND
  - Breadboard GND to Mozzi speaker GND

Talkie:
  - From https://github.com/ArminJo/Talkie#hints
  - No electrical components needed, since a piezo speaker is used?
  - There is a bit of distortion and whine but those seem appropriate for a train station PA system
