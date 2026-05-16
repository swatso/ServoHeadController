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
#define SERVO1_ON_POSITION   29
#define SERVO2_OFF_POSITION  0
#define SERVO2_ON_POSITION   32

// ---------------------------------------------------------------------------
// Smooth-movement parameters
// ---------------------------------------------------------------------------
// Total travel time in milliseconds
#define MOVE_DURATION_MS  5000
// Time between individual position steps (ms) – smaller = smoother
#define STEP_INTERVAL_MS    20

// Demand input filtering
#define DEMAND_BOOT_SETTLE_MS  300
#define DEMAND_DEBOUNCE_MS     80

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------
Servo servo1;
Servo servo2;

float currentPos1 = SERVO1_OFF_POSITION;
float currentPos2 = SERVO2_OFF_POSITION;
bool  isOn        = false;

// Debounced demand tracking
bool demandRawLast        = false;
bool demandStableState    = false;
unsigned long demandSince = 0;

// ---------------------------------------------------------------------------
// readDemandOnRaw
// Reads demand input as a boolean where true means "On" requested.
// ---------------------------------------------------------------------------
bool readDemandOnRaw()
{
    return (digitalRead(DEMAND_INPUT_PIN) == LOW);
}

// ---------------------------------------------------------------------------
// initializeDemandState
// Waits briefly for external circuitry to settle, then captures a stable
// initial demand state to avoid startup chatter.
// ---------------------------------------------------------------------------
void initializeDemandState()
{
    delay(DEMAND_BOOT_SETTLE_MS);

    bool sample = readDemandOnRaw();
    unsigned long stableStart = millis();

    while ((millis() - stableStart) < DEMAND_DEBOUNCE_MS)
    {
        bool now = readDemandOnRaw();
        if (now != sample)
        {
            sample = now;
            stableStart = millis();
        }
        delay(2);
    }

    demandRawLast     = sample;
    demandStableState = sample;
    demandSince       = millis();
}

// ---------------------------------------------------------------------------
// readDemandOnDebounced
// Applies a time-based debounce filter to reject short input glitches.
// ---------------------------------------------------------------------------
bool readDemandOnDebounced()
{
    bool raw = readDemandOnRaw();
    if (raw != demandRawLast)
    {
        demandRawLast = raw;
        demandSince = millis();
    }

    if ((millis() - demandSince) >= DEMAND_DEBOUNCE_MS)
    {
        demandStableState = raw;
    }

    return demandStableState;
}

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

        Serial.println(pos1);
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
    Serial.begin(9600);
    // Configure demand input with internal pull-up (signal is active low)
    pinMode(DEMAND_INPUT_PIN, INPUT_PULLUP);
    Serial.println("Initializing...");

    // Servo power output – keep power OFF during initialisation
    // Active-high: HIGH = power On, LOW = power Off
    pinMode(SERVO_POWER_PIN, OUTPUT);
    digitalWrite(SERVO_POWER_PIN, LOW);

    initializeDemandState();

    Serial.println("Attaching servos and moving to Off position...");
    // Attach servos and drive them to the Off position before enabling power
    servo1.attach(SERVO1_PIN);
    servo2.attach(SERVO2_PIN);

    if (demandStableState)
    {
        Serial.println("Initialising servos to On position (demand active)...");
        servo1.write(SERVO1_ON_POSITION);
        servo2.write(SERVO2_ON_POSITION);
        currentPos1 = SERVO1_ON_POSITION;
        currentPos2 = SERVO2_ON_POSITION;
        isOn = true;
    }
    else
    {
        Serial.println("Initialising servos to Off position (demand inactive)...");
        servo1.write(SERVO1_OFF_POSITION);
        servo2.write(SERVO2_OFF_POSITION);
        currentPos1 = SERVO1_OFF_POSITION;
        currentPos2 = SERVO2_OFF_POSITION;
        isOn = false;
    }

    // Brief settle time so the PWM signal is stable before power is applied
    delay(200);

    Serial.println("Initialisation complete, enabling servo power...");
    // Enable servo power now that initialisation is complete
    digitalWrite(SERVO_POWER_PIN, HIGH);
    Serial.println("Setup complete, waiting for demand input...");
}

// ---------------------------------------------------------------------------
// loop
// ---------------------------------------------------------------------------
void loop()
{
    bool demandOn = readDemandOnDebounced();

    if (demandOn && !isOn)
    {
        Serial.println("Demand ON detected, moving servos to On position...");
        moveServosSmooth(SERVO1_ON_POSITION, SERVO2_ON_POSITION);
        isOn = true;
        Serial.println("Servos turned ON");
    }
    else if (!demandOn && isOn)
    {
        Serial.println("Demand OFF detected, moving servos to Off position...");
        moveServosSmooth(SERVO1_OFF_POSITION, SERVO2_OFF_POSITION);
        isOn = false;
        Serial.println("Servos turned OFF");
    }
}
