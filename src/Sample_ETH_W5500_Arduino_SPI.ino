#include <SPI.h>
#include <Ethernet.h>
#include <ArtnetEther.h>

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

unsigned int localPort = 6454; // Art-Net standard port
ArtnetReceiver artnet;    // Art-Net instance
uint16_t universe1 = 1;  // 0 - 15
uint16_t universe2 = 2;  // 0 - 15

void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data) {
    Serial.print("Universe: ");
    Serial.print(universe);
    Serial.print(", Length: ");
    Serial.print(length);
    Serial.print(", Data: ");
    for (int i = 0; i < length; i++) {
        Serial.print(data[i]);
        Serial.print(" ");
    }
    Serial.println();
}

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
    
    // Start Art-Net
    artnet.begin();
    
    // // if Artnet packet comes to this universe, this function is called
    // artnet.subscribeArtDmxUniverse(universe1, [&](const uint8_t* data, const uint16_t size) {
    //     Serial.print("artnet data (universe : ");
    //     Serial.print(universe1);
    //     Serial.print(", size = ");
    //     Serial.print(size);
    //     Serial.print(") :");
    //     for (size_t i = 0; i < size; ++i) {
    //         Serial.print(data[i]);
    //         Serial.print(",");
    //     }
    //     Serial.println();
    // });

    // // you can also use pre-defined callbacks
    // artnet.subscribeArtDmxUniverse(universe2, onDmxFrame);
}

void loop() {
    artnet.parse();
}
