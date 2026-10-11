#pragma once

#include "DataHandler.hpp"
#include "Core/Structure.h"
#include "Core/Main.hpp"
#include "Core/DialogSession.hpp"
#include "Utils/MiscUtils.hpp"

namespace ModCore
{
    class Reorder
    {
    public:
        static void ReorderStart()
        {
            const auto ticket = DialogSession::Begin(DialogSession::Kind::Reorder);
            Main::GetModIDFromSelectedEntryAsync([ticket](std::string modID) {
                if (!DialogSession::Current(ticket, DialogSession::Kind::Reorder)) return;
                if (modID.empty() || !MiscUtils::IsConfigPanelActive()) {
                    DialogSession::Cancel(ticket);
                    return;
                }
                std::vector<MCMEntry> snapshot;
                {
                    std::shared_lock lock(g_mcmStructureMutex);
                    snapshot = mcmStructure;
                }
                const auto it = std::find_if(snapshot.begin(), snapshot.end(), [&](const MCMEntry& e) { return e.modID == modID; });
                if (it == snapshot.end()) { DialogSession::Cancel(ticket); return; }
                const auto currentEntry = *it;
                if (currentEntry.disabled) {
                    {
                        std::unique_lock lock(g_mcmStructureMutex);
                        auto target = std::find_if(mcmStructure.begin(), mcmStructure.end(), [&](const MCMEntry& e) { return e.modID == modID; });
                        if (target != mcmStructure.end()) target->disabled = false;
                    }
                    DialogSession::Cancel(ticket);
                    Main::UpdateMenuModNames();
                    return;
                }
                std::sort(snapshot.begin(), snapshot.end(), [](const MCMEntry& a, const MCMEntry& b) { return a.modPosition < b.modPosition; });
                const std::string title = currentEntry.modName.empty() ? modID : currentEntry.modName;
                std::vector<std::string> mapping(3);
                std::vector<std::string> options{
                    SettingsIni::SettingsManager().GetLanguageValue("AutoSort", ModData::gameLanguage, "+ Auto sort"),
                    SettingsIni::SettingsManager().GetLanguageValue("DisableMenu", ModData::gameLanguage, "+ Disable menu"),
                    SettingsIni::SettingsManager().GetLanguageValue("PlaceFirst", ModData::gameLanguage, "> Place first")
                };
                const auto placeAfter = SettingsIni::SettingsManager().GetLanguageValue("PlaceAfter", ModData::gameLanguage, "> Place after: %s");
                for (const auto& entry : snapshot) {
                    if (entry.modID == modID) continue;
                    auto label = placeAfter;
                    const auto name = MiscUtils::GetSKSETranslation(entry.modName.empty() ? entry.modID : entry.modName);
                    if (const auto pos = label.find("%s"); pos != std::string::npos) label.replace(pos, 2, name);
                    options.push_back(std::move(label));
                    mapping.push_back(entry.modID);
                }
                auto panel = MiscUtils::GetConfigPanelGfx();
                if (!panel.IsObject()) { DialogSession::Cancel(ticket); return; }
                std::vector<RE::GFxValue> labelArgs;
                labelArgs.reserve(options.size());
                for (const auto& option : options) {
                    RE::GFxValue value;
                    value.SetString(option.c_str());
                    labelArgs.push_back(value);
                }
                RE::GFxValue titleValue;
                titleValue.SetString(title.c_str());
                panel.SetMember("_dialogTitleText", titleValue);
                if (!panel.Invoke("setMenuDialogOptions", nullptr, labelArgs.data(), static_cast<unsigned int>(labelArgs.size()))) {
                    DialogSession::Cancel(ticket);
                    return;
                }
                RE::GFxValue args[2];
                args[0].SetNumber(0);
                args[1].SetNumber(0);
                MiscUtils::SetSearchWidgetDisabled(true);
                if (!DialogSession::Open(ticket, DialogSession::Kind::Reorder, { modID, std::move(mapping) }) ||
                    !panel.Invoke("setMenuDialogParams", nullptr, args, 2)) {
                    DialogSession::Cancel(ticket);
                    MiscUtils::SetSearchWidgetDisabled(false);
                }
            });
        }

        static void ReorderDone(int selectedIndex)
        {
            auto request = DialogSession::Take(DialogSession::Kind::Reorder);
            if (!request) return;
            MiscUtils::SetSearchWidgetDisabled(false);
            if (selectedIndex < 0 || selectedIndex >= static_cast<int>(request->mapping.size())) return;
            const auto& modID = request->modID;
            {
                std::unique_lock lock(g_mcmStructureMutex);
                auto it = std::find_if(mcmStructure.begin(), mcmStructure.end(), [&](const MCMEntry& e) { return e.modID == modID; });
                if (it == mcmStructure.end()) return;
                auto& currentEntry = *it;
                if (selectedIndex == 0) {
                    currentEntry.modPosition = -1;
                } else if (selectedIndex == 1) {
                    currentEntry.disabled = !currentEntry.disabled;
                } else if (selectedIndex == 2) {
                    for (auto& entry : mcmStructure) {
                        if (entry.modID != modID && entry.modPosition >= 0) ++entry.modPosition;
                    }
                    currentEntry.modPosition = 0;
                } else {
                    const auto& afterModID = request->mapping[selectedIndex];
                    const auto after = std::find_if(mcmStructure.begin(), mcmStructure.end(), [&](const MCMEntry& e) { return e.modID == afterModID; });
                    if (after == mcmStructure.end()) return;
                    const int position = after->modPosition;
                    for (auto& entry : mcmStructure) {
                        if (entry.modID != modID && entry.modPosition > position) ++entry.modPosition;
                    }
                    currentEntry.modPosition = position + 1;
                }
            }
            logger::info("Dialog fix: accepted owned reorder for '{}' (index {})", modID, selectedIndex);
            Main::UpdateMenuModNames();
        }
    };
}
