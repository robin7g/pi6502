/* 
 * 
 * This code is a part of Raspberry Pi 6502 Processor Hat project 
 * and implements an Apple 1 replica using a real 6502 processor 
 * where the Raspberry Pi acts as the memory, I/O bus and
 * system clock for the CPU. 
 * 
 * This file is Copyright 2022 Robin Grosset, all rights reserved.
 * 
 * Code is provided AS IS in the hope it is useful under the terms of 
 * Apache License 2.0
 * 
 * Note that files included in this project may be subject to different
 * license terms which are marked. 
 * 
 * mailbox.h, mailbox.c are copyright Broadcom Europe Ltd and subject to 
 * terms included in those files.
 */

#include <assert.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <ncurses.h>

// Physical Memory Allocation, from raspberrypi/userland demo.
#include "mailbox.h"


// Don't bother auto-detecting but have it a compile-time option.
// Change if you want to compile for another Raspberry Pi board
// Originally developed on a Raspberry Pi Zero W2 which is 
// essentially  a PI 3
#ifndef PI_VERSION
#  define PI_VERSION 3
#endif

#define BCM2708_PI1_PERI_BASE  0x20000000
#define BCM2709_PI2_PERI_BASE  0x3F000000
#define BCM2711_PI4_PERI_BASE  0xFE000000

// General, Pi-specific setup.
#if PI_VERSION == 1
#  define PERI_BASE BCM2708_PI1_PERI_BASE
#elif PI_VERSION == 2 || PI_VERSION == 3
#  define PERI_BASE BCM2709_PI2_PERI_BASE
#else
#  define PERI_BASE BCM2711_PI4_PERI_BASE
#endif

#define PAGE_SIZE 4096

// GPIO specific defines
#define GPIO_REGISTER_BASE 0x200000
#define GPIO_SET_OFFSET 0x1C
#define GPIO_CLR_OFFSET 0x28
#define PHYSICAL_GPIO_BUS (0x7E000000 + GPIO_REGISTER_BASE)

#define GPIO_LEV0_OFFSET 0x34
#define GPIO_LEV1_OFFSET 0x38

// ---- Memory mappping defines
#define BUS_TO_PHYS(x) ((x)&~0xC0000000)

// ---- Memory allocating defines
// https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface
#define MEM_FLAG_DIRECT           (1 << 2)
#define MEM_FLAG_COHERENT         (2 << 2)
#define MEM_FLAG_L1_NONALLOCATING (MEM_FLAG_DIRECT | MEM_FLAG_COHERENT)




// Return a pointer to a periphery subsystem register.
static void *mmap_bcm_register(off_t register_offset) {
  const off_t base = PERI_BASE;

  int mem_fd;
  if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) {
    perror("can't open /dev/mem: ");
    fprintf(stderr, "You need to run this as root!\n");
    return NULL;
  }

  uint32_t *result =
    (uint32_t*) mmap(NULL,                  // Any adddress in our space will do
                     PAGE_SIZE,
                     PROT_READ|PROT_WRITE,  // Enable r/w on GPIO registers.
                     MAP_SHARED,
                     mem_fd,                // File to map
                     base + register_offset // Offset to bcm register
                     );
  close(mem_fd);

  if (result == MAP_FAILED) {
    fprintf(stderr, "mmap error %p\n", result);
    return NULL;
  }
  return result;
}

void set_databus_pins_to_output(volatile uint32_t *gpio_registerset){
  // Objective: Set pins 0 to 7 to output mode as fast as possible.
  // 
  // How: The gpio_registerset address contains 1st 10 GPIO pins configs 
  // in bits 0 to 29 each IO pin has 3 configuration bits. To set to 
  // output I need to set the following bit pattern '001'. Note bits 30 
  // and 31 are not used. To set the first 8 pins to output leaving 
  // others as input I need to set a binary pattern like the following. 
  //
  // Pin#    10  9   8   7   6   5   4   3   2   1   0
  // Binary 000 000 000 001 001 001 001 001 001 001 001
  //
  // which is 
  //
  // 1+8+64+512+4096+32768+262144+2097152 = 2396745
  //
  // Magic number time! 
  // A single assignment sets all pins to the right state. 
  *(gpio_registerset) = 2396745;
}

void set_databus_pins_to_input(volatile uint32_t *gpio_registerset){
  // pins 0 to 7 set as fast as possible
  // This is easier the bit pattern of input is '000' 
  // so just set everything to zero 
  *(gpio_registerset) = 0;
}


void initialize_gpio_for_output(volatile uint32_t *gpio_registerset, int bit) {
  *(gpio_registerset+(bit/10)) &= ~(7<<((bit%10)*3));  // prepare: set as input
  *(gpio_registerset+(bit/10)) |=  (1<<((bit%10)*3));  // set as output.
}

// Data Lines D0-D7 connect to GPIO 0-7 
// Address Lines A0-A15 connect to GPIO 8-23
// This wiring makes the coding easier. 
// GPIO defines outside of these
#define GPIO_PH0   24
#define GPIO_RESET 25
#define GPIO_PH2   26
#define GPIO_RW    27
#define GPIO_TIMEWASTE 26

// Apple 1 I/O Address Registers
#define DSP   0xD012
#define DSPCR 0xD013
#define KBD   0xD010
#define KBDCR 0xD011

// Control Key Macro
#define ctrl(x)    ((x) & 0x1f)


void perform_memory_read(unsigned char * memory, int address,volatile uint32_t *set_reg,volatile uint32_t *clr_reg ){
  unsigned char readDataByte = memory[address];
  // set output value
  *set_reg = readDataByte&0xff;
  *clr_reg = ~readDataByte&0xff;

  if (address >=0xD000 && address <=0xDFFF) {
    // Reading keyboard reset CR after 
    if (address == 0xD010) {
      memory[0xD010] =  0x00;
      memory[0xD011] = 0x00;
    }
  } else {
    //if (busDebug)
    //  printf("o");
  }
}
void perform_memory_write(unsigned char * memory, int address, char writeDataByte, WINDOW * console ){
  // This code implements memory writes inclyding I/O write operations, 
  // and checks ROM vs RAM memory writes.
  if (address >=0xD000 && address <=0xDFFF) {
    // Writing to I/O 
    memory[address] = writeDataByte;
    char outputChar = writeDataByte & 0x7F;
    if (address == 0xD012){     
      switch (outputChar){
        case 0:
        case 0x7f:
          // No nuffin
          break;
        case 0xA: //\n
          break;
        case 0xD: //\r
            waddch(console, '\n');
            wrefresh(console);
          break;
        default:
            waddch(console, outputChar);
            wrefresh(console);    
          break;
      }
      // set memory to 00 as character has been output
      memory[address] = 0x00;   
    }   
  } else if (address >= 0xE000 && address <= 0xEFFF) { 
    // Ignore writes to the
    // Apple 1 Basic ROM area.
  } else if (address >= 0 && address<0xFF00) {
    // Normal write to RAM
    memory[address] = writeDataByte;      
  } else {
    printf("Illegal memory write to address 0x%04x",address);
  }
}

void reset_sequence(volatile uint32_t *set_reg, volatile uint32_t *clr_reg, int delay) {

  int startup, loop;
  // 6502 RESET Interrupt process.

  *set_reg = (1<<GPIO_RESET);  //RESET HIGH
  for(startup=0;startup<1000;startup++) {    
    // Pulse the clock a bit before the RESET cycle. 
    // to allow the clock/cpu to settle at first power up      
    *clr_reg = (1<<GPIO_PH0); // CPU LOW
    for(loop=0;loop<delay;loop++) *set_reg = (1<<GPIO_TIMEWASTE);
    *set_reg = (1<<GPIO_PH0); // CPU HIGH  
    for(loop=0;loop<delay;loop++) *set_reg = (1<<GPIO_TIMEWASTE);
    // After 100 clock cycles pull the RESET line LOW
    if (startup==100) {   
      *clr_reg = (1<<GPIO_RESET); //RESET LOW for remainder of startup loop
    }
  }
  // Now set the CPU into run mode by setting REST HIGH
  *set_reg = (1<<GPIO_RESET);  //RESET HIGH
}

void load_rom(char * filename, unsigned char * memory, int address_base, int size) {
  if( access(filename, F_OK ) == 0 ) {
    FILE * f = fopen(filename, "r");
    fread(&(memory[address_base]), 1, size, f);
    fclose(f);
  } 
}

void bus_loop(WINDOW * console) {

  // Prepare GPIO ports by mapping GPIOs into address space. 
  volatile uint32_t *gpio_port = mmap_bcm_register(GPIO_REGISTER_BASE);
  initialize_gpio_for_output(gpio_port, GPIO_PH0);
  initialize_gpio_for_output(gpio_port, GPIO_RESET);
  volatile uint32_t *set_reg = gpio_port + (GPIO_SET_OFFSET / sizeof(uint32_t));
  volatile uint32_t *clr_reg = gpio_port + (GPIO_CLR_OFFSET / sizeof(uint32_t));
  volatile uint32_t *lev0_reg = gpio_port + (GPIO_LEV0_OFFSET / sizeof(uint32_t));
  
  // The delay value is the number of cycles in each half of a clock cycle.
  // The following was measured on an overclocked Raspberry Pi Zero W2 
  // (arm_freq=1200 MHz)
  // Delay 15 = 1.77MHz CPU clock.  
  // Delay 40 = 1.015MHz CPU clock like the original Apple 1. 
  int delay = 15; 
  long loop;
  uint64_t cycle = 1; 
  int reset_flag = 1;
  unsigned char memory[65535];

  // Initialise all 65K of memory space to zero 
  for(loop=0;loop<65535;loop++){
    memory[loop] = 0;
  }

  /* 
   * Load apple1rom.bin 
   * 
   * This is the Woz Monitor written by Steve Wozniak in 1976. 
   * Instructions on how to use and links to source code can 
   * be found here
   * https://www.sbprojects.net/projects/apple1/wozmon.php
   * 
   * The original source code was included in the Apple 1 
   * operating manual http://retro.hansotten.nl/6502-sbc/apple-1/
   * 
   * The Makefile compiles this rom using CC65 assembler
   * to show how this is done. 
   */ 
  load_rom("apple1rom.bin", memory,0xFF00, 256);


  /* Loading apple1basic.bin if it exists
   * 
   * This is 4K integer basic also created by Woz. 
   * https://en.wikipedia.org/wiki/Integer_BASIC
   * 
   * To run basic type 'E000R' and hit return.  
   * 
   * You can find the Apple 1 software images including
   * operating manual online here 
   * http://retro.hansotten.nl/6502-sbc/apple-1/
   * 
   */
  load_rom("apple1basic.bin", memory,0xE000, 4096);

  // Main loop is the memory, I/O bus and clock for the 6502
  for(;;){  

    // CPU must be reset at first boot and if the 
    // reset flag is subsequently set. (Ctrl+A)
    if (reset_flag) {
      reset_sequence(set_reg,clr_reg, delay);
      reset_flag = 0;
    }

    *clr_reg = (1<<GPIO_PH0); // LOW CPU CLOCK
    for(loop=0;loop<delay/2;loop++) *set_reg = (1<<GPIO_TIMEWASTE);
    // Middle of low clock pulese reset data pins to input mode 
    // to allow CPU to use them. 
    set_databus_pins_to_input(gpio_port);
    for(loop=0;loop<delay/2;loop++) *clr_reg = (1<<GPIO_TIMEWASTE);

    // End of Low clock pulse read RW line ,Address bus
    char readWriteSignal = ((*lev0_reg)>>27) & 0x01 ? 'R' : 'W' ;    
    int address = ((*lev0_reg)>>8) &0xffff;

    *set_reg = (1<<GPIO_PH0); // HIGH CPU CLOCK
    for(loop=0;loop<delay/2;loop++) *set_reg = (1<<GPIO_TIMEWASTE);
    // Perform data and memory accesses in the middle of HIGH clock
    if (readWriteSignal == 'R') {
        // Read data from address
        set_databus_pins_to_output(gpio_port);        
        perform_memory_read(memory, address, set_reg, clr_reg);
    } else { 
        // Write data to address 
        set_databus_pins_to_input(gpio_port);
        unsigned char writeDataByte = ((*lev0_reg)&0xff);
        perform_memory_write(memory, address, writeDataByte, console);
    }    
    for(loop=0;loop<delay/2;loop++) *set_reg = (1<<GPIO_TIMEWASTE);

    // keep track of cycles. 
    cycle++;
  
    // Check for keyboard input every 50 cycles
    if (cycle % 50 == 0){
      int keyb = getch();
      if (keyb != ERR) {
        // Check for Ctrl+A keys = RESET
        if (keyb == ctrl('a') || keyb == ctrl('A')) {
          reset_flag = 1;
          wprintw(console, " RESET \n\n");
          wrefresh(console); 
        } else {
          if (keyb == 0x0a) // Convert \n LF to \r CR
            keyb = 0x0d;
          memory[0xD010] = (keyb & 0x7f) | 0x80;
          memory[0xD011] = 0x80;
        }
      }
    }// end keyboard        
  }// end infinite loop
}

int main() {
  // Setup ncurses
  int height, width;
  WINDOW *console;
  initscr();
  nodelay(stdscr, true);
  noecho();  
  getmaxyx(stdscr, height, width);
  console = newwin(height - 2, width - 2, 1, 1);
  scrollok(console,TRUE);

  // Runs the Apple 1
  bus_loop(console);

  endwin();
  return 0;
}