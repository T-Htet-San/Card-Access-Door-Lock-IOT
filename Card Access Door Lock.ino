#include <ESP8266WiFi.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>

#define SS_PIN 2   // Pin for RFID SS
#define RST_PIN 0  // Pin for RFID RST
#define SERVO_PIN 4  // Pin for the Servo
#define GREEN_LED_PIN 15  // Pin for Green LED
#define RED_LED_PIN 5  // Pin for Red LED

// Wi-Fi credentials
const char* ssid = "NodeMCU__HOTSPOT";
const char* password = "12341234";

// Create a server instance on port 80
WiFiServer server(80);

MFRC522 rfid(SS_PIN, RST_PIN);  // Create RFID instance
Servo myServo;  // Create Servo instance
MFRC522::MIFARE_Key key;

// Set allowed card ID to F3 68 00 28
byte allowedCard[4] = {0xE3, 0xC4, 0x59, 0x14};  

bool doorOpen = false;  // Variable to track if the door is open or closed

void setup() {
  Serial.begin(115200);  // Initialize serial communication
  SPI.begin();  // Init SPI bus
  rfid.PCD_Init();  // Init MFRC522
  myServo.attach(SERVO_PIN);  // Attach the servo to the specified pin
  myServo.write(0);  // Initialize servo to closed position

  // Configure LED pins as output
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);

  // Turn off both LEDs initially
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);

  // Disable built-in LED
  //pinMode(LED_BUILTIN, OUTPUT);   //Set built-in LED pin as output
  //digitalWrite(LED_BUILTIN, HIGH);  //Turn off the built-in LED (since it’s active-low)

  // Start Wi-Fi in Access Point mode
  Serial.println("Setting up Wi-Fi...");
  WiFi.softAP(ssid, password);
  
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  // Start the server
  server.begin();
  Serial.println("Server started.");
}

void loop() {
  // Look for a card
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    Serial.print("Card UID: ");
    for (byte i = 0; i < rfid.uid.size; i++) {
      Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
      Serial.print(rfid.uid.uidByte[i], HEX);
    }
    Serial.println();

    // Check if the scanned card matches the allowed card
    if (checkCard(rfid.uid.uidByte)) {
      Serial.println("Card recognized");
      digitalWrite(GREEN_LED_PIN, HIGH);  // Turn on the green LED
      digitalWrite(RED_LED_PIN, LOW);  // Turn off the red LED
      if (doorOpen) {
        closeDoor();  // Close the door if it's currently open
      } else {
        openDoor();  // Open the door if it's currently closed
      }
    } else {
      Serial.println("Access Denied");
      digitalWrite(GREEN_LED_PIN, LOW);  // Turn off the green LED
      digitalWrite(RED_LED_PIN, HIGH);  // Turn on the red LED
    }
    rfid.PICC_HaltA();  // Halt the RFID reader
  }

  // Handle client requests
  WiFiClient client = server.available();
  if (client) {
    Serial.println("New Client connected.");
    String currentLine = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (c == '\n') {
          if (currentLine.length() == 0) {
            // Send the HTML response
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/html");
            client.println("Connection: close");
            client.println();
            
            // HTML + CSS for styled interface
            client.println("<!DOCTYPE html><html>");
            client.println("<head><title>Card Access Door Lock</title>");
            client.println("<meta http-equiv='refresh' content='3'>");  // Auto-refresh every 3 seconds
            client.println("<style>");
            client.println("body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background-color: #f7f8fa; color: #444; display: flex; justify-content: center; align-items: center; height: 100vh; margin: 0; }");
            client.println("h1 { font-size: 2.5em; color: #333; margin-bottom: 20px; }");
            client.println(".container { background-color: #fff; padding: 20px 40px; border-radius: 12px; box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1); text-align: center; }");
            client.println(".status { font-size: 1.2em; padding: 12px 20px; color: #fff; border-radius: 8px; display: inline-block; width: 200px; margin-top: 10px; }");
            client.println(".open { background-color: #28a745; }");  // Green for door open
            client.println(".closed { background-color: #dc3545; }");  // Red for door closed
            client.println("footer { margin-top: 20px; font-size: 0.9em; color: #888; }");
            client.println("</style>");
            client.println("</head><body>");

            client.println("<div class='container'>");
            client.println("<h1>Door Lock Status</h1>");

            // Display door status
            client.println("<div class='status " + String(doorOpen ? "open" : "closed") + "'>" + String(doorOpen ? "Door Open" : "Door Closed") + "</div>");
            client.println("<footer></footer>");

            client.println("</div>");
            client.println("</body></html>");

            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }
    client.stop();
    Serial.println("Client disconnected.");
  }
  delay(100);
}

// Function to check if the scanned card matches the allowed card UID
bool checkCard(byte *readCard) {
  for (byte i = 0; i < 4; i++) {
    if (readCard[i] != allowedCard[i]) {
      return false;
    }
  }
  return true;
}

// Function to open the door (rotate the servo to open position)
void openDoor() {
  Serial.println("Opening Door...");
  myServo.write(180);  // Rotate the servo to 180 degrees (open)
  delay(1000);  // Let the servo move to the position
  doorOpen = true;  // Update the door state to open
}

// Function to close the door (rotate the servo to closed position)
void closeDoor() {
  Serial.println("Closing Door...");
  myServo.write(0);  // Rotate the servo to 0 degrees (closed)
  delay(1000);  // Let the servo move to the position
  doorOpen = false;  // Update the door state to closed
  digitalWrite(GREEN_LED_PIN, LOW);
}
