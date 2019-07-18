#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif
#define PIN 13
#include <Wire.h>
#include "paj7620.h"

#define NUM_ROWS 5
#define NUM_COLS 8
#define NUM_PINS NUM_ROWS*NUM_COLS
#include <MsTimer2.h>
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_PINS, PIN, NEO_GRB + NEO_KHZ800);

#define GES_REACTION_TIME    800
#define GES_QUIT_TIME     1000

uint8_t current_cursor = 0;
uint8_t last_cursor = 0;
uint8_t data = 0;

bool LAST_PAINTABLE_FLAG = true;
bool PAINTABLE_FLAG = false;    //paintable mode => if Paintable flag is on => the led is not 

int index_color = 1;
float brightness = 0.5;
int mode = 1;

uint32_t COLORS[] = 
{
  strip.Color(0.5*255*brightness,0.5*255*brightness,0.5*255*brightness), 
  strip.Color(0.5*255*brightness,0,0), 
  strip.Color(0,0.5*255*brightness,0), 
  strip.Color(0,0,0.5*255*brightness), 
  strip.Color(0,0,0)
};
enum COLOR_TYPE {WHITE, RED, GREEN, BLUE, NONE };
byte it_COLORS = WHITE;   //iterator of COLOR

#define NUM_OF_COLORS (int)sizeof(COLORS)/sizeof(COLORS[0])



void setup()
{
  Serial.begin(9600);
  Serial.println("RGB_Lampe");
  
  strip.begin();
  for (int num = 0; num < NUM_PINS; num++)
    strip.setPixelColor(num, strip.Color(0.5 * 255, 0.5 * 255, 0.5 * 255));
  strip.show();

  uint8_t error = paj7620Init();      // initialize Paj7620 registers
  if (error)
  {
    Serial.print("INIT ERROR,CODE:");
    Serial.println(error);
  }
  else
    Serial.println("INIT OK");
    
  Serial.println("Please input your gestures:");
}

void loop()
{
  getMovementData();
  modeChange();
  modeControl();
}

// get movement instruction
void getMovementData()
{
  uint8_t error;
  if (! (error = paj7620ReadReg(0x43, 1, &data)) )
    instruction();
}

// mode change
void modeChange()
{
  bool MODE_CHANGED_FLAG = false;
  switch (data)
  {
  case GES_CLOCKWISE_FLAG: 
    MODE_CHANGED_FLAG = true;
    if (mode < 3) mode++;
    else          mode = 1;
    break;
  case GES_COUNT_CLOCKWISE_FLAG:
    MODE_CHANGED_FLAG = true;
    if (mode > 1) mode--;
    else          mode = 3;
    break;
  }

  if(MODE_CHANGED_FLAG)
  {
    MsTimer2::stop();
    LED_Off();

    last_cursor = current_cursor = 0;
    PAINTABLE_FLAG = false; 
    LAST_PAINTABLE_FLAG = true;
  }
}

// control in Mode
void modeControl()
{
  switch (mode)
  {
  case 1: //mode 1
    LED_lighting();
    break;
  case 2: //mode 2
    LED_wave();
    break;
  case 3: //mode 3
    if(PAINTABLE_FLAG) changeColor();
    else               moveCursor();
    painting();
    break;
  }
  updateBrightness();
}

void LED_Off()
{
  for (int num = 0; num < NUM_PINS; num++)
    strip.setPixelColor(num, COLORS[NONE]);
  strip.show();
}

void updateBrightness()
{
  COLORS[0] = strip.Color(255*brightness,255*brightness,255*brightness);
  COLORS[1] = strip.Color(255*brightness,0,0); 
  COLORS[2] = strip.Color(0,255*brightness,0);
  COLORS[3] = strip.Color(0,0,255*brightness);
  COLORS[4] = strip.Color(0,0,0);
}



void LED_lighting()
{
  controlLED();
  for (int num = 0; num < NUM_PINS; num++)
    strip.setPixelColor(num, COLORS[it_COLORS]);
  strip.show();
}



void LED_wave()
{
  controlLED();
  for (int i = 0; i < 40; i++)
  {
    strip.setPixelColor(i, strip.Color(brightness * 255, 0, 0));
    strip.show();
    delay(30);
  }
  for (int i = 0; i < 40; i++)
  {
    strip.setPixelColor(i, strip.Color(0, brightness * 255, 0));
    strip.show();
    delay(30);
  }
  for (int i = 0; i < 40; i++)
  {
    strip.setPixelColor(i, strip.Color(0, 0, brightness * 255));
    strip.show();
    delay(30);
  }
  for (int i = 0; i < 40; i++)
  {
    strip.setPixelColor(i, strip.Color(brightness * 255, brightness * 255, brightness * 255));
    strip.show();
    delay(30);
  }
}


void controlLED ()
{
  switch (data)
  {
  case GES_RIGHT_FLAG:
    if (it_COLORS < 3)  it_COLORS++;
    else                it_COLORS = 0;
    break;
  case GES_LEFT_FLAG:
    if (it_COLORS > 0)  it_COLORS--;
    else                it_COLORS = 3;
    break;
  case GES_UP_FLAG:
    if (brightness < 1) brightness += 0.25;
    else                brightness = 0.25;
    break;
  case GES_DOWN_FLAG:
    if (brightness > 0.25) brightness -= 0.25;
    else                   brightness = 1;
    break;
  }
}

void instruction()
{
  switch (data)                   // When different gestures be detected, the variable 'data' will be set to different values by paj7620ReadReg(0x43, 1, &data).
  {
    case GES_RIGHT_FLAG:
      delay(GES_REACTION_TIME);
      paj7620ReadReg(0x43, 1, &data); // to avoid double movement, read the instruction once again.
      if(data == GES_LEFT_FLAG)       // in the case of Left after Right => not Right-Left but just Left
      {
        Serial.println("Left");
        data = GES_LEFT_FLAG;
      }
      else
      {
        Serial.println("Right");
        data = GES_RIGHT_FLAG;
      }          
      break;
    case GES_LEFT_FLAG:
      delay(GES_REACTION_TIME);
      paj7620ReadReg(0x43, 1, &data);
      if(data == GES_RIGHT_FLAG) 
      {
        Serial.println("Right");
        data = GES_RIGHT_FLAG;
      }
      else
      {
        Serial.println("Left");
        data = GES_LEFT_FLAG;
      }          
      break;
    case GES_UP_FLAG:
      delay(GES_REACTION_TIME);
      paj7620ReadReg(0x43, 1, &data);
      if(data == GES_DOWN_FLAG) 
      {
        Serial.println("Down");
        data = GES_DOWN_FLAG;
      }
      else
      {
        Serial.println("Up");
        data = GES_UP_FLAG;
      }
      break;
    case GES_DOWN_FLAG:
      delay(GES_REACTION_TIME);
      paj7620ReadReg(0x43, 1, &data);
      if(data == GES_UP_FLAG) 
      {
        Serial.println("Up");
        data = GES_UP_FLAG;
      }
      else
      {
        Serial.println("Down");
        data = GES_DOWN_FLAG;
      }
      break;
    case GES_FORWARD_FLAG:
      delay(GES_REACTION_TIME);
      paj7620ReadReg(0x43, 1, &data);
      if(data == GES_BACKWARD_FLAG) 
      {
        Serial.println("Backward");
        data = GES_BACKWARD_FLAG;
      }
      else
      {
        Serial.println("Forward");
        data = GES_FORWARD_FLAG;
      }
      break;
    case GES_BACKWARD_FLAG:     
      delay(GES_REACTION_TIME);
      paj7620ReadReg(0x43, 1, &data);
      if(data == GES_FORWARD_FLAG) 
      {
        Serial.println("Forward");
        data = GES_FORWARD_FLAG;
      }
      else
      {
        Serial.println("Backward");
        data = GES_BACKWARD_FLAG;
      }
      break;
    case GES_CLOCKWISE_FLAG:
      Serial.println("Clockwise");
      break;
    case GES_COUNT_CLOCKWISE_FLAG:
      Serial.println("Counter-Clockwise");
      break;
  }
}

// set current position of cursor (pixel number)
void moveCursor()
{
  int8_t temp_cursor = current_cursor;

  switch(data)
  {
  case GES_RIGHT_FLAG:
    temp_cursor++;
    if(temp_cursor<strip.numPixels())
      current_cursor = temp_cursor;
    break;
  case GES_LEFT_FLAG:
    temp_cursor--;
    if(temp_cursor>=0)
      current_cursor = temp_cursor;
    break;
  case GES_UP_FLAG:
    temp_cursor-=NUM_COLS;
    if(temp_cursor>=0)
      current_cursor = temp_cursor;
    break;
  case GES_DOWN_FLAG:
    temp_cursor+=NUM_COLS;
    if(temp_cursor<strip.numPixels())
      current_cursor = temp_cursor;
    break;
    
  case GES_FORWARD_FLAG:
    PAINTABLE_FLAG = true;
    break;
  }
}

void changeColor()
{
  controlLED();

  if(data == GES_BACKWARD_FLAG)
    PAINTABLE_FLAG = false;
}

void blinkLED() {
  static bool toggle;
  if(toggle)
    strip.setPixelColor(current_cursor, strip.Color(0,0,0) );
  else
    strip.setPixelColor(current_cursor, COLORS[it_COLORS] );
  strip.show();
  toggle = !toggle;
}

void painting()
{ 
  if( LAST_PAINTABLE_FLAG && !PAINTABLE_FLAG )  //falling edge of PAINTABLE_FLAG (only runs at the start of MOVABLE mode)
  {
      MsTimer2::set(250, blinkLED); // 500ms period blink
      MsTimer2::start(); 
  }else if(!LAST_PAINTABLE_FLAG && PAINTABLE_FLAG){ //rising edge of PAINTABLE_FLAG (only runs at the end of MOVABLE mode)
      MsTimer2::stop();
  }

  if(PAINTABLE_FLAG)
     strip.setPixelColor(current_cursor, COLORS[it_COLORS] );
  else
     if(last_cursor != current_cursor)    //only when the position is changed, set color
        strip.setPixelColor(last_cursor, COLORS[it_COLORS] );
        
  strip.show();
  
  LAST_PAINTABLE_FLAG = PAINTABLE_FLAG;
  last_cursor = current_cursor;
}






