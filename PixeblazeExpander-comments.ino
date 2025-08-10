#include <FastLED.h>

// Configuration for this board
#define FIRST_CHANNEL 8         // Change this per board (e.g., 0, 8, 16, 24)
#define NUM_CHANNELS 8          // How many channels this board handles
#define NUM_LEDS_PER_CHANNEL 15 // Number of LEDs per channel

const uint8_t channelPins[NUM_CHANNELS] = {PA0, PA1, PA2, PA3, PA4, PA5, PA6, PA7}; // Change per board if needed
CRGB leds[NUM_CHANNELS][NUM_LEDS_PER_CHANNEL];

void setup() {
  Serial1.begin(115200); // Match Pixelblaze output baud rate
  for (int i = 0; i < NUM_CHANNELS; i++) {
    FastLED.addLeds<WS2812, channelPins[i], GRB>(leds[i], NUM_LEDS_PER_CHANNEL);
  }
}

void loop() {
  // Buffer size for one frame of all channels
  const int FRAME_SIZE = 64 * 15 * 3; // 64 channels * 15 LEDs * 3 bytes (RGB)
  static uint8_t frameBuffer[FRAME_SIZE];
  static int framePos = 0;

  // 1. Read serial data into buffer
  while (Serial1.available() && framePos < FRAME_SIZE) {
    frameBuffer[framePos++] = Serial1.read();
  }

  // 2. When full frame received
  if (framePos == FRAME_SIZE) {
    // 3. Extract and display channels for this board
    for (int ch = 0; ch < NUM_CHANNELS; ch++) {
      int globalCh = FIRST_CHANNEL + ch;
      int channelOffset = globalCh * NUM_LEDS_PER_CHANNEL * 3;
      for (int led = 0; led < NUM_LEDS_PER_CHANNEL; led++) {
        int pixelOffset = channelOffset + led * 3;
        leds[ch][led] = CRGB(
          frameBuffer[pixelOffset],
          frameBuffer[pixelOffset + 1],
          frameBuffer[pixelOffset + 2]
        );
      }
    }
    FastLED.show();
    framePos = 0; // Ready for next frame
  }
}
