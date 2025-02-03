# ESP32 DevBoard + W5500 ethernet shield 

## Simple UDP communication

ESP32 creates Ethernet instance and listens for UDP messages on port 8888.

Send a message from a console window from a connected client by typing into the terminal:
```
nc -u 192.168.1.22 8888
```
... and then start typing your messages :)
