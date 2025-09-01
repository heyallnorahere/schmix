#include "schmixpch.h"
#include "schmix/script/Bindings.h"

#include "schmix/core/Ref.h"

#include "schmix/audio/Mixer.h"

#include <Coral/Array.hpp>

namespace schmix {
    static void AddRef_Impl(RefCounted* object) { Ref<RefCounted>::IncreaseRefCount(object); }
    static void RemoveRef_Impl(RefCounted* object) { Ref<RefCounted>::DecreaseRefCount(object); }

    static void AddSignalToChannel_Impl(Mixer* mixer, std::uint32_t channel,
                                        std::int32_t audioChannels, std::int32_t length,
                                        Coral::Array<double> interleaved) {
        auto signal =
            StereoSignal<double>::FromInterleaved(audioChannels, length, interleaved.Data());

        mixer->AddSignalToChannel(channel, signal);
    }

    static std::int32_t GetChunkSize_Impl(Mixer* mixer) {
        return (std::int32_t)mixer->GetChunkSize();
    }

    static std::int32_t GetSampleRate_Impl(Mixer* mixer) {
        return (std::int32_t)mixer->GetSampleRate();
    }

    static std::int32_t GetAudioChannels_Impl(Mixer* mixer) {
        return (std::int32_t)mixer->GetAudioChannels();
    }

    void Bindings::Get(std::vector<ScriptBinding>& bindings) {
        bindings.insert(
            bindings.end(),
            {
                { "Schmix.RefCounted", "AddRef_Impl", (void*)AddRef_Impl },
                { "Schmix.RefCounted", "RemoveRef_Impl", (void*)RemoveRef_Impl },

                { "Schmix.Mixer", "AddSignalToChannel_Impl", (void*)AddSignalToChannel_Impl },
                { "Schmix.Mixer", "GetChunkSize_Impl", (void*)GetChunkSize_Impl },
                { "Schmix.Mixer", "GetSampleRate_Impl", (void*)GetSampleRate_Impl },
                { "Schmix.Mixer", "GetAudioChannels_Impl", (void*)GetAudioChannels_Impl },
            });
    }
} // namespace schmix
