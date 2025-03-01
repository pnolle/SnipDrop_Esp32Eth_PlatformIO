# ESP32 DevBoard + W5500 ethernet shield 



# SnipDrop
Code for my LED rollup banner project 'SnipDrop'.
http://snippetupperlaser.com

## How it works
* DAW sends out MIDI
* Qlc+ receives MIDI and has mappings to a bunch of functions, matrices etc.
* 3 ESP32 DevBoard units are connected via Ethernet to the computer running Qlc+.
* Qlc+ broadcasts all Artnet universes to the network.
* ESPs filter universes according to their firmware configuration.
* ESPs use FastLED to [get alight](https://i.ytimg.com/vi/rTCefc-uuEw/maxresdefault.jpg).


## Firmware configuration

These values are set according to the selected firmware configuration:

|Config|IP|MAC|NUM_LEDS|
|-|-|-|-|
|MODE_CIRCLE|192.168.1.24|222.173.190.239.254.237|5074|
|MODE_ARROW|192.168.1.25|222.173.190.239.254.238|452|
|MODE_LASERSCISSORS|192.168.1.26|222.173.190.239.254.239|627|

## Libraries
Main libs involved are
* ``FastLED`` for LED control
* ``ArtnetEther`` for network communication
Details see ``platformio.ini``.
