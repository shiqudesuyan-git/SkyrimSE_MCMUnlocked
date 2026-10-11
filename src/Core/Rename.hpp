#pragma once

#include "DataHandler.hpp"

#include "Core/Structure.h"
#include "Core/Main.hpp"
#include "Core/JSONHandler.hpp"
#include "Core/DialogSession.hpp"

#include "Utils/MiscUtils.hpp"

namespace ModCore
{
	class Rename
	{
	public:

		static void RenameStart()
		{
			const auto ticket = DialogSession::Begin(DialogSession::Kind::Rename);
			Main::GetModIDFromSelectedEntryAsync([ticket](std::string modID) {
				if (!DialogSession::Current(ticket, DialogSession::Kind::Rename)) return;
				if (modID.empty() || !MiscUtils::IsConfigPanelActive()) {
					DialogSession::Cancel(ticket);
					return;
				}

				auto* jsonEntry = JSONHandler::GetModEntryFromModID(modID);
				if (!jsonEntry) { DialogSession::Cancel(ticket); return; }

				MiscUtils::SetSearchWidgetDisabled(true);

				std::string dialogTitle = SettingsIni::SettingsManager().GetLanguageValue("RenameMenu", ModData::gameLanguage, "Rename: %s");
				if (size_t pos = dialogTitle.find("%s"); pos != std::string::npos) dialogTitle.replace(pos, 2, MiscUtils::GetSKSETranslation(jsonEntry->modID));

				OpenTextInputDialog(ticket, modID, dialogTitle, (jsonEntry->modName.empty() ? jsonEntry->modID : jsonEntry->modName));
			});
		}
		
		static void RenameDone(std::string newName)
		{
			auto request = DialogSession::Take(DialogSession::Kind::Rename);
			if (!request) return;
			const auto& modID = request->modID;

			if (newName.empty()) newName = MiscUtils::NormalizeModID(modID);

			Main::SetModNameFromModID(modID, newName);
			logger::info("Dialog fix: accepted owned rename for '{}'", modID);

			MiscUtils::SetSearchWidgetDisabled(false);

			Main::UpdateMenuModNames();
		}

	private:

		static void OpenTextInputDialog(std::uint64_t ticket, const std::string& modID, const std::string& titleText, const std::string& initialText = "")
		{
			// Caller already runs on the UI thread; avoid another delayed UI task.
			[&]() {
				auto panel = MiscUtils::GetConfigPanelGfx();
				if (!panel.IsObject()) {
					DialogSession::Cancel(ticket);
					MiscUtils::SetSearchWidgetDisabled(false);
					return;
				}

				RE::GFxValue titleVal;
				titleVal.SetString(titleText.c_str());
				panel.SetMember("_dialogTitleText", titleVal);

				RE::GFxValue args[1];
				args[0].SetString(initialText.c_str());
				if (!DialogSession::Open(ticket, DialogSession::Kind::Rename, { modID, {} }) ||
					!panel.Invoke("setInputDialogParams", nullptr, args, 1)) {
					DialogSession::Cancel(ticket);
					MiscUtils::SetSearchWidgetDisabled(false);
				}
			}();
		}
	};
}
