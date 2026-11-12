#ifndef MOCK_ESPMDNS_H
#define MOCK_ESPMDNS_H

#include <cstdint>

class MockMDNS {
public:
    bool begin(const char* host) { return true; }
    void addService(const char* service, const char* proto, uint16_t port) {}
    void addServiceTxt(const char* service, const char* proto, const char* key, const char* value) {}
};

static MockMDNS MDNS;

#endif
