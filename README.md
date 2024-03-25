# Introduction 
The primary goal of this lab was to encompass and apply all the knowledge we learned within this class. This involved a basic program that can simultaneously compute FFTs while also performing real-time tasks. Further, we designed our final project to be based on the very famous game of Tetris. The game of Tetris works by having different blocks of different sizes and shapes fall into a platform. The goal is to create horizontal lines without any gaps. As lines are completed, they disappear (increasing your score), and the game continues with increasing speed. The goal is to prevent the stack of blocks from reaching the top of the screen for as long as possible. This project attempts to solve the problem of recreating an iconic game to cure boredom while displaying what we’ve learned from our time taking Embedded Systems. We intend to implement this by using the Arduino Mega in conjunction with a set of peripheral devices (keypad, display, buttons, etc.) running on the FreeRTOS platform to schedule our functional tasks.

# Objective 
* Implement and explain the usage of FreeRTOS
* Be able to explain how round robin schedulers work for FreeRTOS
* Be able to explain how schedulers can use Task Control Blocks (TCBs) for each task


The successful completion of this lab will give us a comprehensive understanding of how FreeRTOS uses different scheduling techniques to balance various tasks such as handling button inputs, updating the game display, and controlling the gravity effect in the Tetris game.

# Implementation Details (FFT)
The FFT code was structured around a few basic high-level tasks: blinking an offboard LED, playing the Tetris theme, tracking the runtime of an FFT & printing it, and actually running the FFT.

## RT1 - Blink LED
This function repeatedly blinks an external LED attached to pin 47. The code accomplishes this by setting this pin to high, calling a vTaskDelay of 100ms, setting the pin to low, then calling a vTaskDelay of 200ms. This function runs forever.

## RT2 - Play Tetris Theme
This function repeatedly plays the Tetris theme 3 times, waiting 1.5 sec between plays, then stopping to an external buzzer attached to pin 6. The code accomplishes this by cycling through the melody array, calling the “set” function to set the current value of the period to accomplish a desired sound. Following its third run, the function deletes itself from the schedule.

## RT3p0 - Initialize FFT
This function runs before the scheduler starts, setting up an array of random doubles for the FFT to compute, as well as creating queues for inter-task communication, and creating the remaining FFT tasks. Notably, this function passes the random double array into the parameters for RT3p1.

## RT3p1 - Show FFT time
This function takes its random doubles from RT3p0 and passes them into the queue for RT4 to receive. Once the queue receives a new time value, it prints the current time value to the serial monitor. After 5 runs, this function suspends itself indefinitely.

## RT4 - Compute FFT
This function uses the ArduinoFFT library to compute an FFT of the set of random doubles sent to it via the queue from RT3p1. Once this function has completed, it returns the current time value of how long it has been running for to the queue to be reported by RT3p1

## Set
This function takes in a frequency value and sets the OCRA4 (i.e. the output compare register value for timer 4) to the relevant period in order to achieve the desired sound.

# Implementation Details (Tetris Simulation)
## Button Setup
This module initializes the pins for the left and right buttons and defines their states. These buttons are used for rotating the Tetris pieces.

## Keypad Setup
This module sets up the 4x4 keypad, which is used for controlling the movement of the Tetris pieces. It defines the keys, row and column pinouts, and creates a Keypad object. Seven Segment

## Display Setup
This module initializes two seven-segment displays, one for showing the current score and the other for displaying the game's difficulty level.
Tetris Blocks Definition
This module defines the arrays for all the Tetris blocks (line, J-piece, L-piece, box, S-piece, Z-piece, and T-piece) in their initial orientations.

## Piece Structure
This module defines a struct for a Tetris piece, containing its position and shape. It also includes functions for generating new pieces and checking for collisions.

## Movement and Rotation
This module contains functions for moving the current piece left, right, or down, and for rotating it clockwise or counterclockwise. These functions check for collisions before applying the transformations.

## Line Clearing
This module includes functions for checking if any lines are completed and need to be cleared, and for clearing those lines from the grid.
Grid Management
This module handles adding the current piece to the grid when it can no longer move down and initializing the game board with borders for collision detection.

## Display Functions
This module contains a function for displaying the current state of the game board on an LED matrix using SPI communication.
Task Definitions
This module defines the FreeRTOS tasks, such as blinking an LED, handling gravity (moving the current piece down periodically), initializing the game board, displaying the game board, and reading input from the keypad and buttons.

## Setup and Loop Functions
The setup() function initializes the hardware components, seeds the random number generator, and creates the FreeRTOS tasks. The loop() function is empty, as the tasks are managed by the FreeRTOS scheduler.

# Hardware Setup
![graph](Tetris_FreeRTOS/doc/ref1.png)
