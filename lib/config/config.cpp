#include "config.h"

Config Config::init()
{
    Serial.println("No SD card found, will assume defaults");
    return Config();
}
