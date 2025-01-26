#include <ETH.h>
#include <SPI.h>

#ifndef ETH_PHY_CS
#define ETH_PHY_TYPE ETH_PHY_W5500
#define ETH_PHY_ADDR 0
#define ETH_PHY_CS   15
#define ETH_PHY_IRQ  -1 // 4
#define ETH_PHY_RST  5
#endif

#define ETH_SPI_SCK  14
#define ETH_SPI_MISO 12
#define ETH_SPI_MOSI 13

static bool eth_connected = false;

void onEvent(arduino_event_id_t event, arduino_event_info_t info) {
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      Serial.println("ETH Started");
      ETH.setHostname("esp32-eth0");
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      Serial.println("ETH Connected");
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      Serial.printf("ETH Got IP: '%s'\n", esp_netif_get_desc(info.got_ip.esp_netif));
      eth_connected = true;
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
    case ARDUINO_EVENT_ETH_STOP:
      Serial.println("ETH Disconnected/Stopped");
      eth_connected = false;
      break;
    default:
      break;
  }
}

void testClient(const char *host, uint16_t port) {
  Serial.print("\nconnecting to ");
  Serial.println(host);

  EthernetClient client;  // Use EthernetClient
  if (!client.connect(host, port)) {
    Serial.println("connection failed");
    return;
  }

  client.printf("GET / HTTP/1.1\r\nHost: %s\r\n\r\n", host);
  while (client.connected() && !client.available());
  while (client.available()) {
    Serial.write(client.read());
  }

  Serial.println("closing connection\n");
  client.stop();
}

void setup() {
  Serial.begin(115200);

  SPI.begin(ETH_SPI_SCK, ETH_SPI_MISO, ETH_SPI_MOSI);
  ETH.begin(ETH_PHY_TYPE, ETH_PHY_ADDR, ETH_PHY_CS, ETH_PHY_IRQ, ETH_PHY_RST, SPI);
  ETH.onEvent(onEvent);
}

void loop() {
  if (eth_connected) {
    testClient("google.com", 80);
  }
  delay(10000);
}
