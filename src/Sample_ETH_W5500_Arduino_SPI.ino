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
    MODE_CIRCLE = 0,
    MODE_ARROW = 1,
    MODE_LASERSCISSORS = 2
};

// Firmware configuration
#define CONFIG MODE_ARROW

byte mac[6];
IPAddress local_IP;
IPAddress gateway(192, 168, 1, 5);  // Gateway IP
IPAddress subnet(255, 255, 255, 0); // Subnet Mask
IPAddress primaryDNS(8, 8, 8, 8);   // optional

unsigned int localPort = 6454; // Art-Net standard port
ArtnetReceiver artnet;         // Art-Net instance
uint8_t universe1 = 1;         // 0 - 15
// uint8_t universe2 = 2;  // 0 - 15

// LED settings for LED strips
const int NUM_LEDS_C = 507; // 507 leds_A in Circle
const int NUM_LEDS_A = 452; // 452 leds_A in Arrow
// const int NUM_LEDS_L = 646; // 646 leds_A in Laser v3 + Scissors, 585 in use without deadSpace
const int NUM_LEDS_L = 627; // 646 leds_A in Laser v3 + Scissors, 585 in use without deadSpace
CRGB leds_C[NUM_LEDS_C];
CRGB leds_A[NUM_LEDS_A];
CRGB leds_L[NUM_LEDS_L];

const uint8_t PIN_LED_DATA = 22;
const int pixelFactor = 3; // number of pixels displaying the same information to save universes

// Art-Net / DMX settings
const int startUniverse = 0; // CHANGE FOR YOUR SETUP most software this is 1, some software send out artnet first universe as 0.
const int START_UNIVERSE_A = 4;
const int START_UNIVERSE_L = 7;
int thisUniverse = 0;

bool firstDmxFrameReceived = false;

void assignMacAndIps()
{
#if CONFIG == MODE_CIRCLE
  mac[0] = 0xDE; // 222
  mac[1] = 0xAD; // 173
  mac[2] = 0xBE; // 190
  mac[3] = 0xEF; // 239
  mac[4] = 0xFE; // 254
  mac[5] = 0xED; // 237
  local_IP = IPAddress(192, 168, 1, 24);

#elif CONFIG == MODE_ARROW
  mac[0] = 0xDE;
  mac[1] = 0xAD;
  mac[2] = 0xBE;
  mac[3] = 0xEF;
  mac[4] = 0xFE;
  mac[5] = 0xEE; // 238
  local_IP = IPAddress(192, 168, 1, 25);

#elif CONFIG == MODE_LASERSCISSORS
  mac[0] = 0xDE;
  mac[1] = 0xAD;
  mac[2] = 0xBE;
  mac[3] = 0xEF;
  mac[4] = 0xFE;
  mac[5] = 0xEF; // 239
  local_IP = IPAddress(192, 168, 1, 26);
#endif
}

void testBlinkThree(CRGB blinkColor, int numLeds, CRGB* leds)
{
  for (int r = 0; r < 3; r++)
  {
    for (int i = 0; i < numLeds; i++)
    {
      leds[i] = blinkColor;
    }
    FastLED.show();
    delay(500);
    for (int i = 0; i < numLeds; i++)
    {
      leds[i] = CRGB(0, 0, 0);
    }
    FastLED.show();
    delay(500);
  }
}

void initTest(int numLeds, CRGB* leds)
{
  Serial.printf("Init test %i\n", CONFIG);

  // Test blink three times with color depending on mode: Circle red, Arrow green, Laser + Scissors blue
  if (CONFIG == MODE_CIRCLE)
  {
    testBlinkThree(CRGB(127, 0, 0), numLeds, leds);
  }
  if (CONFIG == MODE_ARROW)
  {
    testBlinkThree(CRGB(0, 127, 0), numLeds, leds);
  }
  if (CONFIG == MODE_LASERSCISSORS)
  {
    testBlinkThree(CRGB(0, 0, 127), numLeds, leds);
  }

  // Default test
  for (int i = 0; i < numLeds; i++)
  {
    leds[i] = CRGB(127, 127, 127);
  }
  FastLED.show();
  delay(500);
  for (int i = 0; i < numLeds; i++)
  {
    leds[i] = CRGB(127, 0, 0);
  }
  FastLED.show();
  delay(500);
  for (int i = 0; i < numLeds; i++)
  {
    leds[i] = CRGB(0, 127, 0);
  }
  FastLED.show();
  delay(500);
  for (int i = 0; i < numLeds; i++)
  {
    leds[i] = CRGB(0, 0, 127);
  }
  FastLED.show();
  delay(500);
  for (int i = 0; i < numLeds; i++)
  {
    leds[i] = CRGB(0, 0, 0);
  }
  FastLED.show();
}

CRGB getColors(int i, const uint8_t *data)
{
  return CRGB(data[i * 3], data[i * 3 + 1], data[i * 3 + 2]);
}

// [&](const uint8_t *data, uint16_t size, const ArtDmxMetadata& metadata, const ArtNetRemoteInfo& remote) {
//   // if Artnet packet comes, this function is called for every universe
// }
void onDmxFrame(const uint8_t *data, uint16_t size, const ArtDmxMetadata &metadata, const ArtNetRemoteInfo &remote)
{
  Serial.print("NAMED subscribeArtDmxUniverse: artnet data from ");
  Serial.println(remote.ip);
  Serial.print(":");
  Serial.println(remote.port);
  Serial.print("size: ");
  Serial.println(size);
  Serial.print(", universe: ");
  Serial.println(thisUniverse);
  // Serial.print(", Data: ");
  // for (size_t i = 0; i < size; ++i)
  // {
  //   Serial.print(data[i]);
  //   Serial.print(" ");
  // }
  // Serial.println();

  if (!firstDmxFrameReceived)
  {
    Serial.println("DMX reception started.");
    firstDmxFrameReceived = true;
  }

  // // set brightness of the whole strip
  // if (universe == 15)
  // {
  //   FastLED.setBrightness(data[0]);
  //   FastLED.show();
  // }

  // // range check
  // if (universe < startUniverse)
  // {
  //   return;
  // }
  // uint8_t thisUniverse = universe - startUniverse;

  // Serial.printf("onDmxFrame %u/%u %u %u %i %i\n", universe, maxUniverses, size, sequence, thisUniverse);

  // special treatment for L strip
  int leapLCounter = 0;
  int leapLNow = 0;

  // read universe and put into the right part of the display buffer
  for (int i = 0; i < size / 3; i++)
  {
    // thisUniverse is the first relevant universe
    if (thisUniverse < START_UNIVERSE_A) // this is the C strip on universe 1
    {
      int led = i * pixelFactor + ((thisUniverse - 1) * 170); // for thisUniverse==1 ? led start at 0 : led start at 170
      // Serial.printf("C-STRIP from 0 to %i \tled%i/%i %u/%u-%i %u %u %i %i\n", START_UNIVERSE_A-1, led, NUM_LEDS_C, universe, maxUniverses, 0, size, sequence, thisUniverse, sendFrame);
      for (int p = 0; p < pixelFactor; p++)
      {
        if (led < NUM_LEDS_C)
        {
          leds_C[led] = getColors(i, data);
          // Serial.printf("ledNo %i | r %i | g %i | b %i\n", led, data[i * 3], data[i * 3 + 1], data[i * 3 + 2]);
        }
        led++;
      }
    }
    else if (thisUniverse >= START_UNIVERSE_A && thisUniverse < START_UNIVERSE_L) // this is the A strip on universe 4
    {
      int led = i * pixelFactor + ((thisUniverse - START_UNIVERSE_A) * 170); // for thisUniverse==3 ? led start at 0 : led start at 170
      // Serial.printf("%i: A-STRIP from %i to %i \tthisUniverse%i led%i/%i %u/%u %u\n", i, START_UNIVERSE_A, START_UNIVERSE_L-1, thisUniverse, led, NUM_LEDS_A, size);
      for (int p = 0; p < pixelFactor; p++)
      {
        if (led < NUM_LEDS_A)
        {
          leds_A[led] = getColors(i, data);
          // Serial.printf("leds_A ledNo %i | thisUniverse %i | r %i | g %i | b %i\n", led, thisUniverse, data[i * 3], data[i * 3 + 1], data[i * 3 + 2]);
        }
        led++;
      }
    }
    else if (thisUniverse >= START_UNIVERSE_L) // this is the L strip on universe 7
    {
      int led = i * pixelFactor + ((thisUniverse - START_UNIVERSE_L) * 170) + leapLCounter; // for thisUniverse==7 ? led start at 0 : <nothing else>
      // if (led==509) Serial.printf("L-STRIP from %i to infinityyy! \tled%i/%i %u/%u-%i %u %u %i %i\n", START_UNIVERSE_L, led, NUM_LEDS_L, universe, maxUniverses, START_UNIVERSE_L, size, sequence, thisUniverse, sendFrame);

      // special treatment for the L strip because uses 585 leds, which is 75 longer than 3*170 (=510): add 1 extra LED every 2nd time
      int thisPixelFactor = pixelFactor;
      if (leapLNow == 2)
      {
        thisPixelFactor++;
        leapLNow = 0;
        leapLCounter++;
      }
      leapLNow++;
      for (int p = 0; p < thisPixelFactor; p++)
      {
        int deadSpaceLed = addDeadSpace(led);
        if (deadSpaceLed < NUM_LEDS_L)
        {
          leds_L[deadSpaceLed] = getColors(i, data);
          // Serial.printf("leds_L led %i => led incl. deadSpace %i | thisUniverse %i | r %i | g %i | b %i\n", led, addDeadSpace(led), thisUniverse, data[i * 3], data[i * 3 + 1], data[i * 3 + 2]);
        }
        led++;
      }
      // Serial.println("");
    }
  }

  FastLED.show();
}

int addDeadSpace(int led) {
  int deadSpace = 0;
  if (led > 19)
  {
    deadSpace = 5;
  }
  if (led > 45)
  {
    deadSpace += 6;
  }
  if (led > 65) //3
  {
    deadSpace += 5;
  }
  if (led > 91)
  {
    deadSpace += 6;
  }
  if (led > 111)
  {
    deadSpace += 5;
  }
  if (led > 138)  //6
  {
    deadSpace += 6;
  }
  if (led > 158)
  {
    deadSpace += 5;
  }
  if (led > 186)
  {
    deadSpace += 6;
  }
  if (led > 205)
  {
    deadSpace += 5;
  }
  if (led > 232)
  {
    deadSpace += 5;
  }
  if (led > 252)
  {
    deadSpace += 7;
  }
  // maximum deadSpace is 61
  return led + deadSpace;
}

  // Initialize Serial for debugging
void setup()
{
  Serial.println("This firmware is from the 'esp32_ethernet_platformIO' repo, 'artnetReceiver' branch.");

  assignMacAndIps();

  #if CONFIG == MODE_CIRCLE
    FastLED.addLeds<WS2812, PIN_LED_DATA, GRB>(leds_C, NUM_LEDS_C);
  #elif CONFIG == MODE_ARROW  
    FastLED.addLeds<WS2812, PIN_LED_DATA, GRB>(leds_A, NUM_LEDS_A);
  #elif CONFIG == MODE_LASERSCISSORS
    FastLED.addLeds<WS2812, PIN_LED_DATA, GRB>(leds_L, NUM_LEDS_L);
  #endif

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

  // Start Art-Net
  artnet.begin();

  // LED test and number display
  #if CONFIG == MODE_CIRCLE
    initTest(NUM_LEDS_C, leds_C);
  #elif CONFIG == MODE_ARROW
    initTest(NUM_LEDS_A, leds_A);
  #elif CONFIG == MODE_LASERSCISSORS
    initTest(NUM_LEDS_L, leds_L);
  #endif

  // if Artnet packet comes to this universe, forward them to fastled directly
  // artnet.forwardArtDmxDataToFastLED(universe1, leds, NUM_LEDS);
  // artnet.forwardArtDmxDataToFastLED(4, leds, NUM_LEDS);

  // // individual callback
  // thisUniverse = 1;
  // artnet.subscribeArtDmxUniverse(universe1, onDmxFrame);

  artnet.subscribeArtDmx(onDmxFrame);

  // if Artnet packet comes to this universe, this function (lambda) is called
  // artnet.subscribeArtDmxUniverse(1, [&](const uint8_t *data, uint16_t size, const ArtDmxMetadata &metadata, const ArtNetRemoteInfo &remote) {
  //     Serial.print("INLINE subscribeArtDmxUniverse: artnet data from ");
  //     Serial.print(remote.ip);
  //     Serial.print(":");
  //     Serial.print(remote.port);
  //     Serial.print(", universe = 1");
  //     Serial.print(", size = ");
  //     Serial.print(size);
  //     Serial.print(") :");
  //     for (size_t i = 0; i < size; ++i) {
  //         Serial.print(data[i]);
  //         Serial.print(",");
  //     }
  //     Serial.println();
  // });
}

void loop()
{
  artnet.parse();
  FastLED.show();
}
