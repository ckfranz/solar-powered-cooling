/*
* This ESP32 code is created by esp32io.com
*
* This ESP32 code is released in the public domain
*
* For more detail (instruction and wiring diagram), visit https://esp32io.com/tutorials/esp32-light-sensor
*/

#include <AccelStepper.h>
#include <Wire.h>
#include <Adafruit_INA219.h>

// REMEMBER TO CHANGE PINS ACCORDINGLY :)
#define LIGHT_SENSOR_PIN_A 33 // ESP32 pin for first photoresistor
#define LIGHT_SENSOR_PIN_B 32 // ESP32 pin for second photoresistor

#define pin1 14
#define pin2 27
#define pin3 26
#define pin4 25

// INA SOLAR PANEL SETUP
Adafruit_INA219 ina219;

// Define pin
int base = 4;
// Reporting frequency
float freq = 2; // Hz
// Delay after changing state of transistor
int del = 2;
// Sensor variables
float current_mA = 0;
float voltage = 0;
float power_mW = 0;
// Tracking time
unsigned long last = 0;
unsigned long led_last = 0;
float t = 0;
int led = 2000;
bool state = false;

// STEPPER MOTOR SETUP
AccelStepper stepper(AccelStepper::FULL4WIRE, pin1, pin3, pin2, pin4);
int incomingAngle;
double degreesPerStep = 5.625 / 64;
// Average tracking
int left_sum = 0;
int right_sum = 0;
int average_left = 0;
int average_right = 0;
// Time tracking
int counter = 0;
int sample_length = 20;                // Number of 30 milliseconds used for average
int power_save_sample_length = 500;    // 15 seconds
// State variables
bool has_moved = false;
bool power_save = false;

void setup() {
    // REMEMBER TO CHANGE BAUD RATE OF SERIAL MONITOR TO 9600 :)
    Serial.begin(9600);

    // INA SOLAR PANEL
    pinMode(base, OUTPUT);
    pinMode(2, OUTPUT);
    while (!Serial) {
        Serial.println("Starting...");
        delay(1);
    }

    if (!ina219.begin()) {
        Serial.println("Failed to find INA219 chip");
        while (1) {
            delay(10);
        }
    }

    ina219.setCalibration_16V_400mA(); // Use 16V, 400mA range

    last = millis();
    led_last = millis();

    // STEPPER MOTOR
    stepper.setMaxSpeed(4096);
    stepper.setAcceleration(40);
}

void loop() {
    int right_photo_res = analogRead(LIGHT_SENSOR_PIN_A);
    int left_photo_res = analogRead(LIGHT_SENSOR_PIN_B);

    left_sum += left_photo_res;
    right_sum += right_photo_res;

    if (counter == sample_length && power_save == false) {
        average_left = left_sum / sample_length;
        average_right = right_sum / sample_length;

        if (average_left > average_right + 70) {
            stepper.moveTo(stepper.currentPosition() - (6 / degreesPerStep));
            has_moved = true;
        } else if (average_left > average_right + 35) {
            stepper.moveTo(stepper.currentPosition() - (3 / degreesPerStep));
            has_moved = true;
        } else if (average_right > average_left + 70) {
            stepper.moveTo(stepper.currentPosition() + (6 / degreesPerStep));
            has_moved = true;
        } else if (average_right > average_left + 35) {
            stepper.moveTo(stepper.currentPosition() + (3 / degreesPerStep));
            has_moved = true;
        } else {
            stepper.stop();
            if (has_moved == true) {
                power_save = true;
                stepper.disableOutputs();
            }
        }

        left_sum = 0;
        right_sum = 0;
        counter = 0;
    }

    if (power_save == false) {
        stepper.run();
    }

    if (millis() - last > 1000.0 / freq) {
        last = millis();
        t = last / 1000.0;

        digitalWrite(base, HIGH);
        delay(del);
        current_mA = ina219.getCurrent_mA();
        digitalWrite(base, LOW);
        delay(del);
        voltage = ina219.getBusVoltage_V();
        delay(del);

        power_mW = (0.7 * voltage) * current_mA;

        Serial.print(t);
        Serial.print(", ");
        Serial.print(voltage);
        Serial.print(", ");
        Serial.print(current_mA);
        Serial.print(", ");
        Serial.println(power_mW);
    }

    if (millis() - led_last > led) {
        state = !state;
        digitalWrite(2, state ? HIGH : LOW);
        led_last = millis();
    }

    if (counter == power_save_sample_length && power_save == true) {
        counter = 0;
        power_save = false;
        has_moved = false;
    }

    counter += 1;
    delay(30);
}
