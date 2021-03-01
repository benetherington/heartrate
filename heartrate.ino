#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include "arduino-timer.h"
#include "heartrate.h"

// IO
#define HR_RX     12
#define LED       13
#define BUTTON_A  9
#define BUTTON_B  6
#define BUTTON_C  5
Adafruit_SH110X display = Adafruit_SH110X(64, 128, &Wire);

// TIMERS
Timer<2>               timer;             // flash_heart, update_display.
Timer<2, millis, int>  led_timer;         // blink_led, led_off. Takes delay.
Timer<1, millis, bool> heart_pulse_timer; // display_heart_interior. Takes filled.

// CONFIGURABLE
#define FASTBLINK         100  // Blinkenlights for fun and debugging
#define SLOWBLINK         1000 //
#define HR_SAMPLE_RATE    2500 // Milliseconds to wait before displaying a new averaged HR
#define DISP_SPARK_HR_MIN 75   // Sparkline Y min in BPM
#define DISP_SPARK_HR_MAX 180  // Sparkline Y max in BPM

// GUI ARRANGEMENT
#define DISP_HEART_X 95 // Pulsing heart
#define DISP_HEART_Y 0  //
#define DISP_RATE_X  94 // Heartrate numerical display
#define DISP_RATE_Y  40 //
#define DISP_GRAPH_X 0  // Sparkline
#define DISP_GRAPH_Y 0  //
#define DISP_GRAPH_W 93 //
#define DISP_GRAPH_H 64 //


// GLOBALS
byte oldSample, sample;
bool newHeartbeat = false;
unsigned long heartrateSinceMillis = millis();
int heartbeatsSince = 0;



/* -------------- *\
|                  |
|  SETUP AND LOOP  |
|                  |
\* -------------- */

void setup() {
  // Change the true/false below to hotswap to button A as an alternate heartrate input.
  // This causes some odd behaviors due to the swapped polarity, but it's good enough for debugging.
  int debug_input;
  if (false) {
    debug_input = BUTTON_A;
  } else {
    debug_input = HR_RX;
  }

  // Set up pins, display, timers, callbacks
  Serial.begin(9600);
  Serial.println("INIT PINS");
  pinMode (HR_RX, INPUT);
  pinMode(LED, OUTPUT);
  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);

  Serial.println("INIT DISPLAY");
  display.begin(0x3C, true);
  display.clearDisplay(); // Love the Adafruit splash screen, but it's upside down for this setup!
  display.setRotation(3);
  init_display();
  display.display();

  Serial.println("INIT TIMERS");
  timer.every(100, flash_heart);
  timer.every(HR_SAMPLE_RATE, update_display);

  Serial.println("INIT INTERRUPTS");
  attachInterrupt(
    digitalPinToInterrupt(debug_input),
    []{newHeartbeat = true; heartbeatsSince += 1;},
    FALLING
  );

  Serial.println("Waiting for heart beat...");
  // led_timer.every(1000, blink_led, FASTBLINK);
  while (!digitalRead(debug_input)) {};
  Serial.println ("Heart beat detected!");
  
}
void loop() {
  // led_timer.tick();
  heart_pulse_timer.tick();
  timer.tick();
}

/* ------------- *\
|                 |
|  BLINKENLIGHTS  |
|                 |
\* ------------- */

bool blink_led(int duration) {
  Serial.println(duration);
  digitalWrite(13, HIGH);
  led_timer.in(duration, led_off, duration);
  return true;
}
bool led_off(int duration) {
  digitalWrite(13, LOW);
  return true;
}



/* ---------------------- *\
|                          |
|  HEARTRATE CALCULATIONS  |
|                          |
\* --------------------- */

template <uint8_t K, class uint_t = uint16_t>
class EMA {
  // Exponential moving average. Allows us to get a nice, smooth heartrate to display.
  // https://tttapa.github.io/Pages/Mathematics/Systems-and-Control-Theory/Digital-filters/Exponential%20Moving%20Average/C++Implementation.html#arduino-example
  public:
    /// Update the filter with the given input and return the filtered output.
    uint_t operator()(uint_t input) {
        state += input;
        uint_t output = (state + half) >> K;
        state -= output;
        return output;
    }

    static_assert(
        uint_t(0) < uint_t(-1),  // Check that `uint_t` is an unsigned type
        "The `uint_t` type should be an unsigned integer, otherwise, "
        "the division using bit shifts is invalid.");

    /// Fixed point representation of one half, used for rounding.
    constexpr static uint_t half = 1 << (K - 1);

  private:
    uint_t state = 0;
};
int averaged_heartrate() {
  static EMA<2> filter;
  // calculate instant heartrate
  float deltaMillis = millis() - heartrateSinceMillis;
  float instantHeartrate = heartbeatsSince / deltaMillis // beats per milli
                           * 1000                        // beats per second
                           * 60;                         // beats per minute
  // reset counters
  heartrateSinceMillis = millis(); heartbeatsSince = 0;
  // return average
  return filter(instantHeartrate);
}


/* -------- *\
|            |
|  GRAPHICS  |
|            |
\* -------- */

void init_display() {
  // heart outline
  display.drawBitmap(DISP_HEART_X, DISP_HEART_Y, heartNoInterior, 33, 32, SH110X_WHITE);
  // graph area
  display.drawRoundRect(
    DISP_GRAPH_X,
    DISP_GRAPH_Y,
    DISP_GRAPH_W,
    DISP_GRAPH_H,
    5,
    SH110X_WHITE
  );
  display.setTextColor(SH110X_WHITE, SH110X_BLACK);
  display.setTextSize(2);
  display.setTextWrap(false);
}

bool update_display(void*) {
  int average_hr = averaged_heartrate();
  update_text(average_hr);
  update_graph(average_hr);
  return true;
}

void update_text(int heartrate) {
  display.setCursor(DISP_RATE_X, DISP_RATE_Y);
  display.print(heartrate); display.print("  ");
}

#define DISP_SPARK_MIN    DISP_GRAPH_X+2
#define DISP_SPARK_MAX    DISP_SPARK_MIN+DISP_GRAPH_W-4
int graph_cursor_position = DISP_SPARK_MIN;
void update_graph(int heartrate) {
  // Calculate scaled Y position
  int bounded_hr = min(max(heartrate, DISP_SPARK_HR_MIN), DISP_SPARK_HR_MAX);
  float excess_hr = bounded_hr-DISP_SPARK_HR_MIN;
  static float display_ratio =
    (float)(DISP_SPARK_HR_MAX-DISP_SPARK_HR_MIN)
    /
    (float)(DISP_GRAPH_Y+DISP_GRAPH_H-4);
  static int bottom_of_graph = (DISP_GRAPH_Y+DISP_GRAPH_H-4);
  float heartrate_pixel_y = bottom_of_graph-excess_hr*display_ratio;

  // Set cursor
  graph_cursor_position += 1;
  if (graph_cursor_position > DISP_SPARK_MAX) {
    graph_cursor_position = DISP_SPARK_MIN;
  }
  // Clear columns
  display.drawFastVLine(
    graph_cursor_position, DISP_GRAPH_Y+3,
    DISP_GRAPH_H-6, SH110X_BLACK
  );
  if (graph_cursor_position > DISP_SPARK_MAX) {
    display.drawFastVLine(
      graph_cursor_position+1, DISP_GRAPH_Y+3,
      DISP_GRAPH_H-6, SH110X_BLACK
    );
  }
  // Draw pixel
  display.drawPixel(
    graph_cursor_position,
    heartrate_pixel_y,
    SH110X_WHITE
  );
}

bool flash_heart(void*) {
  if (newHeartbeat) {
    display_heart_interior(true);
    heart_pulse_timer.in(100, display_heart_interior, false);
    newHeartbeat = false;
  }
  return true;
}

bool display_heart_interior(bool filled) {
  // display.clearDisplay();
  if (filled) {
    display.drawBitmap(DISP_HEART_X, DISP_HEART_Y, heartInterior, 33, 32, SH110X_WHITE);
  } else {
    display.drawBitmap(DISP_HEART_X, DISP_HEART_Y, heartInterior, 33, 32, SH110X_BLACK);
  }
  display.display();
  return true;
}

