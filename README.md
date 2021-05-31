# ESPAsyncRTSPServer-esphome

Async RTSP Server for ESP32 Arduino (no ESP8266 - yet)

It requires AsyncTCP to work To use this library you might need to have the latest git versions of ESP32 Arduino Core

This library's design is heavily based off of [Geeksville's Micro-RTSP server](https://github.com/geeksville/Micro-RTSP),  [Medialan's RTSP/RTP Streaming Server Hello World turtorial](https://www.medialan.de/usecase0001.html), and [me-no-dev's ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer).  

This library relies on [me-no-dev's ESPAsyncTCP](https://github.com/me-no-dev/ESPAsyncTCP) libray to handle incoming TCP requests.

#  Installation

## Using PlatformIO

PlatformIO is an open source ecosystem for IoT development with cross platform build system, library manager and full support for Espressif ESP8266/ESP32 development. It works on the popular host OS: Mac OS X, Windows, Linux 32/64, Linux ARM (like Raspberry Pi, BeagleBone, CubieBoard).

*  Install [PlatformIO IDE](http://platformio.org/platformio-ide)
*  Create new project using "PlatformIO Home > New Project"
*  Update dev/platform to staging version:
    [Instruction for Espressif 32](http://docs.platformio.org/en/latest/platforms/espressif32.html#using-arduino-framework-with-staging-version)
*  Add "ESP Async WebServer" to project using [Project Configuration File](http://docs.platformio.org/page/projectconf.html) platformio.ini and [lib_deps](http://docs.platformio.org/page/projectconf/section_env_library.html#lib-deps) option:

```
[env:myboard]
platform = espressif...
board = ...
framework = arduino

# using the latest stable version
lib_deps = ESPAsyncRTSPServer-esphome

# or using GIT Url (the latest development version)
lib_deps = https://github.com/crossan007/ESPAsyncRTSPServer-esphome.git
```

# Principles of operation

  There are two main processes responsible for handling RTSP connections / clients.  

## AsyncRTSPServer
This process listens for incoming TCP connections and instantiates a new instance of `AsyncRTSPClient` for each new connection.  This process also exposes a method for consumers to pass new images to be streamed to all connected clients.  This process does not process any RTSP commands

## AsyncRTSPClient
This process receives and parses all RTSP requests from connected clients.  It manages the streaming state per-client.  It receives RTP buffers from the `AsyncRTSPServer` class and forwards the buffers out to the related client.

