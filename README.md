# Raspberry Pi 6502 Processor Hat

A different Apple 1 replica. The picture below shows a Rockwell 6502 processor sitting on top of Raspberry Pi Zero W2. The processor 'hat' is a prototype board and uses wirewrap to connect the 6502 CPU to the Raspberry Pi header. 

The processor is running at about 1.7 MHz (faster than the original Apple 1) and the Raspberry Pi is implementing the memory bus and system clock to implement the functions of an Apple 1. The clock speed can be slowed to match the original 1.023 Mhz of the Apple 1 if desired. 

![6502 Hat in action](doc/Pi6502.jpg)



## Why? Speed. ##
I made this because I came across the very cool pi86 project https://github.com/homebrew8088/pi86 which implements an IBM XT like system with a 8088 processor in a similar Raspberry Pi hat setup. (I have ordered the 8086 hat). I wanted to see if a 6502 could work the same way. The 6502 supports 3.3v levels which 8088/8086 does not unless you get a special NEC V20/V30 versions and I had a 6502 CPU lying around so I could try this out right away. 

The challenge I noted with the pi86 project was that the processor was not clocking higher than about 300 KHz. This is not close to realistic speeds. The Raspberry Pi was not driving the IO pins fast enough. See the video below showing some game play. A part of the objective of this project was to see if the Pi can operate fast enough to create a 4.7 MHz clock, I am currently able to do about 2 Mhz with the 6502 and there is room for more optimization. To do this you have memory map the GPIOs and avoid configuring, reading or writing the GPIO pins one by one. 

Maximum possible clock speed you can generate with a Pi appears to be above 100 Mhz if you drive the pins directly (my Oscilloscope and logic analyser can't measure above about 50 MHz) however the Pi has to not just clock the CPU but operate the bus to read and write memory, reads and writes typically have to happen during a single clock cycle. 

![IMAGE ALT TEXT HERE](https://img.youtube.com/vi/drXkA7xGNrc/0.jpg)](https://www.youtube.com/watch?v=drXkA7xGNrct=197)


## Pi6502 Schematic ## 

## Pre-requisities ##

## Building ##

## Running ##

