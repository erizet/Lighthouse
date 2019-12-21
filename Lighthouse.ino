/*
    Basic Pin setup:
    ------------                                  ---u----
    ARDUINO   13|-> SCLK (pin 25)           OUT1 |1     28| OUT channel 0
              12|                           OUT2 |2     27|-> GND (VPRG)
              11|-> SIN (pin 26)            OUT3 |3     26|-> SIN (pin 11)
              10|-> BLANK (pin 23)          OUT4 |4     25|-> SCLK (pin 13)
               9|-> XLAT (pin 24)             .  |5     24|-> XLAT (pin 9)
               8|                             .  |6     23|-> BLANK (pin 10)
               7|                             .  |7     22|-> GND
               6|                             .  |8     21|-> VCC (+5V)
               5|                             .  |9     20|-> 2K Resistor -> GND
               4|                             .  |10    19|-> +5V (DCPRG)
               3|-> GSCLK (pin 18)            .  |11    18|-> GSCLK (pin 3)
               2|                             .  |12    17|-> SOUT
               1|                             .  |13    16|-> XERR
               0|                           OUT14|14    15| OUT channel 15
    ------------                                  --------

    -  Put the longer leg (anode) of the LEDs in the +5V and the shorter leg
         (cathode) in OUT(0-15).
    -  +5V from Arduino -> TLC pin 21 and 19     (VCC and DCPRG)
    -  GND from Arduino -> TLC pin 22 and 27     (GND and VPRG)
    -  digital 3        -> TLC pin 18            (GSCLK)
    -  digital 9        -> TLC pin 24            (XLAT)
    -  digital 10       -> TLC pin 23            (BLANK)
    -  digital 11       -> TLC pin 26            (SIN)
    -  digital 13       -> TLC pin 25            (SCLK)
    -  The 2K resistor between TLC pin 20 and GND will let ~20mA through each
       LED.  To be precise, it's I = 39.06 / R (in ohms).  This doesn't depend
       on the LED driving voltage.
    - (Optional): put a pull-up resistor (~10k) between +5V and BLANK so that
                  all the LEDs will turn off when the Arduino is reset.

    If you are daisy-chaining more than one TLC, connect the SOUT of the first
    TLC to the SIN of the next.  All the other pins should just be connected
    together:
        BLANK on Arduino -> BLANK of TLC1 -> BLANK of TLC2 -> ...
        XLAT on Arduino  -> XLAT of TLC1  -> XLAT of TLC2  -> ...
    The one exception is that each TLC needs it's own resistor between pin 20
    and GND.

    This library uses the PWM output ability of digital pins 3, 9, 10, and 11.
    Do not use analogWrite(...) on these pins.

    This sketch does the Knight Rider strobe across a line of LEDs.

    Alex Leone <acleone ~AT~ gmail.com>, 2009-02-03 */

#include "SparkFun_Tlc5940.h"
#include <StopWatch.h>
#include <avdweb_Switch.h>

enum States 
{
  Init,
  Waiting,
  Shining  
};

States currentState = Init;
const int FIRST_LIGHT_TLC_POSITON = 4;  // index to first light in TLC
const int NUM_LIGHTS = 8;
const int LAST_LIGHT_TLC_POSITON = FIRST_LIGHT_TLC_POSITON + NUM_LIGHTS - 1;  // index to last light in TLC

const int SHORT_INTERVAL = 30;
const int LONG_INTERVAL = 5 * 60;
bool useShortInterval = true;
int interval = SHORT_INTERVAL;

FrameRateCounter fps(12);
StopWatch stopwatch;
Switch button(A1); // = new Switch(A1);

const int ANIMATION_MAX_VALUE = 4095;
const int ANIMATION_FRAMES = 5;
const int ANIMATION_LENGTH = 4;
int lightAnimation[ANIMATION_FRAMES][ANIMATION_LENGTH] = {
    { 0.2 * ANIMATION_MAX_VALUE, 1.0 * ANIMATION_MAX_VALUE, 0.2 * ANIMATION_MAX_VALUE, 0.0 * ANIMATION_MAX_VALUE },
    { 0.2 * ANIMATION_MAX_VALUE, 0.9 * ANIMATION_MAX_VALUE, 0.3 * ANIMATION_MAX_VALUE, 0.0 * ANIMATION_MAX_VALUE },
    { 0.1 * ANIMATION_MAX_VALUE, 0.8 * ANIMATION_MAX_VALUE, 0.7 * ANIMATION_MAX_VALUE, 0.1 * ANIMATION_MAX_VALUE },
    { 0.1 * ANIMATION_MAX_VALUE, 0.2 * ANIMATION_MAX_VALUE, 0.8 * ANIMATION_MAX_VALUE, 0.1 * ANIMATION_MAX_VALUE },
    { 0.0 * ANIMATION_MAX_VALUE, 0.2 * ANIMATION_MAX_VALUE, 1.0 * ANIMATION_MAX_VALUE, 0.1 * ANIMATION_MAX_VALUE },
};

void AnimationFrameToTLC(int animationFrame, int startChannel)
{
    Tlc.clear();

    for(int animPos=0; animPos < ANIMATION_LENGTH; animPos++)
    {
      int value = lightAnimation[animationFrame][animPos];
      int channel = startChannel + animPos;
      if(channel > LAST_LIGHT_TLC_POSITON)
        channel = channel - NUM_LIGHTS;

      Tlc.set(channel, value);
      Serial.print("Kanal: ");
      Serial.print(channel);
      Serial.print(", värde: ");
      Serial.println(value);
    }

    Tlc.update();
}

void RotateLights()
{
    static int startChannel = FIRST_LIGHT_TLC_POSITON;
    static int animationFrame = 0;

    AnimationFrameToTLC(animationFrame, startChannel);

    animationFrame++;
    if(animationFrame>=ANIMATION_FRAMES)
    {
      animationFrame = 0;
    
      startChannel++;

      if(startChannel > LAST_LIGHT_TLC_POSITON)
        startChannel = FIRST_LIGHT_TLC_POSITON; 
    }
}

bool Shine()
{
  //Serial.print("Shine: ");
  //Serial.println(fps.isRunning());

  fps.update();

  if (fps.isRunning())
  {
      if (fps.isNext())
      {
          Serial.print("frame no. = ");
          Serial.print(fps.frame());
          Serial.print(", time = ");
          Serial.println(fps.ms());

          RotateLights();
      }
  }

  return fps.count() < (NUM_LIGHTS * 10); // 10 varv
}

void ChangeInterval()
{
  useShortInterval = !useShortInterval;
  interval = useShortInterval ? SHORT_INTERVAL : LONG_INTERVAL;
}

void SetAllLights(bool on)
{
    int value = on ? 4095 : 0;
    Tlc.clear();

    for(int i = FIRST_LIGHT_TLC_POSITON; i < FIRST_LIGHT_TLC_POSITON + NUM_LIGHTS; i++)
    {
      Tlc.set(i, value);
    }

    Tlc.update();
}

void BlinkLights()
{
  SetAllLights(true);
  delay(200);
  SetAllLights(false);
  delay(300);
  SetAllLights(true);
  delay(200);
  SetAllLights(false);
}

void setup()
{
  Serial.begin(115200);
  /* Call Tlc.init() to setup the tlc.
     You can optionally pass an initial PWM value (0 - 4095) for all channels.*/
  Tlc.init();
}

/* This loop will create a Knight Rider-like effect if you have LEDs plugged
   into all the TLC outputs.  NUM_TLCS is defined in "tlc_config.h" in the
   library folder.  After editing tlc_config.h for your setup, delete the
   Tlc5940.o file to save the changes. */

void loop()
{
  //Serial.write("loop: ");
  button.poll();
  if(button.longPress())
  {
    Serial.println("byt intervall");
    ChangeInterval();
    BlinkLights();
    Serial.print("Nytt intervall: ");
    Serial.println(interval);
  }

  switch(currentState)
  {
    case Init:
      Serial.println("Init");
      stopwatch.restart();
      currentState = Waiting;
      break;
    case Waiting:
      Serial.print("Waiting: ");
      Serial.println(stopwatch.sec());

      if(stopwatch.sec() > interval || button.singleClick())
      {
        stopwatch.stop();
        fps.restart();
        currentState = Shining;
        Serial.println("Start Shining");
      }
      break;
    case Shining:
      //Serial.println("Shining");
      if(!Shine())
      {
        currentState = Init;
        Serial.println("Turn off all lights before INIT-state");
        SetAllLights(false);
      }
      break;
  }
}
