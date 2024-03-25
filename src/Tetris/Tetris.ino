#include <Arduino_FreeRTOS.h>
#include <task.h>
#include <SPI.h>
#include <Keypad.h>
#include <LedControl.h>
#include "SevSeg.h"

// ******************** BUTTON SETUP START ********************
const int leftButtonPin = 2;  // the number of the pushbutton pin
const int rightButtonPin = 3;

int leftButtonState = 0;
int rightButtonState = 0;
// ******************** BUTTON SETUP STOP ********************

// ******************** KEYPAD SETUP START ********************
const int ROW_NUM = 4;    // DO NOT CHANGE | four rows
const int COLUMN_NUM = 4; // DO NOT CHANGE | four columns

char keys[ROW_NUM][COLUMN_NUM] = {
  {'1','2','3', 'A'},
  {'4','5','6', 'B'},
  {'7','8','9', 'C'},
  {'*','0','#', 'D'}
};

volatile int difficulty;

byte pin_rows[ROW_NUM] = {31, 29, 27, 25};      //connect to the row pinouts of the keypad
byte pin_column[COLUMN_NUM] = {39, 37, 35, 33}; //connect to the column pinouts of the keypad

Keypad keypad = Keypad( makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM );
// ******************** KEYPAD SETUP STOP ********************

// ******************** SEVEN SEGMENT DISPLAY SETUP START ********************
SevSeg sevseg; // Instantiate a seven segment controller object
SevSeg sevseg_single; // Instantiate a seven segment controller object
// ******************** SEVEN SEGMENT DISPLAY SETUP STOP ********************

TaskHandle_t show_handle;
TaskHandle_t gravity_handle;
TaskHandle_t init_handle;
TaskHandle_t piece_init_handle;

int DIN = 12;
int CS =  11;
int CLK = 10;

LedControl lc = LedControl(DIN, CLK, CS, 0);

#define DIN_MASK (1 << 4)  // Pin 12 on PORTB
#define CS_MASK  (1 << 5)  // Pin 11 on PORTB
#define CLK_MASK (1 << 6)  // Pin 10 on PORTB

#define configUSE_PREEMPTION 0
#define INCLUDE_vTaskDelete 1
#define INCLUDE_xTaskAbortDelay 1

// Define arrays for all Tetris Blocks 
const int line[4][4]    {	{1, 1, 1, 1}, 
                          {0, 0, 0, 0}, 
                          {0, 0, 0, 0}, 
                          {0, 0, 0, 0} }; // INITIALLY HORIZONTAL
const int j_piece[4][4] { {1, 0, 0, 0}, 
                          {1, 1, 1, 0}, 
                          {0, 0, 0, 0}, 
                          {0, 0, 0, 0} }; // INITIALLY POINTS UP
const int l_piece[4][4] {	{1, 1, 1, 0}, 
                          {1, 0, 0, 0}, 
                          {0, 0, 0, 0}, 
                          {0, 0, 0, 0} }; // INITIALLY POINTS DOWN
const int box[4][4]     {	{1, 1, 0, 0}, 
                          {1, 1, 0, 0}, 
                          {0, 0, 0, 0}, 
                          {0, 0, 0, 0} };
const int s_piece[4][4] {	{1, 1, 0, 0}, 
                          {0, 1, 1, 0}, 
                          {0, 0, 0, 0}, 
                          {0, 0, 0, 0} }; // INITIALLY "FLAT"
const int z_piece[4][4] {	{0, 1, 1, 0}, 
                          {1, 1, 0, 0}, 
                          {0, 0, 0, 0}, 
                          {0, 0, 0, 0} }; // INITIALLY "FLAT"
const int tee[4][4]     {	{0, 1, 0, 0}, 
                          {1, 1, 0, 0}, 
                          {0, 1, 0, 0}, 
                          {0, 0, 0, 0} }; // iNITIALLY POINTS LEFT

static int score = 0;

// Stores relevant info about a given piece
struct piece {
  int x; // Current x position of the top left of the piece (in 4x4)
  int y; // Current y position of the top left of the piece (in 4x4)
  int block[4][4];
} piece_struct;

struct piece cur_piece;



// NOTE: need to create a "cup" shape of 1s for collision on the borders.
int grid[10][9]; 

// Return a new random piece struct
struct piece get_new_piece(){
 struct piece new_piece;
  new_piece.x = 3;
  new_piece.y = 0;
  switch (random(1,8)) {
    case 1:
      for (int i = 0; i < 4; i++){
        for (int j = 0; j < 4; j++){
          new_piece.block[i][j] = line[j][i];
        }
      }
      break;
    case 2:
      for (int i = 0; i < 4; i++){
        for (int j = 0; j < 4; j++){
          new_piece.block[i][j] = j_piece[j][i];
        }
      }
      break;
    case 3:
      for (int i = 0; i < 4; i++){
        for (int j = 0; j < 4; j++){
          new_piece.block[i][j] = l_piece[j][i];
        }
      }
      break;
    case 4:
      for (int i = 0; i < 4; i++){
        for (int j = 0; j < 4; j++){
          new_piece.block[i][j] = box[j][i];
        }
      }
      break;
    case 5:
      for (int i = 0; i < 4; i++){
        for (int j = 0; j < 4; j++){
          new_piece.block[i][j] = s_piece[j][i];
        }
      }
      break;
    case 6:
      for (int i = 0; i < 4; i++){
        for (int j = 0; j < 4; j++){
          new_piece.block[i][j] = z_piece[j][i];
        }
      }
      break;
    case 7:
      for (int i = 0; i < 4; i++){
        for (int j = 0; j < 4; j++){
          new_piece.block[i][j] = tee[j][i];
        }
      }
      break;
    default:
      break;
  }
  return new_piece;
}

// Creates a copy of the current piece to protect memory if it is not going to be overwritten
struct piece getCurCopy(){
  struct piece cur_copy;
  cur_copy.x = cur_piece.x;
  cur_copy.y = cur_piece.y;
  for(int x = 0; x < 4; x++){
    for(int y = 0; y < 4; y++){
      cur_copy.block[x][y] = cur_piece.block[x][y];
    }
  }
  return cur_copy;
}

// Basically the most important function in this code
bool collision_check(struct piece next_piece){
  for(int x = 0; x <= 3; x++){
    for(int y = 0; y <= 3; y++){
      if(next_piece.block[x][y] && grid[next_piece.x + x][next_piece.y + y]){
        return 1;
      }
    }
  }
  return 0;
}


// ***************** CURRENT PIECE ROTATION & MOVEMENT *****************
struct piece move_left(){
  struct piece new_piece = getCurCopy();
  new_piece.x--;
  if(collision_check(new_piece)) {
    return cur_piece;
  }
  else {
    return new_piece;
  }
}

struct piece move_right(){
  struct piece new_piece = getCurCopy();
  new_piece.x++;
  if(collision_check(new_piece)) {
    return cur_piece;
  }
  else {
    return new_piece;
  }
}

struct piece move_down(){
  struct piece new_piece = getCurCopy();
  new_piece.y++;
  if(collision_check(new_piece)) {
    return cur_piece;
  }
  else {
    return new_piece;
  }
}

struct piece rotate_left() {
  struct piece new_piece = getCurCopy();
  for (int i = 0; i < 4; i++){
    for(int j = 0; j < 4; j++){
        new_piece.block[j][3 - i] = cur_piece.block[i][j];
    }
  }
  if(collision_check(new_piece)){
    return cur_piece;
  }
  else{
    return new_piece;
  }
}

struct piece rotate_right() {
  struct piece new_piece = getCurCopy();
  for (int i = 0; i < 4; i++){
    for(int j = 0; j < 4; j++){
        new_piece.block[3 - j][i] = cur_piece.block[i][j];
    }
  }
  if(collision_check(new_piece)){
    return cur_piece;
  }
  else{
    return new_piece;
  }
}

// ***************** LINE CLEARING FUNCTIONALITY *****************


// Based on check algorithm from https://circuitdigest.com/microcontroller-projects/creating-tetris-game-with-arduino-and-oled-display
void line_clear_check(){
  bool clear;
  int lines = 0;
  for(int y = 7; y >= 0; y--){
    clear = true;

    // If any of the parts of the line in the grid are off, this will change clear to zero
    for(int x = 8; x >= 1; x--){
      clear = clear && grid[x][y];
    }

    if(clear){
      clearline(y);
      y++; // Makes sure the system still checks the newly moved line after clearline is done
      lines++;
    }
  }

  // Single line clear = 40pts
  if(lines == 1){
    score += 40;
  } 
  
  // Double line clear = 50pts
  else if (lines == 2){
    score += 50;
  } 
  
  // Triple line clear = 100pts
  else if (lines == 3){
    score += 100;
  } 

  // Tetris clear = 300pts
  else if (lines == 4){
    score += 300;
  }
  if(difficulty < 5){
    difficulty = score / 100;
  }
  

  sevseg.setNumber(score, 0);
  sevseg_single.setNumber(difficulty, 0);
}

// Based on clear algorithm from https://circuitdigest.com/microcontroller-projects/creating-tetris-game-with-arduino-and-oled-display
void clearline(int cleared_line){

  // Shift down every row starting from the cleared line, moving upwards
  for(int y = cleared_line; y >= 0; y--){
    for(int x = 8; x >= 1; x--){
      grid[x][y] = grid[x][y-1];
    }
  }

  // Set top of display to zero
  for(int x = 8; x >= 1; x--){
    grid[x][0] = 0;
  }
}




// Adds the current block to the grid (called if bottom collision occurs)
void add_block_to_grid(){
  for (int x = 0; x < 4; x++){
    for (int y = 0; y < 4; y++){
        grid[cur_piece.x + x][cur_piece.y + y] |= cur_piece.block[x][y];
    }
  }
}

// ************************** HELPER FUNCTION FOR SPI ON LED DISPLAY **************************

//Function provided from https://github.com/wayoda/LedControl/tree/master
void printByte(byte character []) {

int i = 0;
  for(i=0;i<8;i++)
  {
    lc.setRow(0,i,character[i]);
  }
}


// define two tasks for Blink & AnalogRead
void TaskBlink( void *pvParameters );
void TaskAnalogRead( void *pvParameters );

void TaskGravity( void *pvParameters );
void TaskInitBoard( void *pvParameters );
void TaskShowBoard( void *pvParameters );
void buttonsTask(void *pvParameters);

// the setup function runs once when you press reset or power the board
void setup() {

  // Button setup
  pinMode(leftButtonPin, INPUT);
  pinMode(rightButtonPin, INPUT);

  pinMode(8, OUTPUT); // Setup pin for external LED

  randomSeed(29420);

  // Seven Segment Display (4-digit) setup
  byte numDigits = 4;
  byte digitPins[] = {50, 44, 42, 41};                    //Digit pins setup for easy connection order
  byte segmentPins[] = {48, 40, 45, 49, 51, 46, 43, 47};  //Segment pins setup for easy connection order
  bool resistorsOnSegments = false;
  byte hardwareConfig = COMMON_CATHODE;
  bool updateWithDelays = false;
  bool leadingZeros = false;
  bool disableDecPoint = true;

  sevseg.begin(hardwareConfig, numDigits, digitPins, segmentPins, resistorsOnSegments,
               updateWithDelays, leadingZeros, disableDecPoint);
  sevseg.setBrightness(90);

  // Seven Segment Display (1-digit) setup
  byte numDigits1 = 1;
  byte segmentPins1[] = {28, 30, 36, 34, 32, 26, 24, 38};  //Segment pins setup for easy connection order
  bool resistorsOnSegments1 = true;
  byte hardwareConfig1 = COMMON_CATHODE;
  bool updateWithDelays1 = false;
  bool leadingZeros1 = false;
  bool disableDecPoint1 = true;

  sevseg_single.begin(hardwareConfig1, numDigits1, 1, segmentPins1, resistorsOnSegments1,
               updateWithDelays1, leadingZeros1, disableDecPoint1);
  sevseg_single.setBrightness(90);

  // LED matrix setup
  lc.shutdown(0, false);
  lc.setIntensity(0,3);
  lc.clearDisplay(0);

  //ADD INITIALIZATION FOR BOARD

  // initialize serial communication at 9600 bits per second:
  Serial.begin(19200);
  
  difficulty = 1;

  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB, on LEONARDO, MICRO, YUN, and other 32u4 based boards.
  } 

  xTaskCreate(
    TaskBlink
    , "Blink LED"
    , 128
    , NULL
    , 2
    , NULL );
  xTaskCreate(
    TaskGravity
    ,  "Gravity"   // A name just for humans
    ,  256  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  2  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  &gravity_handle );

  xTaskCreate(
    TaskInitBoard
    ,  "Initialize Board"
    ,  256  // Stack size
    ,  NULL
    ,  3  // Priority
    ,  &init_handle );

  xTaskCreate(
    TaskInitPiece
    , "Initialize Piece"
    , 128
    , NULL
    , 3
    , &piece_init_handle );

  xTaskCreate(
    TaskShowBoard
    ,  "Show Board"
    ,  256  // Stack size
    ,  NULL
    ,  2  // Priority
    ,  &show_handle );
  xTaskCreate(
    keypadTask
    , "Keypad Task"
    , 256
    , NULL
    , 1
    , NULL );
  xTaskCreate(
    buttonsTask
    , "Buttons Task"
    , 256
    , NULL
    , 1
    , NULL );
  xTaskCreate(
    TaskRefresh
    , "Refresh Display"
    , 128
    , NULL
    , 1
    , NULL );

  //vTaskSuspend(piece_init_handle);
  //vTaskSuspend(gravity_handle);
  // Now the task scheduler, which takes over control of scheduling individual tasks, is automatically started.
  //  (note how the above comment is WRONG!!!)
  vTaskStartScheduler();


}

void loop()
{
  // Empty. Things are done in Tasks.
}

// ******************** KEYPAD TASK START ********************
void keypadTask(void *pvParameters) {
  (void) pvParameters;

  for (;;) {
    char key = keypad.getKey();

    if (key) {
      if(key == '4') {
        cur_piece = move_left();
      }
      else if (key == '6') {
        cur_piece = move_right();
      }
      else if (key == '2') { // FIX PINOUT TO GO TO 8 INSTEAD
        struct piece next_piece = move_down();

        if(next_piece.y == cur_piece.y){
          add_block_to_grid();
          line_clear_check();
          cur_piece = get_new_piece();
        } else{
          cur_piece = next_piece;
        }
      }
    }
    vTaskDelay(101 / portTICK_PERIOD_MS);
  }
}
// ******************** KEYPAD TASK STOP ********************

// ******************** BUTTON TASK START ********************
void buttonsTask(void *pvParameters) {
  (void) pvParameters;

  for (;;) {
    leftButtonState = digitalRead(leftButtonPin);
    rightButtonState = digitalRead(rightButtonPin);

    if (leftButtonState == HIGH) {
      cur_piece = rotate_left();
    }

    else if (rightButtonState == HIGH) {
      cur_piece = rotate_right();
    }

    vTaskDelay(171 / portTICK_PERIOD_MS);
  }
}
// ******************** BUTTON TASK STOP ********************

// ******************** SEVEN SEGMENT DISPLAY TASK START ********************
void TaskRefresh(void *pvParameters) {
  (void) pvParameters;

  for (;;) {
    sevseg.refreshDisplay();
    sevseg_single.refreshDisplay();
    vTaskDelay(2 / portTICK_PERIOD_MS);
  }
}
// ******************** SEVEN SEGMENT DISPLAY TASK STOP ********************
// Initialize the board with the bottom, left, and right edges as ones (creates a bounding collision box)
void TaskInitBoard ( void *pvParameters ){

  for(;;){
      //TaskHandle_t TaskHandle_1;
    for(int x = 0; x <= 9; x++) {
      for(int y = 0; y <= 8; y++) {

        // If the current position is along the edge (not top edge y = 0), set to 1. Zero otherwise
        if((x == 0) || (x == 9) || (y == 8)) {
          grid[x][y] = 1;
        }
        else {
          grid[x][y] = 0;
        }
      }
    } 
    vTaskSuspend(NULL);
  }
  
}

void TaskInitPiece (void *pvParameters){
  for (;;){
    cur_piece = get_new_piece();
    vTaskSuspend(NULL);
  }
}

void TaskGravity( void *pvParameters ){

  for(;;) {
    struct piece next_piece = move_down();

    if(next_piece.y == cur_piece.y){
      add_block_to_grid();
      line_clear_check();
      cur_piece = get_new_piece();
    } else{
      cur_piece = next_piece;
    }
    vTaskDelay( (1031 - 200*difficulty) / portTICK_PERIOD_MS );
  }
}

void TaskShowBoard( void *pvParameters ){
  
  for(;;){
    
    PORTB |= DIN_MASK | CS_MASK | CLK_MASK;
    byte screen[8];

    for (int y = 0; y < 8; y++){
        screen[y] = 0;
    }

    // Add the static blocks to the screen
    for (int x = 0; x < 8; x++){
      for (int y = 0; y <8; y++){
        screen[x] |= (grid[x + 1][y] << y);
      }
    }

    // Add the moving block to the screen
    for (int x = 0; x < 4; x++){
      for (int y = 0; y < 4; y++){
        screen[cur_piece.x + x - 1] |= (cur_piece.block[x][y] << cur_piece.y + y);
      }
    }

    printByte(screen);
    vTaskDelay( 51 / portTICK_PERIOD_MS );
  }
}

void TaskBlink( void *pvParameters ){
  
  for(;;){
    digitalWrite(8, HIGH);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    digitalWrite(8, LOW);
    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}
