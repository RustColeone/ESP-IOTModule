#ifndef APP_NETWORK_H
#define APP_NETWORK_H

#include "config.h"

void connectWiFi();
void updateTime();
void startDeviceDiscovery();
void handleDeviceDiscovery();
String getDeviceId();
String getDeviceHostname();

#endif
