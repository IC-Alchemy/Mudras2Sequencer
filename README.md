# Pico Mudras Sequencer
Fully featured sequencer project for the Raspberry Pi Pico2 microcontroller. 
Currently under active development but audio output works and midi output.

### Goal:  
Make a step sequencer that:
feels like performing live instead of turning knobs, 
simple to understand yet difficult to naster
sounds GOOD, and feels GOOD

Mudras is a eurorack module I made that uses a LIDAR sensor to measure the distance from the module as an input for a CV sequencer.
This project expands on that concept by using the same LIDAR technology to sequence multiple parameters on 4 seperate synth voices.


# Hardware

This Sequencer needs a couple custom pieces of hardware (touch matrix and magnetic encoder)  
I will explain how to make them here at some point but if you want to use a Pico for DSP NOW, check out
https://github.com/IC-Alchemy/Pico-DSP-Garden

### Microcontroller: Raspberry Pi Pico2 (RP2350)
### Audio Codec: Uses standard I2S audio output (compatible with common, easy-to-find audio codecs)
### User Interface: 4x8 Touch matrix w/ MPR121, OLED display, distance sensor, home made magnetic encoder

I need to upload guides to making the touch matrix and magnetic encoder.

### What Makes it Unique

This is a 4-voice polyphonic sequencer with:

Most paramaters have per step modulation, each parameter 
has its own track and adjustable length for **easy polymetric sequencing**

Real-time parameter control via **LIDAR sensor**
Per Step parameter control with a **Magnetic Encoder**
Step sequencing with **per-step parameter automation**
MIDI I/O support

Key Files


src/voice/Voice.cpp - The heart of the DSP engine
PicoMudrasSequencer.ino - Main Arduino sketch
src/audio/ - I2S audio interface for the Pico2
src/sequencer/ - Step sequencer logic
src/dsp/ - DSP building blocks (filters, oscillators, effects)

Getting Started

This is an Arduino project designed for the Raspberry Pi Pico2. You'll need the Arduino IDE with RP2040/RP2350 board support and the required libraries (mainly DaisySP for DSP functions).
You DO NOT need to install the Daisyduino library from Arduino IDE (it is fine if you did)  This project uses modified versions of some of the DaisySP files and they are all include in src/DSP
The audio output uses I2S, so you'll need an I2S-compatible DAC/codec. Most common audio breakout boards should work fine.

License

MIT License - see LICENSE file for details.
