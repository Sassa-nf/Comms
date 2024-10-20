#ifndef _LORA_CONFIG_H
#define _LORA_CONFIG_H

#include <Arduino.h>

struct Config
{
    String listen_on_serial0;
    String listen_on_usb;

    Config()
        : listen_on_serial0(String("none")),
          listen_on_usb("readline") {};

    static Config init();
};
#endif
