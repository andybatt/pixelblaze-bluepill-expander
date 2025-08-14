#include <FastLED.h>
#define FIRST_CHANNEL 8
#define NUM_CHANNELS 8
#define NUM_LEDS_PER_CHANNEL 15
const uint8_t channelPins[NUM_CHANNELS] = {PA0, PA1, PA2, PA3, PA4, PA5, PA6, PA7};
CRGB leds[NUM_CHANNELS][NUM_LEDS_PER_CHANNEL];
void setup() {
  Serial1.begin(115200);
  for (int i = 0; i < NUM_CHANNELS; i++) {
    FastLED.addLeds<WS2815
      , channelPins[i], GRB>(leds[i], NUM_LEDS_PER_CHANNEL);
  }
}
void loop() {
  const int FRAME_SIZE = 64 * 15 * 3;
  static uint8_t frameBuffer[FRAME_SIZE];
  static int framePos = 0;
  while (Serial1.available() && framePos < FRAME_SIZE) {
    frameBuffer[framePos++] = Serial1.read();
  }
  if (framePos == FRAME_SIZE) {
    for (int ch = 0; ch < NUM_CHANNELS; ch++) {
      int globalCh = FIRST_CHANNEL + ch;
      int channelOffset = globalCh * NUM_LEDS_PER_CHANNEL * 3;
      for (int led = 0; led < NUM_LEDS_PER_CHANNEL; led++) {
        int pixelOffset = channelOffset + led * 3;
        leds[ch][led] = CRGB(frameBuffer[pixelOffset], frameBuffer[pixelOffset + 1], frameBuffer[pixelOffset + 2]);
      }
    }
    FastLED.show();
    framePos = 0;
  }
}
