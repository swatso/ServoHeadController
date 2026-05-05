#include <Arduino.h>
#include <Servo.h>

// ---------------------------------------------------------------------------
// Pin assignments
// ---------------------------------------------------------------------------
#define SERVO1_PIN        9   // PWM output – servo 1 signal
#define SERVO2_PIN        10  // PWM output – servo 2 signal
#define DEMAND_INPUT_PIN  2   // Digital input  – On/Off demand (active low)
#define SERVO_POWER_PIN   4   // Digital output – servo power enable (active low)

// ---------------------------------------------------------------------------
// Servo positions (degrees)
// ---------------------------------------------------------------------------
#define SERVO1_OFF_POSITION   0
#define SERVO1_ON_POSITION   90
#define SERVO2_OFF_POSITION 180
#define SERVO2_ON_POSITION   90

// ---------------------------------------------------------------------------
// Smooth-movement parameters
// ---------------------------------------------------------------------------
// Total travel time in milliseconds
#define MOVE_DURATION_MS  5000
// Time between individual position steps (ms) – smaller = smoother
#define STEP_INTERVAL_MS    20

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------
Servo servo1;
Servo servo2;

float currentPos1 = SERVO1_OFF_POSITION;
float currentPos2 = SERVO2_OFF_POSITION;
bool  isOn        = false;

// ---------------------------------------------------------------------------
// moveServosSmooth
// Linearly interpolates both servos from their current positions to the
// supplied target positions over MOVE_DURATION_MS milliseconds.
// ---------------------------------------------------------------------------
void moveServosSmooth(int target1, int target2)
{
    float start1 = currentPos1;
    float start2 = currentPos2;

    int steps = MOVE_DURATION_MS / STEP_INTERVAL_MS;

    for (int i = 1; i <= steps; i++)
    {
        float t    = (float)i / (float)steps;
        int   pos1 = (int)(start1 + t * ((float)target1 - start1));
        int   pos2 = (int)(start2 + t * ((float)target2 - start2));

        servo1.write(pos1);
        servo2.write(pos2);

        delay(STEP_INTERVAL_MS);
    }

    currentPos1 = (float)target1;
    currentPos2 = (float)target2;
}

// ---------------------------------------------------------------------------
// setup
// ---------------------------------------------------------------------------
void setup()
{
    // Configure demand input with internal pull-up (signal is active low)
    pinMode(DEMAND_INPUT_PIN, INPUT_PULLUP);

    // Servo power output – keep power OFF during initialisation
    // Active-low: HIGH = power off, LOW = power on
    pinMode(SERVO_POWER_PIN, OUTPUT);
    digitalWrite(SERVO_POWER_PIN, HIGH);

    // Attach servos and drive them to the Off position before enabling power
    servo1.attach(SERVO1_PIN);
    servo2.attach(SERVO2_PIN);

    servo1.write(SERVO1_OFF_POSITION);
    servo2.write(SERVO2_OFF_POSITION);

    currentPos1 = SERVO1_OFF_POSITION;
    currentPos2 = SERVO2_OFF_POSITION;
    isOn        = false;

    // Brief settle time so the PWM signal is stable before power is applied
    delay(200);

    // Enable servo power now that initialisation is complete
    digitalWrite(SERVO_POWER_PIN, LOW);
}

// ---------------------------------------------------------------------------
// loop
// ---------------------------------------------------------------------------
void loop()
{
    // DEMAND_INPUT_PIN is active low; LOW means "On" requested
    bool demandOn = (digitalRead(DEMAND_INPUT_PIN) == LOW);

    if (demandOn && !isOn)
    {
        moveServosSmooth(SERVO1_ON_POSITION, SERVO2_ON_POSITION);
        isOn = true;
    }
    else if (!demandOn && isOn)
    {
        moveServosSmooth(SERVO1_OFF_POSITION, SERVO2_OFF_POSITION);
        isOn = false;
    }
}
