#include <FastLED.h>

// ==== USER CONFIGURATION ====
// Set these for each board!
#define FIRST_CHANNEL 0    // Board 0: 0, Board 1: 8, Board 2: 16, Board 3: 24
#define NUM_CHANNELS 8     // Board 0/1/2: 8, Board 3: 6
#define NUM_LEDS_PER_CHANNEL 15
#define TOTAL_CHANNELS 30  // Total channels on the bus

// Use your actual GPIO pins for channels here:
const uint8_t channelPins[NUM_CHANNELS] = {PA0, PA1, PA2, PA3, PA4, PA5, PA6, PA7};

CRGB leds[NUM_CHANNELS][NUM_LEDS_PER_CHANNEL];

// Frame protocol definitions
#define MAGIC_HEADER "UPXL"
#define MAGIC_SIZE 4
#define MAX_PIXELS 240     // Protocol supports up to 240 RGB pixels per channel

// Buffer sizes
#define UART_BUFFER_SIZE 2048    // Size to hold max possible frame

// CRC32 implementation (polynomial 0x04C11DB7, standard)
uint32_t crc32(const uint8_t *data, size_t len) {
  uint32_t crc = 0xFFFFFFFF;
  for (size_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 1)
        crc = (crc >> 1) ^ 0xEDB88320;
      else
        crc >>= 1;
    }
  }
  return ~crc;
}

// Frame structs
typedef struct {
  char magic[4];        // "UPXL"
  uint8_t channel;
  uint8_t recordType;
} __attribute__((packed)) PBFrameHeader;

typedef struct {
  uint8_t numElements; // 3 (RGB) or 4 (RGBW)
  uint8_t redIndex:2, greenIndex:2, blueIndex:2, whiteIndex:2; // color order
  uint16_t pixels;     // Number of pixels in this channel
} __attribute__((packed)) PBChannel;

// Per-channel pixel buffer (max 240 pixels, 4 elements)
struct PixelBuffer {
  uint8_t numElements;
  uint8_t colorOrder[4]; // [red, green, blue, white]
  uint16_t pixels;
  uint8_t data[MAX_PIXELS * 4]; // RGB or RGBW data
  bool valid;
};

PixelBuffer channelBuffers[NUM_CHANNELS];

void setup() {
  Serial1.begin(2000000);  // 2Mbps
  for (int i = 0; i < NUM_CHANNELS; i++) {
    FastLED.addLeds<WS2812, channelPins[i], GRB>(leds[i], NUM_LEDS_PER_CHANNEL);
    channelBuffers[i].valid = false;
  }
}

// Helper: parse color order bits
void parseColorOrder(uint8_t r, uint8_t g, uint8_t b, uint8_t w, uint8_t *order, uint8_t numElements) {
  order[0] = r;
  order[1] = g;
  order[2] = b;
  if (numElements == 4) order[3] = w;
}

// Frame parsing state
uint8_t uartBuffer[UART_BUFFER_SIZE];
size_t uartPos = 0;

void loop() {
  // Read available serial data into buffer
  while (Serial1.available() && uartPos < UART_BUFFER_SIZE) {
    uartBuffer[uartPos++] = Serial1.read();
  }

  // State machine: scan for valid frames
  size_t scanPos = 0;
  while (scanPos + MAGIC_SIZE + sizeof(PBFrameHeader) <= uartPos) {
    // Look for magic header
    if (memcmp(uartBuffer + scanPos, MAGIC_HEADER, MAGIC_SIZE) == 0) {
      PBFrameHeader frame;
      memcpy(&frame, uartBuffer + scanPos, sizeof(PBFrameHeader));

      // SET_CHANNEL_WS2812
      if (frame.recordType == 1) {
        // Check enough data for PBChannel struct
        if (scanPos + sizeof(PBFrameHeader) + sizeof(PBChannel) > uartPos) break;
        PBChannel chan;
        memcpy(&chan, uartBuffer + scanPos + sizeof(PBFrameHeader), sizeof(PBChannel));

        size_t pixelDataLen = chan.pixels * chan.numElements;
        size_t frameLen = sizeof(PBFrameHeader) + sizeof(PBChannel) + pixelDataLen + 4; // +4 for CRC

        if (scanPos + frameLen > uartPos) break; // Wait for full frame

        // CRC check
        uint32_t receivedCRC;
        memcpy(&receivedCRC, uartBuffer + scanPos + frameLen - 4, 4);
        uint32_t computedCRC = crc32(uartBuffer + scanPos, frameLen - 4);

        if (receivedCRC == computedCRC) {
          // Is this channel on this board?
          int globalCh = frame.channel;
          if (globalCh >= FIRST_CHANNEL && globalCh < FIRST_CHANNEL + NUM_CHANNELS) {
            int chIdx = globalCh - FIRST_CHANNEL;
            // Save buffer
            channelBuffers[chIdx].numElements = chan.numElements;
            parseColorOrder(chan.redIndex, chan.greenIndex, chan.blueIndex, chan.whiteIndex, channelBuffers[chIdx].colorOrder, chan.numElements);
            channelBuffers[chIdx].pixels = min(chan.pixels, MAX_PIXELS);
            memcpy(channelBuffers[chIdx].data, uartBuffer + scanPos + sizeof(PBFrameHeader) + sizeof(PBChannel), pixelDataLen);
            channelBuffers[chIdx].valid = true;
          }
        } else {
          // CRC error: zero out buffer
          int globalCh = frame.channel;
          if (globalCh >= FIRST_CHANNEL && globalCh < FIRST_CHANNEL + NUM_CHANNELS) {
            int chIdx = globalCh - FIRST_CHANNEL;
            channelBuffers[chIdx].valid = false;
          }
        }
        scanPos += frameLen;
        continue;
      }

      // DRAW_ALL
      else if (frame.recordType == 2) {
        size_t frameLen = sizeof(PBFrameHeader) + 4; // header + CRC
        if (scanPos + frameLen > uartPos) break;

        // CRC check
        uint32_t receivedCRC;
        memcpy(&receivedCRC, uartBuffer + scanPos + frameLen - 4, 4);
        uint32_t computedCRC = crc32(uartBuffer + scanPos, frameLen - 4);

        if (receivedCRC == computedCRC) {
          // Draw all valid channels
          for (int ch = 0; ch < NUM_CHANNELS; ch++) {
            PixelBuffer *buf = &channelBuffers[ch];
            if (!buf->valid) {
              // Zero pixels if buffer invalid
              for (int i = 0; i < NUM_LEDS_PER_CHANNEL; i++) leds[ch][i] = CRGB(0,0,0);
            } else {
              for (int i = 0; i < min(NUM_LEDS_PER_CHANNEL, buf->pixels); i++) {
                uint8_t r=0, g=0, b=0;
                uint8_t *px = buf->data + i * buf->numElements;
                // Map color order
                r = px[buf->colorOrder[0]];
                g = px[buf->colorOrder[1]];
                b = px[buf->colorOrder[2]];
                leds[ch][i] = CRGB(r,g,b);
              }
              // Zero out remaining LEDs if buffer smaller than NUM_LEDS_PER_CHANNEL
              for (int i = buf->pixels; i < NUM_LEDS_PER_CHANNEL; i++) leds[ch][i] = CRGB(0,0,0);
            }
          }
          FastLED.show();
        }
        scanPos += frameLen;
        continue;
      }
      // Unknown command, skip frame header
      scanPos += sizeof(PBFrameHeader);
    } else {
      // If not magic header, advance scan position
      scanPos++;
    }
  }

  // Remove processed data from uartBuffer
  if (scanPos > 0) {
    memmove(uartBuffer, uartBuffer + scanPos, uartPos - scanPos);
    uartPos -= scanPos;
  }
}
