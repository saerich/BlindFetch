#include <stdio.h>
#include <stdint.h>
#include "../Firmware/src/BlindFetch.h"
#include "../Firmware/lib/OPAQUE/NetworkedOPAQUE.h"
#include "../Firmware/lib/net/NetUtils.h"


const char* SERVER_URL = "https://192.168.1.208:5000";
const char* OPAQUE_PASSWORD = ")u102osahAFNOQAWhgwpuA*(DCHg)";
const char* FIRMWARE_KEY = "63F92A1568F52C12FC59914E9FC71C731767609014B159D696E939A6557DB2C1";

int main(void)
{
    char mac[18];
    GetDeviceMACAsString(mac);
    uint8_t skClient[64];
    uint8_t exportKey[64];
    OPAQUEInit(mac);
    NetworkedOPAQUERegister(SERVER_URL, mac, OPAQUE_PASSWORD);
    NetworkedOPAQUELogin(SERVER_URL, mac, OPAQUE_PASSWORD, skClient, exportKey);
    BlindDownloadFirmware(SERVER_URL, FIRMWARE_KEY, mac, skClient);


    return 0;
}