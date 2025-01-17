#include "timers.h"
#include <imgui.h>
#include <numeric>
#include "platform/windows/gui/tools.h"
#include "system.h"
#include "utils/string.h"

namespace gui::debug::timers {

std::string getSyncMode(Timer* timer) {
    int which = timer->which;
    timer::CounterMode mode = timer->mode;

    if (which == 0) {
        using modes = timer::CounterMode::SyncMode0;
        auto mode0 = static_cast<modes>(mode.syncMode);

        switch (mode0) {
            case modes::pauseDuringHblank: return "pauseDuringHblank";
            case modes::resetAtHblank: return "resetAtHblank";
            case modes::resetAtHblankAndPauseOutside: return "resetAtHblankAndPauseOutside";
            case modes::pauseUntilHblankAndFreerun: return "pauseUntilHblankAndFreerun";
        }
    }
    if (which == 1) {
        using modes = timer::CounterMode::SyncMode1;
        auto mode1 = static_cast<modes>(mode.syncMode);

        switch (mode1) {
            case modes::pauseDuringVblank: return "pauseDuringVblank";
            case modes::resetAtVblank: return "resetAtVblank";
            case modes::resetAtVblankAndPauseOutside: return "resetAtVblankAndPauseOutside";
            case modes::pauseUntilVblankAndFreerun: return "pauseUntilVblankAndFreerun";
        }
    }
    if (which == 2) {
        using modes = timer::CounterMode::SyncMode2;
        auto mode2 = static_cast<modes>(mode.syncMode);

        switch (mode2) {
            case modes::stopCounter:
            case modes::stopCounter_: return "stopCounter";
            case modes::freeRun:
            case modes::freeRun_: return "freeRun";
        }
    }
    return "";
}

std::string getClockSource(Timer* timer) {
    int which = timer->which;
    timer::CounterMode mode = timer->mode;
    if (which == 0) {
        using modes = timer::CounterMode::ClockSource0;
        auto clock = static_cast<modes>(mode.clockSource & 1);

        if (clock == modes::dotClock) {
            return "Dotclock";
        } else {
            return "Sysclock";
        }
    } else if (which == 1) {
        using modes = timer::CounterMode::ClockSource1;
        auto clock = static_cast<modes>(mode.clockSource & 1);

        if (clock == modes::hblank) {
            return "HBlank";
        } else {
            return "Sysclock";
        }
    } else if (which == 2) {
        using modes = timer::CounterMode::ClockSource2;
        auto clock = static_cast<modes>((mode.clockSource >> 1) & 1);

        if (clock == modes::systemClock_8) {
            return "Sysclock/8";
        } else {
            return "Sysclock";
        }
    }
    return "";
}

struct Field {
    std::string bits;
    std::string name;
    std::string value;
};

void drawRegisterFields(const char* name, const std::vector<Field>& fields) {
    const int colNum = 3;
    ImVec2 charSize = ImGui::CalcTextSize("_");
    std::vector<int> columnsWidth(colNum);

    for (int i = 0; i < fields.size(); i++) {
        auto& f = fields[i];
        if (f.bits.size() > columnsWidth[0]) {
            columnsWidth[0] = f.bits.size();
        }
        if (f.name.size() > columnsWidth[1]) {
            columnsWidth[1] = f.name.size();
        }
        if (f.value.size() > columnsWidth[2]) {
            columnsWidth[2] = f.value.size();
        }
    }

    ImVec2 padding(2, 2);
    ImVec2 spacing(4, 0);

    int width = 0;
    for (auto w : columnsWidth) {
        width += w * charSize.x + spacing.x;
    }
    ImVec2 windowSize(width, fields.size() * charSize.y + padding.y * 2);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, padding);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, spacing);
    if (ImGui::BeginChild(name, windowSize, true)) {
        ImGui::Columns(3, name, true);

        for (int i = 0; i < colNum; i++) {
            ImGui::SetColumnWidth(i, columnsWidth[i] * charSize.x + spacing.x);
        }

        for (auto& f : fields) {
            ImGui::TextUnformatted(f.bits.c_str());
            ImGui::NextColumn();

            ImGui::TextUnformatted(f.name.c_str());
            ImGui::NextColumn();

            ImGui::TextUnformatted(f.value.c_str());
            ImGui::NextColumn();
        }

        ImGui::Columns(1);
    }
    ImGui::EndChild();
    ImGui::PopStyleVar(3);
}

void Timers::timersWindow(System* sys) {
    ImGui::Begin("Timers", &timersWindowOpen, ImVec2(600, 300), ImGuiWindowFlags_NoScrollbar);

    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);

    std::array<Timer*, 3> timers = {sys->timer[0].get(), sys->timer[1].get(), sys->timer[2].get()};

    ImGui::Columns(3);
    for (auto* timer : timers) {
        int which = timer->which;
        ImGui::Text("Timer%d", which);
        ImGui::Text("Value:  0x%04x", timer->current._reg);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("0x%08x", timer->baseAddress + which * 0x10 + 0);
        }

        ImGui::Text("Target: 0x%04x", timer->target._reg);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("0x%08x", timer->baseAddress + which * 0x10 + 8);
        }

        ImGui::Text("Mode:   0x%04x", timer->mode._reg);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("0x%08x", timer->baseAddress + which * 0x10 + 4);
        }

        const std::vector<Field> fields = {
            {"0", "Sync", timer->mode.syncEnabled ? "On" : "Off"},
            {"1-2", "SyncMode", getSyncMode(timer)},
            {"3", "Reset counter", timer->mode.resetToZero == timer::CounterMode::ResetToZero::whenFFFF ? "after 0xffff" : "after target"},
            {"4", "IRQ on target", timer->mode.irqWhenTarget ? "On" : "Off"},
            {"5", "IRQ on 0xFFFF", timer->mode.irqWhenFFFF ? "On" : "Off"},
            {"6", "IRQ repeat mode",
             timer->mode.irqRepeatMode == timer::CounterMode::IrqRepeatMode::repeatedly ? "Repeatedly" : "One-shot"},
            {"7", "IRQ pulse mode", timer->mode.irqPulseMode == timer::CounterMode::IrqPulseMode::toggle ? "Toggle bit10" : "Pulse bit10"},
            {"8-9", "Clock source", getClockSource(timer)},
            {"10", "IRQ request", timer->mode.interruptRequest ? "No" : "Pending"},
            {"11", "Reached target", timer->mode.reachedTarget ? "Yes" : "No"},
            {"12", "Reached 0xFFFF", timer->mode.reachedFFFF ? "Yes" : "No"},
        };

        drawRegisterFields(string_format("flags##timer%d", which).c_str(), fields);

        ImGui::NextColumn();
    }

    ImGui::Columns(1);

    ImGui::PopFont();

    ImGui::End();
}

void Timers::displayWindows(System* sys) {
    if (timersWindowOpen) timersWindow(sys);
}
}  // namespace gui::debug::timers