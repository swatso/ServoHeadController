#include <Arduino.h>
#include <Servo.h>

// ---------------------------------------------------------------------------
// Pin assignments
// ---------------------------------------------------------------------------
#define SERVO1_PIN        9   // PWM output – servo 1 signal
#define SERVO2_PIN        10  // PWM output – servo 2 signal
#define DEMAND_INPUT_PIN  2   // Digital input  – On/Off demand (active low)
#define SERVO_POWER_PIN   4   // Digital output – servo power enable (active high)
#define PALLET_SENSOR_PIN A3  // Analog pin used as digital input – pallet sensor (active low)
#define LED_PIN           LED_BUILTIN // Digital output – on-board LED (D13), diagnostic: ON when pallet sensor active

// Pallet must be continuously detected for this long before locking is allowed
#define PALLET_DETECT_MS  500

// ---------------------------------------------------------------------------
// Servo pulse lengths (microseconds)
// ---------------------------------------------------------------------------
#define SERVO1_OFF_PULSE_US   2038
#define SERVO1_ON_PULSE_US    1425
#define SERVO2_OFF_PULSE_US   1260
#define SERVO2_ON_PULSE_US    1970

// ---------------------------------------------------------------------------
// Smooth-movement parameters
// ---------------------------------------------------------------------------
// Total travel time in milliseconds
#define MOVE_DURATION_MS  1000
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

float currentPulse1 = SERVO1_OFF_PULSE_US;
float currentPulse2 = SERVO2_OFF_PULSE_US;
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
// palletDetectedStable
// True once the pallet sensor has been continuously active (low) for more
// than PALLET_DETECT_MS. Any inactive reading restarts the timer.
// ---------------------------------------------------------------------------
bool palletDetectedStable()
{
    static unsigned long activeSince = 0;
    static bool wasActive = false;

    bool active = (digitalRead(PALLET_SENSOR_PIN) == LOW);
    if (active && !wasActive)
    {
        activeSince = millis();
    }
    wasActive = active;

    return active && ((millis() - activeSince) > PALLET_DETECT_MS);
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
// supplied target pulse lengths over MOVE_DURATION_MS milliseconds.
// ---------------------------------------------------------------------------
void moveServosSmooth(int targetPulse1, int targetPulse2)
{
    float start1 = currentPulse1;
    float start2 = currentPulse2;

    int steps = MOVE_DURATION_MS / STEP_INTERVAL_MS;

    for (int i = 1; i <= steps; i++)
    {
        float t    = (float)i / (float)steps;
        int pulse1 = (int)(start1 + t * ((float)targetPulse1 - start1));
        int pulse2 = (int)(start2 + t * ((float)targetPulse2 - start2));

        Serial.println(pulse1);
        servo1.writeMicroseconds(pulse1);
        servo2.writeMicroseconds(pulse2);

        delay(STEP_INTERVAL_MS);
    }

    currentPulse1 = (float)targetPulse1;
    currentPulse2 = (float)targetPulse2;
}

// ---------------------------------------------------------------------------
// setup
// ---------------------------------------------------------------------------
void setup()
{
    Serial.begin(9600);
    // Configure demand input with internal pull-up (signal is active low)
    pinMode(DEMAND_INPUT_PIN, INPUT_PULLUP);
    pinMode(PALLET_SENSOR_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
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

    // Start locked only if lock is requested and a pallet is already present
    if (demandStableState && digitalRead(PALLET_SENSOR_PIN) == LOW)
    {
        Serial.println("Initialising servos to On position (demand active, pallet present)...");
        servo1.writeMicroseconds(SERVO1_ON_PULSE_US);
        servo2.writeMicroseconds(SERVO2_ON_PULSE_US);
        currentPulse1 = SERVO1_ON_PULSE_US;
        currentPulse2 = SERVO2_ON_PULSE_US;
        isOn = true;
    }
    else
    {
        Serial.println("Initialising servos to Off position...");
        servo1.writeMicroseconds(SERVO1_OFF_PULSE_US);
        servo2.writeMicroseconds(SERVO2_OFF_PULSE_US);
        currentPulse1 = SERVO1_OFF_PULSE_US;
        currentPulse2 = SERVO2_OFF_PULSE_US;
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
    bool palletOk = palletDetectedStable();

    // Diagnostic: LED mirrors the raw (unfiltered) pallet sensor state
    digitalWrite(LED_PIN, digitalRead(PALLET_SENSOR_PIN) == LOW ? HIGH : LOW);

    // Locking requires demand AND a stable pallet detection. Once locked, the
    // servos stay locked until demand is released (pallet flicker is ignored).
    if (demandOn && palletOk && !isOn)
    {
        Serial.println("Demand ON and pallet detected, moving servos to On position...");
        moveServosSmooth(SERVO1_ON_PULSE_US, SERVO2_ON_PULSE_US);
        isOn = true;
        Serial.println("Servos turned ON");
    }
    else if (!demandOn && isOn)
    {
        Serial.println("Demand OFF detected, moving servos to Off position...");
        moveServosSmooth(SERVO1_OFF_PULSE_US, SERVO2_OFF_PULSE_US);
        isOn = false;
        Serial.println("Servos turned OFF");
    }
}
