# Sequencer One

An open sequencer platform based on common components in the Eurorack format. 

* Module size: 36HP (182.9mm)
* Power:  (+12V);  (-12V); (+5V)

!!! repository "Project Source"

    The project files, including schematic and layout, are available on [github](https://github.com/xdylanm/squencer-1)

## Features

The module is implemented around the [ItsyBitsy M4](https://www.adafruit.com/product/3800) with a SAMD51 core, but can also use the [ItsyBitsy M0](https://www.adafruit.com/product/3727) (SAMD21 core). These development boards are reasonably low cost and have an accessible development ecosystem (Arduino or CircuitPython).

The interface is modeled after the [Korg SQ-1](https://www.korg.com/us/products/dj/sq_1/), featuring

* up to 16 steps with level potentiometers for each step
* run/pause and mode buttons to enable direct configuration
* two pairs of gate and CV (V/oct) outputs that can be used together or independently
* sync and CV (+/-5V) inputs
* a 1" display and control knob for advanced configuration

This interface should enable many variations on sequencing, including quantization, octave shifts and transpositions, and  arpegiation.

## Documentation

[Design](theory.md)

[Assembly Guide](assembly.md)

[Schematic](assets/schematic.pdf)

## References / Inspiration

1.  [Korg SQ-1](https://www.korg.com/us/products/dj/sq_1/)
2.  [Kassutronics Quantizer](https://kassu2000.blogspot.com/2019/10/quantizer.html)
3.  [A Study of Scales](https://ianring.com/musictheory/scales/)