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

unsigned int localPort = 8888;       // local port to listen for UDP packets

const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

// A UDP instance to let us send and receive packets over UDP
EthernetUDP Udp;

// Static IP settings
IPAddress local_IP_AP(192, 168, 1, 22); // Device's IP
IPAddress gateway(192, 168, 1, 5);     // Gateway IP
IPAddress subnet(255, 255, 255, 0);    // Subnet Mask
IPAddress primaryDNS(8, 8, 8, 8);   // optional

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
    if (Udp.parsePacket()) {
        Serial.print("Received packet of size ");
        Serial.println(Udp.available());
        Serial.print("From ");
        IPAddress remote = Udp.remoteIP();
        for (int i = 0; i < 4; i++) {
            Serial.print(remote[i], DEC);
            if (i < 3) {
                Serial.print(".");
            }
        }
        Serial.print(", port ");
        Serial.println(Udp.remotePort());

        // We've received a packet, read the data from it
        Udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
        for (int i = 0; i < NTP_PACKET_SIZE; i++) {
            // print the hexadecimal value of the packet
            Serial.print(packetBuffer[i], DEC);
            Serial.print(" ");
        }
        Serial.println();
        delay(100);
    }
}
