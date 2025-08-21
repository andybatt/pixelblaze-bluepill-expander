#include "PBDriverAdapter.hpp"

PBDriverAdapter driver;

// How many output channels per board (8 for PA0–PA7)
const int channelCount = 8;
// How many pixels per channel (adjust as needed)
const int pixelsPerChannel = 15;

void setup() {
  // Configure 8 output channels
  std::unique_ptr<std::vector<PBChannel>> channels(new std::vector<PBChannel>(channelCount));
  for (int i = 0; i < channelCount; i++) {
    (*channels)[i].channelId = i;
    (*channels)[i].channelType = CHANNEL_WS2812;
    (*channels)[i].numElements = 3; // RGB
    (*channels)[i].redi = 1;   // Color order: GRB (adjust as needed)
    (*channels)[i].greeni = 0;
    (*channels)[i].bluei = 2;
    (*channels)[i].pixels = pixelsPerChannel;
    (*channels)[i].startIndex = i * pixelsPerChannel;
    (*channels)[i].frequency = 800000; // WS2812 frequency
  }
  driver.configureChannels(std::move(channels));
  driver.begin();
  // Set up Serial1 for input on PA10 (PixelBlaze TX)
Serial1.end();
Serial1.begin(2000000); // PA10 is RX1 by default on Blue Pill
}

void loop() {
  unsigned long ms = millis();
  int channelId = 0;
  uint8_t rgb[3] = {0, 0, 0};

  // This callback is used to fill pixel data for each channel
  driver.show(pixelsPerChannel * channelCount, [&](uint16_t index, uint8_t rgbv[]) {
    uint8_t hue = channelId * 32;
    float v = (1 + sinf(fmodf((ms + channelId*100) / 160.0, PI)))/2;
    v = v*v;
    if (hue < 85) {
      rgb[0] = hue * 3 * v;
      rgb[1] = (255 - hue * 3) * v;
      rgb[2] = 0;
    } else if (hue < 170) {
      hue -= 85;
      rgb[0] = (255 - hue * 3) * v;
      rgb[1] = 0;
      rgb[2] = hue * 3 * v;
    } else {
      hue -= 170;
      rgb[0] = 0;
      rgb[1] = hue * 3 * v;
      rgb[2] = (255 - hue * 3) * v;
    }
    memcpy(rgbv, rgb, 3);
  }, [&](PBChannel * ch) {
    channelId = ch->channelId;
  });
}