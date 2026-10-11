#pragma once

#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace ModCore
{
    // One-shot ownership of the dialog opened by Unlocked. Regular MCM dialogs
    // use the same SKICP events and must never consume an old request.
    class DialogSession
    {
    public:
        enum class Kind { None, Rename, Reorder };
        struct Request
        {
            std::string modID;
            std::vector<std::string> mapping;
        };

        static std::uint64_t Begin(Kind kind)
        {
            std::lock_guard lock(mutex);
            ++generation;
            pendingKind = kind;
            opened = false;
            request = {};
            return generation;
        }

        static bool Current(std::uint64_t ticket, Kind kind)
        {
            std::lock_guard lock(mutex);
            return ticket == generation && pendingKind == kind;
        }

        static bool Open(std::uint64_t ticket, Kind kind, Request value)
        {
            std::lock_guard lock(mutex);
            if (ticket != generation || pendingKind != kind || value.modID.empty()) return false;
            request = std::move(value);
            opened = true;
            return true;
        }

        static std::optional<Request> Take(Kind kind)
        {
            std::lock_guard lock(mutex);
            if (!opened || pendingKind != kind) return std::nullopt;
            auto result = std::move(request);
            Clear(); // Clear before applying the result or refreshing the UI.
            return result;
        }

        static bool Cancel()
        {
            std::lock_guard lock(mutex);
            const bool wasOpened = opened;
            Clear();
            return wasOpened;
        }

        static void Cancel(std::uint64_t ticket)
        {
            std::lock_guard lock(mutex);
            if (ticket == generation) Clear();
        }

    private:
        static void Clear()
        {
            ++generation;
            pendingKind = Kind::None;
            opened = false;
            request = {};
        }
        static inline std::mutex mutex;
        static inline std::uint64_t generation = 0;
        static inline Kind pendingKind = Kind::None;
        static inline bool opened = false;
        static inline Request request;
    };
}
