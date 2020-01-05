# led-sketch
Inspired by the Etch A Sketch, but with an LED matrix and an Atmega168a

There are currently two versions of the project:

1. ledSketch.c

This was the initial concept for this project, using buttons to control the pattern on an LED matrix.
However, it wasn't completely faithful to the idea of an Etch A Sketch.

2. ledSketchDials.c

This idea came about later, when I realized that I could use potentiometers to read the position of dials.
It's harder to control, but much closer to the actual Etch A Sketch idea.

## Goals
My main goal for this project was simply to experiment with hardware design, software design, and embedded debugging.
However, after brainstorming how to implement this project, I set some sub-goals for techniques I wanted to learn and use.

- Interacting with other ICs

Previously, I had mostly worked with LEDs and the occasional button.
I wanted to research and dig through data sheets to learn how to use more complex ICs.

- Controlling an LED matrix

Although I'm aware of libraries for doing this on an Arduino, I wanted to develop my own driver to interact with the LED matrix.
I think this helped me to understand much better how the LED matrix actually worked.

- Use interrupts to handle real-time inputs

I hadn't ever programmed with interrupts before this project, so this was the most unfamiliar goal to me.
As I'll describe below, this ended up causing most of my problems for the project.

- Use ADCs to handle analog inputs

This goal came about later in the process, when I started making the second version of my project with dials.
I realized that I could read the position of a potentiometer dial by setting up the potentiometer as a voltage divider
and connecting an ADC to the middle pin. This ended up being much easier to implement than I expected.

## Future ideas

- Improvements to the LED matrix driver

I already fixed multiple issues with the LED matrix driver, but there's still more work to be done.
Interrupts caused lots of problems, since the interrupts would affect the process of displaying data to the LED matrix.
This became even worse when I implemented debouncing of button inputs, since the delay to debounce caused even more problems with the display loop.

So far, my fixes were at the software level. In the future, I would like to make improvements at the hardware level.
The most effective fix would be to use one device (microcontroller or FPGA) to handle the display loop
and a separate microcontroller to handle inputs in parallel to the display process.

- Reset function

Currently, I just cycle power to the device to reset the display.
A future iteration could use a button to more effectively implement the process of resetting the sketch.
Even better, I could use a sensor like a gyroscope to reset the display when the device is shaken.
