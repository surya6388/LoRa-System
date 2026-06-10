# LoRa-System: Peer-to-Peer Communication

This repository contains the complete firmware and Python interface for a reliable, two-way LoRa communication system using ESP32 and REYAX RYLR890 modules.

## Repository Structure
* `/senderLora`: Firmware for Node 1 (Address: 100).
* `/receiverLora`: Firmware for Node 2 (Address: 200).
* `lora_chat.py`: Python interface to monitor and interact with the LoRa stream via USB Serial.
* `Circuit.png`: Wiring diagram for the ESP32-to-LoRa interface.

## System Overview
The system implements a **Stop-and-Wait ARQ (Automatic Repeat Request)** protocol. 
1. **Reliability:** Each message (`MSG`) must be followed by an acknowledgment (`ACK`) from the destination.
2. **Retries:** If the sender doesn't receive an `ACK` within 2.5 seconds, it automatically retries the transmission (up to 3 times).



## Hardware Connections
Both nodes utilize the same wiring configuration:

| Component | ESP32 Pin |
| :--- | :--- |
| LoRa RX | 16 |
| LoRa TX | 17 |
| Button | 4 |
| LED | 2 |

## How to use
1. **Flash Nodes:** Upload the respective `.ino` files to your ESP32 boards.
2. **Setup:** Connect the boards via USB to your computer.
3. **Run Interface:** You can interact via the Arduino Serial Monitor (115200 baud) or use the provided `lora_chat.py` script to manage the serial traffic from your terminal.

## Protocol Handshake
* **MSG:[ID]:[Payload]** – Outbound data packet.
* **ACK:[ID]** – Confirmation of receipt sent back by the peer.




