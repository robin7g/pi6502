# Raspberry Pi 6502 Processor Hat

A different Apple 1 replica. The picture below shows a Rockwell 6502 processor sitting on top of Raspberry Pi Zero W2. The processor 'hat' is a prototype board and uses wirewrap to connect the 6502 CPU to the Raspberry Pi header. 

The processor is running at about 1.7 MHz (faster than the original Apple 1) and the Raspberry Pi is implementing the memory bus and system clock to implement the functions of an Apple 1. The clock speed can be slowed to match the original 1.023 Mhz of the Apple 1 if desired. 

![6502 Hat in action](doc/Pi6502.jpg)



## Is a Pi fast enough? 
I found this cool pi86 project https://github.com/homebrew8088/pi86 which implements an IBM XT like system with a 8088 processor. The challenge I noted with the pi86 project was that the 8088 processor was not clocking higher than about 300 KHz. Its quite slow, see the video below showing some game play. A part of the objective of this project was to see if the Pi can operate fast enough to run the memory bus and create a 4.7 MHz clock. To do this you have to memory map the GPIOs and a few other tricks. I used a 6502 to prototype this while I wait for my Pi86 board to arrive. So far so good. 

![IMAGE ALT TEXT HERE](https://img.youtube.com/vi/drXkA7xGNrc/0.jpg)](https://www.youtube.com/watch?v=drXkA7xGNrct=197)


## Pi6502 Schematic 

## Pre-requisities 

## Building 

## Running 

