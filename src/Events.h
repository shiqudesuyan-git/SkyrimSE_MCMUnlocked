#pragma once

#include "DataHandler.hpp"

#include "Core/Rename.hpp"
#include "Core/Reorder.hpp"
#include "Core/Reset.hpp"

namespace Events
{
	class ModEventSink :
		public RE::BSTEventSink<SKSE::ModCallbackEvent>,
		public RE::BSTEventSink<RE::MenuOpenCloseEvent>
	{
		ModEventSink() = default;
		ModEventSink(const ModEventSink&) = delete;
		ModEventSink(ModEventSink&&) = delete;
		ModEventSink& operator=(const ModEventSink&) = delete;
		ModEventSink& operator=(ModEventSink&&) = delete;

	public:
		#define continueEvent RE::BSEventNotifyControl::kContinue

		static inline bool postLoadEventsLoaded = false;

		static ModEventSink* GetSingleton()
		{
			static ModEventSink singleton;
			return &singleton;
		}

		static void LoadEvents()
		{
			auto* eventSink = GetSingleton();

			SKSE::GetModCallbackEventSource()->AddEventSink(eventSink);
			RE::UI::GetSingleton()->AddEventSink<RE::MenuOpenCloseEvent>(eventSink);

			auto& trampoline = SKSE::GetTrampoline();
			SKSE::AllocTrampoline(1 << 8);

			REL::Relocation<uintptr_t> hook{ REL::RelocationID(67315, 68617) };
			_processInputHook = trampoline.write_call<5>(hook.address() + REL::Relocate(0x7B, 0x7B, 0x81), ProcessInputHookTemplate);
			logger::info("ProcessInputHook hooked at address: 0x{:X}", _processInputHook.address());
		}

		RE::BSEventNotifyControl ProcessEvent(const SKSE::ModCallbackEvent* event, RE::BSTEventSource<SKSE::ModCallbackEvent>*) override;
		RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override;

		static void ProcessInputHookTemplate(RE::BSTEventSource<RE::InputEvent*>* dispatcher,RE::InputEvent* const* eventList);
		inline static REL::Relocation<decltype(ProcessInputHookTemplate)> _processInputHook;
	};
}
