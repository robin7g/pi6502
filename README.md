# Raspberry Pi 6502 Processor Hat

A different Apple 1 replica. The picture below shows a Rockwell 6502 processor sitting on top of Raspberry Pi Zero W2. The processor 'hat' is a prototype board and uses wirewrap to connect the 6502 CPU to the Raspberry Pi header. 

The processor is running at about 1.7 MHz (faster than the original Apple 1) and the Raspberry Pi is implementing the memory bus and system clock to implement the functions of an Apple 1. The clock speed can be slowed to match the original 1.023 Mhz of the Apple 1 if desired. 

![6502 Hat in action](doc/Pi6502.jpg)


## Is a Pi fast enough? 
I found this cool pi86 project https://github.com/homebrew8088/pi86 which implements an IBM XT system with a 8088 processor on a Pi hat. The challenge I noted with the pi86 project was that the 8088 processor was not clocking higher than about 300 KHz. Its slow, see the video below showing some game play. https://youtu.be/drXkA7xGNrc?t=237 

A part of the objective of this project was to see if the Pi can operate fast enough to run the memory bus and create a 4.7 MHz clock. To do this you have to memory map the GPIOs and a few other tricks to save time. I used a 6502 to prototype this while I wait for my Pi86 board to arrive. I'll post a video of the Apple 1 working soon. 


## Pi6502 Schematic 

## Pre-requisities 

You need a Raspberry Pi. I used a Pi Zero W 2 with Raspbian installed to build this project. Run the following command to install the necessary packages. 

```
sudo apt-get install gcc make cc65 libncurses5-dev libncursesw5-dev
```
If you want to install Integer Basic then download apple1basic.bin from the following source and place in the root folder of the project. 
http://retro.hansotten.nl/6502-sbc/apple-1/

## Building 

To build its as simple as running make, the output is a executable called pi6502
```
make 
```

## Running 
You need to run the program as root so that it has access to the GPIOs

```
sudo ./pi6502
```

The system should startup and show a `\` prompt. There is no physical reset switch but you can reset the system by typing/pressing `Ctrl + A`


