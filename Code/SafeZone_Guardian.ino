/************************************************************
        SAFEZONE GUARDIAN
     ESP32 + Blynk IoT Safety System

     HARDWARE:
     ESP32 DevKit
     MQ-2 Gas Sensor
     IR Obstacle Sensor
     Buzzer Module
     10k + 20k voltage divider
     
     ESP32 PINS:
     MQ-2 AO  -> GPIO 34 (through voltage divider)
     IR OUT   -> GPIO 27
     Buzzer   -> GPIO 26

     BLYNK VIRTUAL PINS:
     V0 = Gas ADC Value
     V1 = Gas Status
     V2 = IR Status
     V3 = Overall Safety Status
************************************************************/

// ==========================================================
//                    BLYNK INFORMATION
// ==========================================================

// Get these values from your Blynk template/device.

#define BLYNK_TEMPLATE_ID   "YOUR_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "SafeZone Guardian"
#define BLYNK_AUTH_TOKEN    "YOUR_AUTH_TOKEN"

// ==========================================================
//                    LIBRARIES
// ==========================================================

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

// ==========================================================
//                    WIFI DETAILS
// ==========================================================

char ssid[] = "YOUR_WIFI_NAME";
char pass[] = "YOUR_WIFI_PASSWORD";

// ==========================================================
//                    HARDWARE PINS
// ==========================================================

const int MQ2_PIN = 34;
const int IR_PIN = 27;
const int BUZZER_PIN = 26;

// ==========================================================
//                    GAS THRESHOLD
// ==========================================================

// Starting prototype threshold.
// CALIBRATE this value with your actual MQ-2 module.

const int GAS_THRESHOLD = 1800;

// Number of MQ-2 readings averaged together
const int GAS_SAMPLES = 10;

// Most common IR modules:
// LOW = object detected
const int IR_DETECTED_STATE = LOW;

// Most buzzer modules:
// HIGH = ON
const bool BUZZER_ACTIVE_HIGH = true;

// ==========================================================
//                    BLYNK TIMER
// ==========================================================

BlynkTimer timer;

// ==========================================================
//             STATE VARIABLES FOR NOTIFICATIONS
// ==========================================================

bool previousGasState = false;
bool previousIRState = false;
bool previousCriticalState = false;

// ==========================================================
//                    BUZZER FUNCTIONS
// ==========================================================

void buzzerOn()
{
  if (BUZZER_ACTIVE_HIGH)
  {
    digitalWrite(BUZZER_PIN, HIGH);
  }
  else
  {
    digitalWrite(BUZZER_PIN, LOW);
  }
}

void buzzerOff()
{
  if (BUZZER_ACTIVE_HIGH)
  {
    digitalWrite(BUZZER_PIN, LOW);
  }
  else
  {
    digitalWrite(BUZZER_PIN, HIGH);
  }
}

// ==========================================================
//                 READ MQ-2 SENSOR
// ==========================================================

int readGasValue()
{
  long total = 0;

  for (int i = 0; i < GAS_SAMPLES; i++)
  {
    total += analogRead(MQ2_PIN);
    delay(5);
  }

  return total / GAS_SAMPLES;
}

// ==========================================================
//                 SEND DATA TO BLYNK
// ==========================================================

void sendSensorData()
{
  // --------------------------------------------------------
  // READ SENSORS
  // --------------------------------------------------------

  int gasValue = readGasValue();
  int irValue = digitalRead(IR_PIN);

  // --------------------------------------------------------
  // DETERMINE CONDITIONS
  // --------------------------------------------------------

  bool gasDetected = (gasValue >= GAS_THRESHOLD);

  bool objectDetected =
      (irValue == IR_DETECTED_STATE);

  bool criticalCondition =
      gasDetected && objectDetected;

  // --------------------------------------------------------
  // SEND VALUES TO BLYNK
  // --------------------------------------------------------

  // V0 = raw/averaged MQ-2 ADC value
  Blynk.virtualWrite(V0, gasValue);

  // V1 = gas status
  Blynk.virtualWrite(V1, gasDetected ? 1 : 0);

  // V2 = IR status
  Blynk.virtualWrite(V2, objectDetected ? 1 : 0);

  // --------------------------------------------------------
  // OVERALL STATUS
  // --------------------------------------------------------

  String safetyStatus;

  if (criticalCondition)
  {
    safetyStatus = "CRITICAL";
  }
  else if (gasDetected)
  {
    safetyStatus = "GAS ALERT";
  }
  else if (objectDetected)
  {
    safetyStatus = "RESTRICTED ZONE";
  }
  else
  {
    safetyStatus = "SAFE";
  }

  // V3 = overall status
  Blynk.virtualWrite(V3, safetyStatus);

  // --------------------------------------------------------
  // SERIAL MONITOR
  // --------------------------------------------------------

  Serial.println();
  Serial.println("======================================");
  Serial.println("        SAFEZONE GUARDIAN");
  Serial.println("======================================");

  Serial.print("MQ-2 Gas Value : ");
  Serial.println(gasValue);

  Serial.print("Gas Status     : ");

  if (gasDetected)
  {
    Serial.println("DANGER");
  }
  else
  {
    Serial.println("NORMAL");
  }

  Serial.print("IR Status      : ");

  if (objectDetected)
  {
    Serial.println("OBJECT DETECTED");
  }
  else
  {
    Serial.println("AREA CLEAR");
  }

  Serial.print("Safety Status  : ");
  Serial.println(safetyStatus);

  // --------------------------------------------------------
  // HAZARD NOTIFICATIONS
  // --------------------------------------------------------

  // Gas state changed from normal to detected
  if (gasDetected && !previousGasState)
  {
    Blynk.logEvent(
      "gas_detected",
      "Gas hazard detected by MQ-2 sensor!"
    );
  }

  // IR state changed from clear to detected
  if (objectDetected && !previousIRState)
  {
    Blynk.logEvent(
      "restricted_zone",
      "Restricted-zone entry detected!"
    );
  }

  // Critical condition changed to detected
  if (criticalCondition && !previousCriticalState)
  {
    Blynk.logEvent(
      "critical_hazard",
      "CRITICAL: Gas and restricted-zone entry detected!"
    );
  }

  // Save current states
  previousGasState = gasDetected;
  previousIRState = objectDetected;
  previousCriticalState = criticalCondition;

  // --------------------------------------------------------
  // LOCAL BUZZER
  // --------------------------------------------------------

  if (criticalCondition)
  {
    // Fast alarm

    buzzerOn();
    delay(150);

    buzzerOff();
    delay(150);

    buzzerOn();
    delay(150);

    buzzerOff();
    delay(150);
  }

  else if (gasDetected)
  {
    // Gas alarm

    buzzerOn();
    delay(400);

    buzzerOff();
    delay(200);
  }

  else if (objectDetected)
  {
    // Restricted-zone warning

    buzzerOn();
    delay(200);

    buzzerOff();
    delay(500);
  }

  else
  {
    // Safe condition

    buzzerOff();
  }
}

// ==========================================================
//                       SETUP
// ==========================================================

void setup()
{
  Serial.begin(115200);

  // --------------------------------------------------------
  // PIN SETUP
  // --------------------------------------------------------

  pinMode(MQ2_PIN, INPUT);
  pinMode(IR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  buzzerOff();

  // --------------------------------------------------------
  // ESP32 ADC
  // --------------------------------------------------------

  analogReadResolution(12);
  analogSetPinAttenuation(MQ2_PIN, ADC_11db);

  // --------------------------------------------------------
  // STARTUP MESSAGE
  // --------------------------------------------------------

  Serial.println();
  Serial.println("======================================");
  Serial.println("      SAFEZONE GUARDIAN ESP32");
  Serial.println("======================================");

  Serial.println("Connecting to Blynk...");
  Serial.println();

  // --------------------------------------------------------
  // CONNECT TO BLYNK
  // --------------------------------------------------------

  Blynk.begin(
    BLYNK_AUTH_TOKEN,
    ssid,
    pass
  );

  Serial.println();
  Serial.println("Blynk connected.");
  Serial.println("System starting...");
  Serial.println();

  // --------------------------------------------------------
  // MQ-2 WARM-UP
  // --------------------------------------------------------

  Serial.println("MQ-2 warming up...");

  for (int i = 15; i > 0; i--)
  {
    Serial.print("Starting in ");
    Serial.print(i);
    Serial.println(" seconds");

    delay(1000);
  }

  Serial.println();
  Serial.println("SYSTEM READY!");
  Serial.println();

  // --------------------------------------------------------
  // RUN SENSOR FUNCTION EVERY 1 SECOND
  // --------------------------------------------------------

  timer.setInterval(1000L, sendSensorData);
}

// ==========================================================
//                        LOOP
// ==========================================================

void loop()
{
  // Keep Blynk connection alive
  Blynk.run();

  // Run scheduled sensor task
  timer.run();
}
