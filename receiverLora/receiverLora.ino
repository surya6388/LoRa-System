/*
ESP32 LoRa RYLR890 - Node 2 (Address 200)
With Serial Monitor Chat + ACK + Retry + Queue
*/
#include <HardwareSerial.h>
#include <queue> 

#define BUTTON_PIN 4
#define LED_PIN 2
#define RX2_PIN 16
#define TX2_PIN 17

#define LORA_ADDRESS 200
#define DEST_ADDRESS 100
#define NETWORK_ID 5
#define FREQUENCY_BAND 865000000

#define MAX_RETRIES 3
#define ACK_TIMEOUT 2500 
#define RETRY_DELAY 800
#define MAX_QUEUE_SIZE 10

HardwareSerial LoRaSerial(2);

struct QueuedMessage {
  String payload;
  int msgId;
};

std::queue<QueuedMessage> messageQueue;
bool buttonState = false;
bool lastButtonState = false;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;
int messageCounter = 0;
bool waitingForAck = false;
int currentMsgId = 0;
unsigned long ackStartTime = 0;
int retryCount = 0;

String readLoRaResponse(unsigned long timeout = 1000) {
  String response = "";
  unsigned long start = millis();
  while (millis() - start < timeout) {
    if (LoRaSerial.available()) {
      response += (char)LoRaSerial.read();
    }
    delay(1);
  }
  response.trim();
  return response;
}

void sendATCommand(String command) {
  while (LoRaSerial.available()) LoRaSerial.read();
  LoRaSerial.println(command);
  delay(100);
  String response = readLoRaResponse(800);
  if (response.length() > 0) {
    Serial.print("AT: "); Serial.print(command);
    Serial.print(" → "); Serial.println(response);
  }
}

void setupLoRa() {
  delay(1000);
  sendATCommand("AT+RESET");
  delay(1500);
  sendATCommand("AT");
  sendATCommand("AT+ADDRESS=" + String(LORA_ADDRESS));
  sendATCommand("AT+NETWORKID=" + String(NETWORK_ID));
  sendATCommand("AT+BAND=" + String(FREQUENCY_BAND));
  sendATCommand("AT+PARAMETER?");
}

void blinkLED(int times, int delayMs) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(delayMs);
    digitalWrite(LED_PIN, LOW);
    delay(delayMs);
  }
}

void sendRawMessage(String message) {
  String cmd = "AT+SEND=" + String(DEST_ADDRESS) + "," + String(message.length()) + "," + message;
  while (LoRaSerial.available()) LoRaSerial.read();
  LoRaSerial.println(cmd);
  delay(100);
  String resp = readLoRaResponse(600);
  if (resp.indexOf("+OK") != -1) {
    Serial.println(" -> Packet transmitted out via radio...");
  }
}

void processQueue() {
  if (!messageQueue.empty() && !waitingForAck) {
    QueuedMessage nextMsg = messageQueue.front();
    currentMsgId = nextMsg.msgId;
    retryCount = 0;
    String fullMessage = "MSG:" + String(currentMsgId) + ":" + nextMsg.payload;

    Serial.println("\n--- Outbound Message ---");
    Serial.print("Sending: "); Serial.println(nextMsg.payload);

    sendRawMessage(fullMessage);
    waitingForAck = true;
    ackStartTime = millis();
  }
}

void enqueueMessage(String payload) {
  if (messageQueue.size() >= MAX_QUEUE_SIZE) {
    messageQueue.pop();
  }
  messageCounter++;
  QueuedMessage msg = {payload, messageCounter};
  messageQueue.push(msg);
  processQueue();
}

void parseReceivedPacket(String packet) {
  int firstComma = packet.indexOf(',');
  int secondComma = packet.indexOf(',', firstComma + 1);
  int thirdComma = packet.indexOf(',', secondComma + 1);
  if (firstComma == -1 || secondComma == -1 || thirdComma == -1) return;
  
  String message = packet.substring(secondComma + 1, thirdComma);
  
  if (message.startsWith("ACK:")) {
    int ackId = message.substring(4).toInt();
    if (waitingForAck && ackId == currentMsgId) {
      waitingForAck = false;
      Serial.println(" [ACK Received back successfully!]");
      if (!messageQueue.empty()) messageQueue.pop();
      blinkLED(2, 60);
      processQueue();
    }
    return;
  }
  
  if (message.startsWith("MSG:")) {
    int colon1 = message.indexOf(':', 4);
    if (colon1 != -1) {
      String msgIdStr = message.substring(4, colon1);
      String payload = message.substring(colon1 + 1);
      
      Serial.println("\n=== Incoming Message ===");
      Serial.print("Received: "); Serial.println(payload);

      String ackMsg = "ACK:" + msgIdStr;
      sendRawMessage(ackMsg);
      blinkLED(1, 200);
    }
  }
}

void receiveMessage() {
  static String buffer = "";
  buffer.reserve(128);
  while (LoRaSerial.available()) {
    char c = LoRaSerial.read();
    buffer += c;
    if (c == '\n') {
      if (buffer.indexOf("+RCV") != -1) {
        parseReceivedPacket(buffer);
      }
      buffer = "";
    }
  }
}

// NEW: This checks if you typed anything into your laptop's Serial Monitor
void checkSerialInput() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim(); 
    if (input.length() > 0) {
      enqueueMessage(input);
    }
  }
}

void checkButton() {
  int reading = digitalRead(BUTTON_PIN);
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == LOW) {
        enqueueMessage("Ping from Button 200!");
        delay(400); 
      }
    }
  }
  lastButtonState = reading;
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32 LoRa Chat System Starting...");
  
  LoRaSerial.begin(115200, SERIAL_8N1, RX2_PIN, TX2_PIN);
  
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  setupLoRa();
  Serial.println("Ready! Type a message above and press Enter, or use the physical button.");
}

void loop() {
  checkButton();
  checkSerialInput(); // Continuously scan for keyboard input
  receiveMessage();
  
  if (waitingForAck) {
    if (millis() - ackStartTime >= ACK_TIMEOUT) {
      retryCount++;
      if (retryCount <= MAX_RETRIES) {
        Serial.print(" No ACK received. Retrying... ("); Serial.print(retryCount); Serial.println(")");
        QueuedMessage current = messageQueue.front();
        String fullMsg = "MSG:" + String(current.msgId) + ":" + current.payload;
        sendRawMessage(fullMsg);
        ackStartTime = millis();
      } else {
        Serial.println(" [Delivery Failed: Max retries reached]");
        if (!messageQueue.empty()) messageQueue.pop();
        waitingForAck = false;
        processQueue(); 
      }
    }
  } else {
    processQueue(); 
  }
  delay(10);
}