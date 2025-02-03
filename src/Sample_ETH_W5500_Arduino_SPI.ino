#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>

// Pin definitions based on your wiring
#define SCK_PIN 14
#define MISO_PIN 12
#define MOSI_PIN 13
#define CS_PIN 15
#define RST_PIN 5 // Optional, leave unconnected if not used
#define IRQ_PIN -1 //4 // Set to 1 if IRQ is not wired

// MAC address for the W5500
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// Static IP settings
IPAddress local_IP_AP(192, 168, 1, 22); // Device's IP
IPAddress gateway(192, 168, 1, 5);     // Gateway IP
IPAddress subnet(255, 255, 255, 0);    // Subnet Mask
IPAddress primaryDNS(8, 8, 8, 8);   // optional

unsigned int localPort = 8888;       // local port to listen for UDP packets
EthernetUDP Udp;    // A UDP instance to let us send and receive packets over UDP

void setup() {
    // Initialize Serial for debugging
    Serial.begin(115200);
    delay(1000); // Allow time for serial monitor to connect

    // Configure SPI pins manually
    SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);

    // Initialize Ethernet with the specified CS pin
    Ethernet.init(CS_PIN);

    // Start Ethernet connection
    Serial.println("Initializing Ethernet...");
    Ethernet.begin(mac, local_IP_AP, primaryDNS, gateway, subnet);
    if (Ethernet.hardwareStatus() != EthernetNoHardware && Ethernet.linkStatus() == LinkON) {
        Serial.println("Ethernet initialized successfully!");
        Serial.print("IP Address: ");
        Serial.println(Ethernet.localIP());
    } else {
        Serial.println("Ethernet initialization failed. Check wiring and settings.");
    }
  Udp.begin(localPort);
}

void loop() {  
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    char packetBuffer[packetSize + 1];
    Udp.read(packetBuffer, packetSize);
    packetBuffer[packetSize] = '\0'; // Null-terminate the string
    Serial.print("Received packet: ");
    Serial.println(packetBuffer);
  }
  delay(10);
}
