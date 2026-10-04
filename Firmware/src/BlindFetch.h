#ifndef BLIND_FETCH_H
#define BLIND_FETCH_H
#include <opaque.h>
#include <oprf/oprf.h>

void BlindDownloadFirmware(const char* downloadServerURL, const char* deviceFirmwareKey, const char* username, const uint8_t skClient[OPAQUE_SHARED_SECRETBYTES]);
void OTAHeaderGeneration(const char* downloadServerURL, const char* deviceFirmwareKey, const char* username, const uint8_t skClient[OPAQUE_SHARED_SECRETBYTES]);

#ifdef PlainOTA
uint64_t PlainOTADownload(const char* downloadURL);
#endif

#ifdef ReportUsage
void SendMetricsToServer(const char* reportServerURL, size_t peakHeap, size_t minHeap, int64_t startTime, int64_t endTime, const char* firmwareName);
#endif

#endif