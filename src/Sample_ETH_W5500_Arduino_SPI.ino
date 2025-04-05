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

// LED settings for LED strips
const int NUM_LEDS_C = 507; // 507 leds_A in Circle
const int NUM_LEDS_A = 452; // 452 leds_A in Arrow
// const int NUM_LEDS_A = 200; // 452 leds_A in Arrow
// const int NUM_LEDS_L = 646; // 646 leds_A in Laser v3 + Scissors, 585 in use without deadSpace
const int NUM_LEDS_L = 627; // 646 leds_A in Laser v3 + Scissors, 585 in use without deadSpace
CRGB leds_C[NUM_LEDS_C];
CRGB leds_A[NUM_LEDS_A];
CRGB leds_L[NUM_LEDS_L];
struct LedStrip {
  CRGB* leds;
  int length;
};

const uint8_t PIN_LED_DATA = 22;
const int pixelFactor = 3; // number of pixels displaying the same information to save universes

// Art-Net / DMX settings
const int startUniverse = 0; // CHANGE FOR YOUR SETUP most software this is 1, some software send out artnet first universe as 0.
const int START_UNIVERSE_A = 4;
const int START_UNIVERSE_L = 7;

bool firstDmxFrameReceived = false;

LedStrip getCurrentStrip() {
  switch (config) {
      case MODE_CIRCLE:
          return {leds_C, NUM_LEDS_C};
      case MODE_ARROW:
          return {leds_A, NUM_LEDS_A};
      case MODE_LASERSCISSORS:
          return {leds_L, NUM_LEDS_L};
      default:
          return {nullptr, 0};
  }
}

void assignMacAndIps()
{
  if (config == MODE_CIRCLE)
  {
    mac[1] = 0xAD; // 173
    mac[2] = 0xBE; // 190
    mac[3] = 0xEF; // 239
    mac[4] = 0xFE; // 254
    mac[5] = 0xED; // 237
    local_IP = IPAddress(192, 168, 1, 24);
  }

  else if (config == MODE_ARROW)
  {
    mac[0] = 0xDE;
    mac[1] = 0xAD;
    mac[2] = 0xBE;
    mac[3] = 0xEF;
    mac[4] = 0xFE;
    mac[5] = 0xEE; // 238
    local_IP = IPAddress(192, 168, 1, 25);
  }
  else if (config == MODE_LASERSCISSORS)
  {
    mac[0] = 0xDE;
    mac[1] = 0xAD;
    mac[2] = 0xBE;
    mac[3] = 0xEF;
    mac[4] = 0xFE;
    mac[5] = 0xEF; // 239
    local_IP = IPAddress(192, 168, 1, 26);
  }
}

void testBlinkThree(CRGB blinkColor)
{
  LedStrip strip = getCurrentStrip();
  for (int r = 0; r < 3; r++)
  {
    for (int i = 0; i < strip.length; i++)
    {
      strip.leds[i] = blinkColor;
    }
    FastLED.show();
    delay(500);
    for (int i = 0; i < strip.length; i++)
    {
      strip.leds[i] = CRGB(0, 0, 0);
    }
    FastLED.show();
    delay(500);
  }
}

void initTest()
{
  Serial.printf("Init test %i\n", config);

  // Test blink three times with color depending on mode: Circle red, Arrow green, Laser + Scissors blue
  if (config == MODE_CIRCLE)
  {
    testBlinkThree(CRGB(127, 0, 0));
  }
  if (config == MODE_ARROW)
  {
    testBlinkThree(CRGB(0, 127, 0));
  }
  if (config == MODE_LASERSCISSORS)
  {
    testBlinkThree(CRGB(0, 0, 127));
  }

  // Default test
  LedStrip strip = getCurrentStrip();
  for (int i = 0; i < strip.length; i++)
  {
    strip.leds[i] = CRGB(127, 127, 127);
  }
  FastLED.show();
  delay(500);
  for (int i = 0; i < strip.length; i++)
  {
    strip.leds[i] = CRGB(127, 0, 0);
  }
  FastLED.show();
  delay(500);
  for (int i = 0; i < strip.length; i++)
  {
    strip.leds[i] = CRGB(0, 127, 0);
  }
  FastLED.show();
  delay(500);
  for (int i = 0; i < strip.length; i++)
  {
    strip.leds[i] = CRGB(0, 0, 127);
  }
  FastLED.show();
  delay(500);
  for (int i = 0; i < strip.length; i++)
  {
    strip.leds[i] = CRGB(0, 0, 0);
  }
  FastLED.show();
}

CRGB getColors(int i, const uint8_t *data)
{
  return CRGB(data[i * 3], data[i * 3 + 1], data[i * 3 + 2]);
}

void onDmxFrame(const uint8_t *data, uint16_t size, const ArtDmxMetadata &metadata, const ArtNetRemoteInfo &remote)
{

  if (config == Mode::MODE_CIRCLE && metadata.universe <= START_UNIVERSE_A) return;
  if (config == Mode::MODE_ARROW && (metadata.universe < START_UNIVERSE_A || metadata.universe >= START_UNIVERSE_L)) return;
  if (config == Mode::MODE_LASERSCISSORS && metadata.universe < START_UNIVERSE_L) return;

  // Serial.println(metadata.universe);
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

  LedStrip strip = getCurrentStrip();
  uint8_t thisUniverse = metadata.universe - startUniverse; // global setting might be changed for certain strip types
  int universalNumLeds = config == MODE_LASERSCISSORS ? NUM_LEDS_L : config == MODE_ARROW ? NUM_LEDS_A : NUM_LEDS_C;
  int universalShift = config == MODE_LASERSCISSORS ? START_UNIVERSE_L : config == MODE_ARROW ? START_UNIVERSE_A : 1;

  // TODO: this is introducing flickering issues with numbers higher than 1. why?
  int pixelOffset = config == MODE_LASERSCISSORS ? 5 : config == MODE_ARROW ? 5 : 5; // offset pushes all pixels to the right to exclude the 5-pixel failover strips

  // special treatment for L strip
  int leapLCounter = 0;
  int leapLNow = 0;
  
  // read universe and put into the right part of the display buffer
  for (int i = 0; i < strip.length / 3; i++)
  {
    int led = i * pixelFactor + ((thisUniverse - universalShift) * 170);
    led += pixelOffset;

    if (thisUniverse < START_UNIVERSE_L) // this is the C or A strip
    {
      for (int p = 0; p < pixelFactor; p++)
      {
        if (led < universalNumLeds)
        {
          strip.leds[led] = getColors(i, data);
        }
        led++;
      }
    }
    else if (thisUniverse >= START_UNIVERSE_L) // this is the L strip on universe 7
    {
      int led = i * pixelFactor + ((thisUniverse - START_UNIVERSE_L) * 170) + leapLCounter; // for thisUniverse==7 ? led start at 0 : <nothing else>
      led += pixelOffset;

      // special treatment for the L strip because uses 585 leds, which is 75 longer than 3*170 (=510): add 1 extra LED every 2nd time (646 leds_A in Laser v3 + Scissors, 585 in use without deadSpace)
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
        // special treatment for the L strip because it has ends that are not supposed to light up
        int deadSpaceLed = addDeadSpace(led);
        if (deadSpaceLed < NUM_LEDS_L)
        {
          strip.leds[deadSpaceLed] = getColors(i, data);
        }
        led++;
      }
    }
  }

  FastLED.show();
}

int addDeadSpace(int led)
{
  int deadSpace = 0;
  if (led > 19)
  {
    deadSpace = 5;
  }
  if (led > 45)
  {
    deadSpace += 6;
  }
  if (led > 65) // 3
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
  if (led > 138) // 6
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
  // Initialize Serial for debugging
  Serial.begin(115200);
  delay(1000); // Allow time for serial monitor to connect
  Serial.println("This firmware is from the 'esp32_ethernet_platformIO' repo, 'artnetReceiver' branch.");

  assignMacAndIps();

  if (config == MODE_CIRCLE)
  {
    FastLED.addLeds<WS2812, PIN_LED_DATA, GRB>(leds_C, NUM_LEDS_C);
  }
  else if (config == MODE_ARROW)
  {
    FastLED.addLeds<WS2812, PIN_LED_DATA, GRB>(leds_A, NUM_LEDS_A);
  }
  else if (config == MODE_LASERSCISSORS)
  {
    FastLED.addLeds<WS2812, PIN_LED_DATA, GRB>(leds_L, NUM_LEDS_L);
  }

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
  initTest();

  artnet.subscribeArtDmx(onDmxFrame);
}

void loop()
{
  artnet.parse();
  FastLED.show();
}
