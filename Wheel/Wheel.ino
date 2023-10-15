#include <LiquidCrystal.h>

// Constants
const int LOW_THRESHOLD = 1500;       // Voltage readings from ADC below this value mean that the sensor is facing black tape
const int HIGH_THRESHOLD = 2000;      // Voltage readings from ADC above this value mean that the sensor is facing the rest of the wheel
const double WHEEL_DIAMETER = 8.125;  // Wheel diameter in inches
const double WHEEL_CIRCUMFERENCE = (WHEEL_DIAMETER / 12) * PI; // Wheel circumference in feet

// Pins
const int rs = 37, en = 35, d4 = 9, d5 = 10, d6 = 11, d7 = 12; // These pins are used for the LCD display
const int PIN_IR_VOLTAGE = 5; // The pin that reads the voltage from the infrared phototransistor
const int PIN_BUZZER = 15;    // The pin that outputs to the active buzzer for debugging

// Conversion constants
const short unsigned int SECONDS_PER_HOUR = 3600;
const double FEET_TO_MILES = 1.0 / 5280;
const double FEET_TO_KILOMETERS = (12 * 2.54) / (100 * 1000);
const double FEET_TO_METERS = (12 * 2.54) / 100;

// This enum represents the different display modes of the Gerbil Speedometer
enum class DisplayMode { Feet, Miles, Kilometers, Meters };

// Function prototypes of display method and interrupt function
void displaySpeedAndSum(DisplayMode current_display_mode, double current_speed_feet, double total_distance_feet);
void buttonPressed();

LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
unsigned long last_rotation_time = 0; // last_rotation_time either stores the time of the last rotation, or 0, which signifies that this is the first rotation
double current_speed_feet = 0;        // Distance and speed are stored as feet and converted to other units
double total_distance_feet = 0;

// These two variables are volatile because they are modified in the interrupt function
volatile DisplayMode current_display_mode = DisplayMode::Feet; // The current units that are displayed on the LCD
volatile unsigned long last_button_time = 0;                   // For debouncing the button

void setup() {                
   // Set pin modes and attach interrupt for button
   pinMode(PIN_IR_VOLTAGE, INPUT);
   pinMode(PIN_BUZZER, OUTPUT);
   attachInterrupt(digitalPinToInterrupt(7), buttonPressed, RISING);

   // Setup LCD display
   lcd.begin(16, 2);
}

void loop() {
  int voltage_average = 0;
  
  // Keep reading voltages until the voltage is below LOW_THRESHOLD, which means that black tape has been detected
  do
  {
    // Take the average of 10 readings for accuracy
    int sum_readings = 0;
    int num_readings = 0;
    for (int i = 0; i < 10; i++)
    {
      num_readings++;
      sum_readings += analogRead(PIN_IR_VOLTAGE);
    }
    voltage_average = sum_readings / num_readings;
  } while (voltage_average > LOW_THRESHOLD);

  // For testing purposes, the buzzer is activated when black tape is seen
  digitalWrite(PIN_BUZZER, HIGH);
  
  // Keep reading voltages until the voltage is above HIGH_THRESHOLD, which means that black tape is no longer being detected
  // The difference between HIGH_THRESHOLD and LOW_THRESHOLD causes hysteresis, which means that there is a "deadzone" between
  // a voltage reading considered low and a voltage reading considered high. This prevents the microcontroller from detecting
  // false transitions from dark to light that would happen because the voltage fluctuates slightly in a transition.
  do
  {
    // Take the average of 10 readings for accuracy
    int sum_readings = 0;
    int num_readings = 0;
    for (int i = 0; i < 10; i++)
    {
      num_readings++;
      sum_readings += analogRead(PIN_IR_VOLTAGE);
    }
    voltage_average = sum_readings / num_readings;
  } while (voltage_average < HIGH_THRESHOLD);

  // Turn off buzzer because the black tape is no longer detected
  digitalWrite(PIN_BUZZER, LOW);

  // Now a full transition from dark to light has been detected
  // If last_rotation_time is 0, that signifies that this is the first rotation, and we save the time of this rotation so that we can see how much
  // Time ellapses between this rotation and the next one
  // else last_rotation_time equals the time of the last rotation, so calculate speed by comparing the current time and the time at last rotation
  if (last_rotation_time == 0)
  {
    last_rotation_time = millis();
  }
  else
  {
    // Get time difference and convert to seconds
    unsigned long current_time = millis();
    double seconds = (current_time - last_rotation_time) / 1000.0;

    // Calculate speed and increment total distance
    current_speed_feet = WHEEL_CIRCUMFERENCE / seconds; 
    total_distance_feet += WHEEL_CIRCUMFERENCE;

    // Update display
    lcd.clear();
    displaySpeedAndSum(current_display_mode, current_speed_feet, total_distance_feet);

    // Set last_rotation_time to 0 to signify that the next rotation will be the first rotation of each set of two rotations
    last_rotation_time = 0;
  }
}

// This function displays the speed and the total distance traveled on the LCD display. It displays a speed of current_speed_feet and a total
// distance of total_distance_feet in the units of current_display_mode
void displaySpeedAndSum(DisplayMode current_display_mode, double current_speed_feet, double total_distance_feet)
{
  // Default to displaying feet
  double current_speed_current_unit = current_speed_feet;
  double total_distance_current_unit = total_distance_feet;
  String unit_name = "ft/sec"; // What is displayed after the actual speed number
  
  // Detect if other display mode is being used, in which case convert to the specified units
  switch (current_display_mode)
  {
    case DisplayMode::Feet: break; // No change needed
    case DisplayMode::Miles:
      unit_name = "mi/hr";
      // Convert feet per second to miles per hour
      current_speed_current_unit = (current_speed_feet * FEET_TO_MILES) * SECONDS_PER_HOUR;

      // Convert feet to miles
      total_distance_current_unit = total_distance_feet * FEET_TO_MILES;
      break;
    case DisplayMode::Kilometers:
      unit_name = "km/hr";
      // Convert feet per second to kilometers per hour
      current_speed_current_unit = (current_speed_feet * FEET_TO_KILOMETERS) * SECONDS_PER_HOUR;

      // Convert feet to kilometers
      total_distance_current_unit = total_distance_feet * FEET_TO_KILOMETERS;
      break;
    case DisplayMode::Meters:
      unit_name = "m/sec";
      // Convert feet per second to meters per second
      current_speed_current_unit = current_speed_feet * FEET_TO_METERS;

      // Convert feet to meters
      total_distance_current_unit = total_distance_feet * FEET_TO_METERS;
      break;
  }

  // Now we have correct units, so display the speed and distance
  lcd.setCursor(0, 0);
  // Print speed on top line
  lcd.print(current_speed_current_unit);
  // Print unit_name right-aligned
  lcd.setCursor(16 - unit_name.length(), 0);
  lcd.print(unit_name);
  // Print total distance on botton row
  lcd.setCursor(0, 1);
  lcd.print(total_distance_current_unit);
  lcd.setCursor(16 - 5, 1); // 5 is the number of letters in "total"
  lcd.print("total");
}

// The interrupt function called when the button is pressed. This function cycles the display mode
void buttonPressed() {
  // Only register button click if it has been 300 milliseconds to debounce
  if (millis() - last_button_time < 300)
  {
    return;
  }
  last_button_time = millis();

  // Increment display mode based on current display mode
  switch (current_display_mode)
  {
    case DisplayMode::Feet: current_display_mode = DisplayMode::Miles; break;
    case DisplayMode::Miles: current_display_mode = DisplayMode::Kilometers; break;
    case DisplayMode::Kilometers: current_display_mode = DisplayMode::Meters; break;
    case DisplayMode::Meters: current_display_mode = DisplayMode::Feet; break;
  }
  // Clear display and re-display results with new units
  lcd.clear();
  displaySpeedAndSum(current_display_mode, current_speed_feet, total_distance_feet);
}
