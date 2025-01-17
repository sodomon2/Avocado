#pragma once
#include <cassert>
#include <deque>
#include <memory>
#include "disc/disc.h"
#include "fifo.h"

struct System;

namespace device {
namespace cdrom {

class CDROM {
    union StatusCode {
        enum class Mode { None, Reading, Seeking, Playing };
        struct {
            uint8_t error : 1;
            uint8_t motor : 1;
            uint8_t seekError : 1;
            uint8_t idError : 1;
            uint8_t shellOpen : 1;
            uint8_t read : 1;
            uint8_t seek : 1;
            uint8_t play : 1;
        };
        uint8_t _reg;

        void setMode(Mode mode) {
            error = seekError = idError = false;
            read = seek = play = false;
            motor = true;
            if (mode == Mode::Reading) {
                read = true;
            } else if (mode == Mode::Seeking) {
                seek = true;
            } else if (mode == Mode::Playing) {
                play = true;
            }
        }

        void setShell(bool opened) {
            shellOpen = opened;
            if (!shellOpen) {
                setMode(Mode::None);
            }
        }

        bool getShell() const { return shellOpen; }

        void toggleShell() {
            if (!shellOpen) {
                shellOpen = true;
                setMode(Mode::None);
            } else {
                shellOpen = false;
            }
        }

        StatusCode() : _reg(0) { shellOpen = true; }
    };

    union CDROM_Status {
        struct {
            uint8_t index : 2;
            uint8_t xaFifoEmpty : 1;
            uint8_t parameterFifoEmpty : 1;  // triggered before writing first byte
            uint8_t parameterFifoFull : 1;   // triggered after writing 16 bytes
            uint8_t responseFifoEmpty : 1;   // triggered after reading last byte
            uint8_t dataFifoEmpty : 1;       // triggered after reading last byte
            uint8_t transmissionBusy : 1;
        };
        uint8_t _reg;

        CDROM_Status() : _reg(0x18) {}
    };

    union Mode {
        struct {
            uint8_t cddaEnable : 1;     // 0 - off, 1 - on
            uint8_t cddaAutoPause : 1;  // Pause upon End of track (CDDA)
            uint8_t cddaReport : 1;     // Report CDDA report interrupts (like cmdGetlocP)
            uint8_t xaFilter : 1;       // Match XA sectors matching Setfilter
            uint8_t ignoreBit : 1;      // dunno, something sector size related
            uint8_t sectorSize : 1;     // 0 - 0x800, 1 - 0x924
            uint8_t xaEnabled : 1;      // 0 - off, 1 - on
            uint8_t speed : 1;          // 0 - normal (75 sectors / second), 1 - double speed
        };
        uint8_t _reg;

        Mode() : _reg(0) {}
    };

    struct Filter {
        uint8_t file;
        uint8_t channel;

        Filter() : file(0), channel(0) {}
    };

    int verbose = 1;

    CDROM_Status status;
    uint8_t interruptEnable = 0;
    fifo<16, uint8_t> CDROM_params;
    fifo<16, uint8_t> CDROM_response;
    fifo<16, uint8_t> interruptQueue;

    Mode mode;
    Filter filter;

    System* sys;
    int readSector = 0;
    int seekSector = 0;

    StatusCode stat;

    void cmdGetstat();
    void cmdSetloc();
    void cmdPlay();
    void cmdReadN();
    void cmdMotorOn();
    void cmdStop();
    void cmdPause();
    void cmdInit();
    void cmdMute();
    void cmdDemute();
    void cmdSetFilter();
    void cmdSetmode();
    void cmdGetlocL();
    void cmdGetlocP();
    void cmdGetTN();
    void cmdGetTD();
    void cmdSeekL();
    void cmdTest();
    void cmdGetId();
    void cmdReadS();
    void cmdReadTOC();
    void cmdUnlock();
    void cmdSetSession();
    void cmdSeekP();
    void handleCommand(uint8_t cmd);

    void writeResponse(uint8_t byte) {
        if (CDROM_response.full()) {
            return;
        }
        CDROM_response.add(byte);
        status.responseFifoEmpty = 1;
    }

    uint8_t readParam() {
        uint8_t param = CDROM_params.get();

        status.parameterFifoEmpty = CDROM_params.empty();
        status.parameterFifoFull = 1;

        return param;
    }

    void postInterrupt(int irq) {
        assert(irq <= 7);

        interruptQueue.add(irq);
    }

    std::string dumpFifo(const fifo<16, uint8_t> f);

   public:
    uint8_t volumeLeftToLeft = 0;
    uint8_t volumeLeftToRight = 0;
    uint8_t volumeRightToLeft = 0;
    uint8_t volumeRightToRight = 0;
    std::pair<std::deque<int16_t>, std::deque<int16_t>> audio;
    std::vector<uint8_t> rawSector;

    std::vector<uint8_t> dataBuffer;
    int dataBufferPointer;  // for DMA

    bool isBufferEmpty();
    uint8_t readByte();

    disc::TrackType trackType;
    std::unique_ptr<disc::Disc> disc;
    bool mute = false;

    CDROM(System* sys);
    void step();
    uint8_t read(uint32_t address);
    void write(uint32_t address, uint8_t data);

    void setShell(bool opened) { stat.setShell(opened); }
    bool getShell() const { return stat.getShell(); }
    void toggleShell() { stat.toggleShell(); }
    void ackMoreData() {
        postInterrupt(1);
        writeResponse(stat._reg);
    }
};
}  // namespace cdrom
}  // namespace device
