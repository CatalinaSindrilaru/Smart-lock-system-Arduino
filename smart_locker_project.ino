#include <Wire.h> 
#include <Servo.h> 
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <Adafruit_Fingerprint.h>

LiquidCrystal_I2C lcd(0x27,16,2);   // 16 characters on a line, 2 lines

bool start = true;
int buzzerPin = 11; // buzzer (pwm pin)
int servoPin = 4;  // servo pin 
Servo Servo1;  // servo object 
int pos = 0;
int failedFingerprintsConsec = 0;

const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns

char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte rowPins[ROWS] = {13, 12, 10, 9}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {8, 7, 6, 5}; //connect to the column pinouts of the keypad

// keypad
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );
char correctCodeRegister[] = "147258"; // Correct 6-digit code (REGISTER)
char enteredCode[7] = "";      // Buffer to store entered code
char correctCodeDelete[] = "258369"; // Correct 6-digit code (DELETE FINGERPRINTS)

// fingerprint reader
SoftwareSerial softwareSerial(2, 3);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&softwareSerial);
uint8_t id = 1; // first id

unsigned long lastFingerprintCheck = 0;
const unsigned long fingerprintCheckInterval = 1000;

void setup()
{
  // lcd setup
  lcd.init();                      
  lcd.backlight();

  // buzzer setup
  pinMode(buzzerPin, OUTPUT);

  // servo motor setup
  Servo1.attach(servoPin); 
  Servo1.write(pos);

  // keypad setup
  Serial.begin(9600);

  // fingerprint reader setup
  // Set the data rate for the sensor serial port
  finger.begin(57600);

  // Check if sensor is connected
  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1) { delay(1); }
  }

  // Get sensor parameters
  Serial.println(F("Reading sensor parameters"));
  finger.getParameters();
  Serial.print(F("Status: 0x")); Serial.println(finger.status_reg, HEX);
  Serial.print(F("Sys ID: 0x")); Serial.println(finger.system_id, HEX);
  Serial.print(F("Capacity: ")); Serial.println(finger.capacity);
  Serial.print(F("Security level: ")); Serial.println(finger.security_level);
  Serial.print(F("Device address: ")); Serial.println(finger.device_addr, HEX);
  Serial.print(F("Packet len: ")); Serial.println(finger.packet_len);
  Serial.print(F("Baud rate: ")); Serial.println(finger.baud_rate);
}

void loop()
{
  if (start) {
    // initial message
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("SMART LOCK");
    lcd.setCursor(0,1);
    lcd.print("SYSTEM");
    start = false;
  }

  if (millis() - lastFingerprintCheck >= fingerprintCheckInterval) {
    lastFingerprintCheck = millis();
    getFingerprintID();
  }
  
  delay(10);

  verify_code();

  // activate buzzer if there are 3 failed fingerprints in a row
  if (failedFingerprintsConsec == 3) {
    failedFingerprintsConsec = 0;
    activateBuzzer();
  }
}

void openDoor() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Opening door...");
  delay(1500);

  for (;pos<=90;pos+=10) {
    Servo1.write(pos);
    delay(80);
  }

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Door is open!");
  delay(3000);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Closing door...");
  delay(1500);

  for (;pos>=0;pos-=10) {
    Servo1.write(pos);
    delay(80);
  }

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Door is closed!");
  delay(2000);
  lcd.clear();
}

void activateBuzzer() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("3 unsuccessful");
  lcd.setCursor(0,1);
  lcd.print("attempts!");

  analogWrite(buzzerPin, 230);
  delay(1000);
  analogWrite(buzzerPin, 0);
  delay(500);
  analogWrite(buzzerPin, 230);
  delay(1000);
  analogWrite(buzzerPin, 0);
  delay(500);
  analogWrite(buzzerPin, 230);
  delay(1000);
  analogWrite(buzzerPin, 0);

  lcd.clear();
}

void verify_code() {
  char key = keypad.getKey();

  if (key) {
    Serial.println(key);
    if (key == '#') { // Check if # key is pressed
      if (strcmp(enteredCode, correctCodeRegister) == 0) {
        registerMode();
      } else if (strcmp(enteredCode, correctCodeDelete) == 0) {
        deleteMode();
      } else {
        incorrectMode();
      }
      // Reset entered code buffer
      memset(enteredCode, 0, sizeof(enteredCode));
    } else {
      // Append the pressed key to the entered code buffer
      if (strlen(enteredCode) < 6) {
        strncat(enteredCode, &key, 1);
      }
    }
  }
}

// called if the register code was introduced
void registerMode() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Correct code!");
  delay(2000);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Register");
  lcd.setCursor(0,1);
  lcd.print("fingerprint!");
  enrollFinger();  // ENROLL NEW FINGERPRINT
}

// called if the delete code was introduced
void deleteMode() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Correct code!");
  delay(2000);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Deleting");
  lcd.setCursor(0,1);
  lcd.print("fingerprints..");
  finger.emptyDatabase();
  delay(2000);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Database");
  lcd.setCursor(0,1);
  lcd.print("empty now.");
  delay(2000);
  lcd.clear();
  id = 1; // reset ids for users
}

// if an incorrect code was introduced
void incorrectMode() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Incorrect code!");
  delay(2000);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Try again!");
  delay(2000);
  lcd.clear();
}

void enrollFinger() {
  Serial.println("Enrolling...");
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Enrolling...");
  delay(1000);

  // Scan fingerprint
  while (!getFingerprintEnroll());
  id++;

  delay(2000);
}

uint8_t getFingerprintEnroll() {

  int p = -1;
  Serial.print("Waiting for valid finger to enroll as #"); Serial.println(id);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Place finger!");

  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.print(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Remove finger!");
  Serial.println("Remove finger");
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  Serial.print("ID "); Serial.println(id);
  p = -1;
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Place same");
  lcd.setCursor(0,1);
  lcd.print("finger again!");
  Serial.println("Place same finger again");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.print(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK converted!
  Serial.print("Creating model for #");  Serial.println(id);

  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Fingerprints");
    lcd.setCursor(0,1);
    lcd.print("match!");
    delay(1500);
    Serial.println("Prints matched!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Fingerprints did not match");
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Fingerprints");
    lcd.setCursor(0,1);
    lcd.print("did not match!");
    delay(1000);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Introduce"); 
    lcd.setCursor(0,1);
    lcd.print("code again!");
    delay(1500);
    lcd.clear();
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  Serial.print("ID "); Serial.println(id);
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Fingerprint");
    lcd.setCursor(0,1);
    lcd.print("registered!");
    Serial.println("Stored!");
    delay(2000);
    lcd.clear();
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not store in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  return true;
}


uint8_t getFingerprintID() {
  uint8_t p = finger.getImage();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK success!

  p = finger.image2Tz();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK converted!
  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK) {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Found a match!");
    failedFingerprintsConsec = 0; // reset the variable
    delay(1000);
    openDoor(); // OPEN DOOR
    Serial.println("Found a print match!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_NOTFOUND) {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Did not find");
    lcd.setCursor(0,1);
    lcd.print("a match!");
    Serial.println("Did not find a match");
    delay(1500);
    failedFingerprintsConsec++; // increase failed fingerprints variable
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Register finger");
    lcd.setCursor(0,1);
    lcd.print("first!");
    delay(1500);
    lcd.clear();
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  Serial.print(" with confidence of "); Serial.println(finger.confidence);

  return finger.fingerID;
}

// returns -1 if failed, otherwise returns ID #
int getFingerprintIDez() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)  return -1;

  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  Serial.print(" with confidence of "); Serial.println(finger.confidence);
  return finger.fingerID;
}
