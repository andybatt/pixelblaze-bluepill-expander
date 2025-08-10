# pixelblaze-bluepill-expander
Pixelblaze Output Expander with STM32 Blue Pill: Distributes serial pixel data from Pixelblaze to 4 STM32F103C8T6 boards, each driving 8 LED channels with FastLED.

# Pixelblaze Blue Pill Output Expander

This project enables a Pixelblaze controller to drive up to 32 channels of addressable LEDs using four STM32F103C8T6 ("Blue Pill") boards. Each Blue Pill receives serial pixel data and outputs up to 8 channels, each controlling up to 15 LEDs via FastLED.

**Features:**
- Distributes Pixelblaze serial data to multiple Blue Pills
- Each Blue Pill controls 8 channels of WS2812 LEDs
- Each channel supports 15 LEDs (easily adjustable)
- Example wiring and schematic included

**Usage:**
Flash each Blue Pill with its assigned `.ino` file. Connect Pixelblaze TX to all Blue Pills' RX1 (PA10) pins, and wire each output pin to its LED strip's data input with a series resistor and power capacitor.

See comments in the code and project documentation for wiring details.
