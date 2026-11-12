#ifndef MOCK_ESPMDNS_H
#define MOCK_ESPMDNS_H

#include <cstdint>

class MockMDNS {
public:
    bool begin(const char*) { return true; }
    void addService(const char*, const char*, uint16_t) {}
    void addServiceTxt(const char*, const char*, const char*, const char*) {}
};

static MockMDNS MDNS __attribute__((unused));

#endif
