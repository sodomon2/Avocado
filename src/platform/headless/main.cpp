#include <cstdio>
#include <string>
#include <memory>
#include <cassert>
#include <thread>
#include <uWS.h>
#include <json.hpp>
#include "utils/file.h"
#include "utils/string.h"
#include "mips.h"

using json = nlohmann::json;

bool emulateGpuCycles(std::unique_ptr<mips::CPU> &cpu, std::unique_ptr<device::gpu::GPU> &gpu, int cycles) {
    static int gpuLine = 0;
    static int gpuDot = 0;
    static bool gpuOdd = false;

    gpuDot += cycles;

    int newLines = gpuDot / 3413;
    if (newLines == 0) return false;

    gpuDot %= 3413;
    gpuLine += newLines;
	if (gpuLine == 264) gpuLine = 0;

	if (gpuLine < 243) {
		gpu->odd = gpu->frames % 2;
	} else {
		gpu->odd = false;
	}
    
    gpu->step();
	if (gpuLine == 243) {
        cpu->interrupt->IRQ(0);
    }
    if (gpuLine >= 263) {
        gpu->frames++;
        return true;
	}
    return false;
}

void emulateFrame(std::unique_ptr<mips::CPU> &cpu, std::unique_ptr<device::gpu::GPU> &gpu) {
    for (;;) {
        cpu->cdrom->step();
		
		cpu->timer0->step(110);
		cpu->timer1->step(110);
	    cpu->timer2->step(110);

        if (!cpu->executeInstructions(70)) {
            printf("CPU Halted\n");
            return;
        }

        if (emulateGpuCycles(cpu, gpu, 110)) {
            return;  // frame emulated
        }
    }
}

int main(int argc, char **argv) {
    std::unique_ptr<mips::CPU> cpu = std::make_unique<mips::CPU>();
	uWS::Hub h;

    h.onMessage([&cpu](uWS::WebSocket<uWS::SERVER> *ws, char *message, size_t length, uWS::OpCode opCode) {
		json regs;
		for (int i = 0; i<32; i++) {
			regs.push_back(cpu->reg[i]);
		}
		json j;
		j["cpu"]["reg"] = regs;
		j["cpu"]["pc"] = cpu->PC;
		j["cpu"]["lo"] = cpu->lo;
		j["cpu"]["hi"] = cpu->hi;

		std::string raw = j.dump(4);
        ws->send(raw.c_str(), raw.length(), opCode);
    });

	h.listen(3000);

    auto _bios = getFileContents("data/bios/SCPH1001.BIN");  // DTLH3000.BIN BOOTS
    if (_bios.empty()) {
        printf("Cannot open BIOS");
        return 1;
    }

    assert(_bios.size() == 512 * 1024);
    std::copy(_bios.begin(), _bios.end(), cpu->bios);
    cpu->state = mips::CPU::State::run;

    auto gpu = std::make_unique<device::gpu::GPU>();
    cpu->setGPU(gpu.get());

	std::thread serverThread([&h](){
		h.run();
	});
	serverThread.detach();

    for (;;) {
        if (cpu->state != mips::CPU::State::run) break;

        emulateFrame(cpu, gpu);
    }
    return 0;
}
