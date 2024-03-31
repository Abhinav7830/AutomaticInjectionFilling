#include <Keypad.h>
#include <LiquidCrystal.h>

const int PulseWire = A0; // PulseSensor connected to analog pin A0
const int motorIN1 = 18; // Motor driver input 1 connected to pin 2
const int motorIN2 = 19; // Motor driver input 2 connected to pin 3
const int motorEnable = 12; // Motor driver enable pin connected to pin 12
const int keypadRows = 4; // Rows of the keypad
const int keypadCols = 4; // Columns of the keypad

char keys[keypadRows][keypadCols] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte rowPins[keypadRows] = {17, 16, 15, 14}; // Connect to the row pinouts of the keypad
byte colPins[keypadCols] =  {11, 10, 9, 8}; // Connect to the column pinouts of the keypad

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, keypadRows, keypadCols);
LiquidCrystal lcd(2, 3, 4, 5, 6, 7); // LCD pins

const int thresholdBPM = 130; // Threshold heart rate for drug injection
const int minBPM = 60; // Minimum heart rate considered valid
const int motorSpeed = 220; // Motor speed (0-255)
const char* authorizationKey = "5839"; // Authorization key (password)

bool authorized = false;
bool injecting = false;
unsigned long startTime = 0;
int injectionTime = 0; // Time to inject drugs in milliseconds, default 1 second

void setup() {
  pinMode(motorIN1, OUTPUT);
  pinMode(motorIN2, OUTPUT);
  analogWrite(motorEnable, 255); // Set motor speed
  digitalWrite(motorIN1, LOW); // Rotate motor in one direction
  digitalWrite(motorIN2, HIGH);
  delay(1000);
  digitalWrite(motorIN1, LOW); // Rotate motor in one direction
  digitalWrite(motorIN2, LOW);
  pinMode(motorEnable, OUTPUT);
  digitalWrite(motorEnable, LOW); // Disable motor initially
  Serial.begin(9600);
  lcd.begin(16, 2);
  lcd.print("System Initializing");
  delay(2000);
  lcd.clear();
  lcd.print("Enter Authorization");
  lcd.setCursor(0, 1);
  lcd.print("Key: ");
}

void loop() {
  if (!authorized) {
    authorize();
  } else {
    int sensorValue = analogRead(PulseWire);
    int BPM = map(sensorValue, 0, 1023, 0, 255); // Map the sensor value to BPM
    Serial.println("BPM: " + String(BPM));

    if (BPM >= minBPM && BPM <= thresholdBPM) {
      lcd.clear();
      lcd.print("Heart rate within");
      lcd.setCursor(0, 1);
      lcd.print("safe range.");
      digitalWrite(motorEnable, LOW); // Set motor speed to minimum
    } else {
      lcd.clear();
      lcd.print("Abnormal heart rate");
      lcd.setCursor(0, 1);
      lcd.print("detected.");
      analogWrite(motorEnable, motorSpeed); // Set motor speed
    }

    if (BPM > thresholdBPM) {
      if (!injecting) {
        injectDrug();
      }
    }

    delay(1000); // Delay for stability
  }
}

void injectDrug() {
  lcd.clear();
  lcd.print("Setting injection");
  lcd.setCursor(0, 1);
  lcd.print("time: ");
  
  // Wait for the user to input the desired injection time
  String inputTime = "";
  while (inputTime.length() < 2) {
    char key = keypad.getKey();
    if (key >= '1' && key <= '5') {
      inputTime += key;
      lcd.setCursor(11, 1);
      lcd.print(inputTime);
      delay(200); // debounce delay
    }
  }
  
  // Convert input time to milliseconds
  int timeInSeconds = inputTime.toInt();
  injectionTime = timeInSeconds * 200; // Convert to milliseconds

  // Ensure the injection time is within the desired range
  injectionTime = constrain(injectionTime, 200, 1000);

  lcd.clear();
  lcd.print("Injecting drug...");
  digitalWrite(motorIN1, HIGH); // Rotate motor in one direction
  digitalWrite(motorIN2, LOW);
  
  startTime = millis(); // Record the start time
  injecting = true;

  // Keep the motor running for the specified duration
  while (millis() - startTime < injectionTime) {
    // Check if there's a key press
    char key = keypad.getKey();
    if (key == '*') {
      lcd.clear();
      lcd.print("Injection canceled");
      digitalWrite(motorIN1, LOW); // Stop motor
      digitalWrite(motorIN2, LOW);
      injecting = false;
      return; // Exit the function
    }
    // Delay for a short period to avoid busy-waiting
    delay(100);
  }
  
  digitalWrite(motorIN1, LOW); // Stop motor
  digitalWrite(motorIN2, LOW);
  lcd.clear();
  lcd.print("Drug injection");
  lcd.setCursor(0, 1);
  lcd.print("completed.");

  injecting = false;
}

void authorize() {
  static String inputKey = "";
  
  char key = keypad.getKey();
  if (key != '\0')  // Check if a valid key is pressed
  {
    lcd.setCursor(10 + inputKey.length(), 1);
    lcd.print(key); // Print the key pressed on the LCD
    inputKey += key;
    Serial.print(key);
  }
  
  if (inputKey.length() == strlen(authorizationKey)) {
    if (inputKey == authorizationKey) {
      authorized = true;
      lcd.clear();
      lcd.print("Authorization");
      lcd.setCursor(0, 1);
      lcd.print("successful.");
      delay(2000);
      lcd.clear();
    } else {
      lcd.clear();
      lcd.print("Authorization");
      lcd.setCursor(0, 1);
      lcd.print("failed. Retry.");
      delay(2000);
      lcd.clear();
    }
    inputKey = "";
    lcd.clear();
  }
}
