#include <stdio.h>
#include <stdint.h>
#include "../Firmware/src/BlindFetch.h"
#include "../Firmware/lib/OPAQUE/NetworkedOPAQUE.h"
#include "../Firmware/lib/net/NetUtils.h"


const char* SERVER_URL = "https://xxx.xxx.xxx.xxx:xxxx";
const char* OPAQUE_PASSWORD = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
const char* FIRMWARE_KEY = "yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy";

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