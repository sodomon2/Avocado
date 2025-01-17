#pragma once
#include <array>
#include "device/device.h"
#include "noise.h"
#include "regs.h"
#include "voice.h"

struct System;

namespace device::cdrom {
class CDROM;
}

namespace spu {
struct SPU {
    static const uint32_t BASE_ADDRESS = 0x1f801c00;
    static const int VOICE_COUNT = 24;
    static const int RAM_SIZE = 1024 * 512;
    static const size_t AUDIO_BUFFER_SIZE = 28 * 2 * 4;

    std::array<Voice, VOICE_COUNT> voices;

    Volume mainVolume;
    Volume cdVolume;
    Volume extVolume;
    Volume reverbVolume;

    Reg16 irqAddress;
    Reg16 dataAddress;
    uint32_t currentDataAddress;
    DataTransferControl dataTransferControl;
    Control control;
    Reg16 SPUSTAT;

    uint32_t captureBufferIndex;

    Noise noise;

    Reg32 _keyOn;
    Reg32 _keyOff;

    std::array<uint8_t, RAM_SIZE> ram;

    bool forceReverbOff = false;           // Debug use
    bool forceInterpolationOff = false;    // Debug use
    bool forcePitchModulationOff = false;  // Debug use
    Reg16 reverbBase;
    std::array<Reg16, 32> reverbRegisters;
    uint32_t reverbCurrentAddress;

    bool bufferReady = false;
    size_t audioBufferPos;
    std::array<int16_t, AUDIO_BUFFER_SIZE> audioBuffer;

    System* sys;

    uint8_t readVoice(uint32_t address) const;
    void writeVoice(uint32_t address, uint8_t data);

    SPU(System* sys);
    void step(device::cdrom::CDROM* cdrom);
    uint8_t read(uint32_t address);
    void write(uint32_t address, uint8_t data);

    void memoryWrite8(uint32_t address, uint8_t data);
    void memoryWrite16(uint32_t address, uint16_t data);

    void dumpRam();
};
}  // namespace spu