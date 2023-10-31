#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include "IUMDPorts.h"
#include "UMDPortsV3.h"
#include <string>

class Cartridge {
    public:

        Cartridge();
        virtual ~Cartridge();

        const String SystemName = "Default";

        virtual uint8_t readByte(uint16_t address);
        virtual uint8_t readByte(uint32_t address);

    protected:
        const uint16_t addressSetupTime = 100;
        const uint16_t readHoldTime = 200;
        static IUMDPorts *_umdPorts;

};

#endif