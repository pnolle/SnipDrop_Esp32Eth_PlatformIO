#include <SPI.h>
#include <Ethernet.h>

#include <FastLED.h> // include FastLED *before* Artnet
#include <ArtnetEther.h>

// Ethernet module wiring
#define SCK_PIN 14
#define MISO_PIN 12
#define MOSI_PIN 13
#define CS_PIN 15
#define RST_PIN 5  // Optional, leave unconnected if not used
#define IRQ_PIN -1 // 4 // Set to 1 if IRQ is not wired

// define enum for the different modes
enum Mode
{
  MODE_CIRCLE,
  MODE_ARROW,
  MODE_LASERSCISSORS
};

// Code configuration
/*
Valid values defined in enum Mode:
1 = Access Point (192.168.1.24) + Circle (C)
2 = Client 1 (192.168.1.25) Arrow (A)
3 = Client 2 (192.168.1.26) Laser + Scissors (L)
*/
Mode config = Mode::MODE_ARROW;
byte mac[6];
IPAddress local_IP;
IPAddress gateway(192, 168, 1, 5);  // Gateway IP
IPAddress subnet(255, 255, 255, 0); // Subnet Mask
IPAddress primaryDNS(8, 8, 8, 8);   // optional

unsigned int localPort = 6454; // Art-Net standard port
ArtnetReceiver artnet;         // Art-Net instance
uint8_t universe1 = 1;         // 0 - 15
// uint8_t universe2 = 2;  // 0 - 15

// LED settings
CRGB leds[627]; // Maximum number of LEDs needed
const uint8_t PIN_LED_DATA = 22;
const int pixelFactor = 3; // number of pixels displaying the same information to save universes

// Art-Net / DMX settings
const int startUniverse = 0; // CHANGE FOR YOUR SETUP most software this is 1, some software send out artnet first universe as 0.
const int START_UNIVERSE_A = 4;
const int START_UNIVERSE_L = 7;

bool firstDmxFrameReceived = false;

void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t *data)
{
  Serial.print("Universe: ");
  Serial.print(universe);
  Serial.print(", Length: ");
  Serial.print(length);
  Serial.print(", Data: ");
  for (int i = 0; i < length; i++)
  {
    Serial.print(data[i]);
    Serial.print(" ");
  }
  Serial.println();
}

void assignMacAndIps()
{
  if (config == Mode::MODE_CIRCLE)
  {
#define NUM_LEDS 507
    mac[0] = 0xDE;
    mac[1] = 0xAD;
    mac[2] = 0xBE;
    mac[3] = 0xEF;
    mac[4] = 0xFE;
    mac[5] = 0xED;
    local_IP = IPAddress(192, 168, 1, 24);
  }
  if (config == Mode::MODE_ARROW)
  {
#define NUM_LEDS 452
    mac[0] = 0xDE;
    mac[1] = 0xAD;
    mac[2] = 0xBE;
    mac[3] = 0xEF;
    mac[4] = 0xFE;
    mac[5] = 0xEE;
  }
  if (config == Mode::MODE_LASERSCISSORS)
  {
#define NUM_LEDS 627
    mac[0] = 0xDE;
    mac[1] = 0xAD;
    mac[2] = 0xBE;
    mac[3] = 0xEF;
    mac[4] = 0xFE;
    mac[5] = 0xEF;
    local_IP = IPAddress(192, 168, 1, 26);
  }
}

void testBlinkThree(CRGB blinkColor)
{
  for (int r = 0; r < 3; r++)
  {
    for (int i = 0; i < NUM_LEDS; i++)
    {
      CRGB *leds = nullptr;
    }
    FastLED.show();
    delay(500);
    for (int i = 0; i < NUM_LEDS; i++)
    {
      leds[i] = CRGB(0, 0, 0);
    }
    FastLED.show();
    delay(500);
  }
}

void initTest()
{
  Serial.printf("Init test %i\n", config);

  // Test blink three times with color depending on mode: Circle red, Arrow green, Laser + Scissors blue
  if (config == Mode::MODE_CIRCLE)
  {
    testBlinkThree(CRGB(127, 0, 0));
  }
  if (config == Mode::MODE_ARROW)
  {
    testBlinkThree(CRGB(0, 127, 0));
  }
  if (config == Mode::MODE_LASERSCISSORS)
  {
    testBlinkThree(CRGB(0, 0, 127));
  }

  // Default test
  for (int i = 0; i < NUM_LEDS; i++)
  {
    leds[i] = CRGB(127, 127, 127);
  }
  FastLED.show();
  delay(500);
  for (int i = 0; i < NUM_LEDS; i++)
  {
    leds[i] = CRGB(127, 0, 0);
  }
  FastLED.show();
  delay(500);
  for (int i = 0; i < NUM_LEDS; i++)
  {
    leds[i] = CRGB(0, 127, 0);
  }
  FastLED.show();
  delay(500);
  for (int i = 0; i < NUM_LEDS; i++)
  {
    leds[i] = CRGB(0, 0, 127);
  }
  FastLED.show();
  delay(500);
  for (int i = 0; i < NUM_LEDS; i++)
  {
    leds[i] = CRGB(0, 0, 0);
  }
  FastLED.show();
}

void setup()
{
  // Initialize Serial for debugging
  Serial.begin(115200);
  delay(1000); // Allow time for serial monitor to connect
  Serial.println("This firmware is from the 'esp32_ethernet_platformIO' repo, 'artnetReceiver' branch.");

  assignMacAndIps();

  FastLED.addLeds<WS2812, PIN_LED_DATA, GRB>(leds, NUM_LEDS);

  // Configure SPI pins manually
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);

  // Initialize Ethernet with the specified CS pin
  Ethernet.init(CS_PIN);

  // Start Ethernet connection
  Serial.println("Initializing Ethernet...");
  Ethernet.begin(mac, local_IP, primaryDNS, gateway, subnet);
  if (Ethernet.hardwareStatus() != EthernetNoHardware && Ethernet.linkStatus() == LinkON)
  {
    Serial.println("Ethernet initialized successfully!");
    Serial.print("IP Address: ");
    Serial.println(Ethernet.localIP());
  }
  else
  {
    Serial.println("Ethernet initialization failed. Check wiring and settings.");
  }

  // LED test and number display
  initTest();

  // Start Art-Net
  artnet.begin();

  // if Artnet packet comes to this universe, forward them to fastled directly
  artnet.forwardArtDmxDataToFastLED(universe1, leds, NUM_LEDS);

  // // if Artnet packet comes to this universe, this function (lambda) is called
  // artnet.subscribeArtDmxUniverse(universe1, [&](const uint8_t *data, uint16_t size, const ArtDmxMetadata &metadata, const ArtNetRemoteInfo &remote) {
  //     Serial.print("lambda : artnet data from ");
  //     Serial.print(remote.ip);
  //     Serial.print(":");
  //     Serial.print(remote.port);
  //     Serial.print(", universe = ");
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

void loop()
{
  artnet.parse();
  FastLED.show();
}
