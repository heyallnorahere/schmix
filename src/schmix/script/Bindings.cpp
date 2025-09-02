#include "schmixpch.h"
#include "schmix/script/Bindings.h"

#include "schmix/core/Ref.h"

#include "schmix/audio/Mixer.h"
#include "schmix/audio/WindowAudioOutput.h"

#include <Coral/Array.hpp>

namespace schmix {
    static void RefCounted_AddRef_Impl(RefCounted* object) {
        Ref<RefCounted>::IncreaseRefCount(object);
    }

    static void RefCounted_RemoveRef_Impl(RefCounted* object) {
        Ref<RefCounted>::DecreaseRefCount(object);
    }

    static void Mixer_AddSignalToChannel_Impl(Mixer* mixer, std::uint32_t channel,
                                              std::int32_t audioChannels, std::int32_t length,
                                              Coral::Array<double> interleaved) {
        auto signal =
            StereoSignal<double>::FromInterleaved(audioChannels, length, interleaved.Data());

        mixer->AddSignalToChannel(channel, signal);
    }

    static std::int32_t Mixer_GetChunkSize_Impl(Mixer* mixer) {
        return (std::int32_t)mixer->GetChunkSize();
    }

    static std::int32_t Mixer_GetSampleRate_Impl(Mixer* mixer) {
        return (std::int32_t)mixer->GetSampleRate();
    }

    static std::int32_t Mixer_GetAudioChannels_Impl(Mixer* mixer) {
        return (std::int32_t)mixer->GetAudioChannels();
    }

    static std::uint32_t WindowAudioOutput_GetDefaultDeviceID_Impl() {
        return WindowAudioOutput::GetDefaultDeviceID();
    }

    static WindowAudioOutput* WindowAudioOutput_ctor_Impl(std::uint32_t deviceID,
                                                          std::int32_t sampleRate,
                                                          std::int32_t channels) {
        auto output = new WindowAudioOutput(deviceID, sampleRate, channels);
        if (!output->IsInitialized()) {
            delete output;
            output = nullptr;
        }

        return output;
    }

    static std::int32_t WindowAudioOutput_GetQueuedSamples_Impl(WindowAudioOutput* output) {
        return output->GetQueuedSamples();
    }

    static std::int32_t WindowAudioOutput_GetSampleRate_Impl(WindowAudioOutput* output) {
        return output->GetSampleRate();
    }

    static std::int32_t WindowAudioOutput_GetChannels_Impl(WindowAudioOutput* output) {
        return output->GetChannels();
    }

    static Coral::Bool32 WindowAudioOutput_PutAudio_Impl(WindowAudioOutput* output,
                                                         std::int32_t length,
                                                         Coral::Array<double> interleaved) {
        auto signal = StereoSignal<double>::FromInterleaved(output->GetChannels(), length,
                                                            interleaved.Data());

        return output->PutAudio(signal);
    }

    void Bindings::Get(std::vector<ScriptBinding>& bindings) {
        bindings.insert(
            bindings.end(),
            {
                { "Schmix.Core.RefCounted", "AddRef_Impl", (void*)RefCounted_AddRef_Impl },
                { "Schmix.Core.RefCounted", "RemoveRef_Impl", (void*)RefCounted_RemoveRef_Impl },

                { "Schmix.Audio.Mixer", "AddSignalToChannel_Impl",
                  (void*)Mixer_AddSignalToChannel_Impl },
                { "Schmix.Audio.Mixer", "GetChunkSize_Impl", (void*)Mixer_GetChunkSize_Impl },
                { "Schmix.Audio.Mixer", "GetSampleRate_Impl", (void*)Mixer_GetSampleRate_Impl },
                { "Schmix.Audio.Mixer", "GetAudioChannels_Impl",
                  (void*)Mixer_GetAudioChannels_Impl },

                { "Schmix.Audio.WindowAudioOutput", "GetDefaultDeviceID_Impl",
                  (void*)WindowAudioOutput_GetDefaultDeviceID_Impl },
                { "Schmix.Audio.WindowAudioOutput", "ctor_Impl",
                  (void*)WindowAudioOutput_ctor_Impl },
                { "Schmix.Audio.WindowAudioOutput", "GetQueuedSamples_Impl",
                  (void*)WindowAudioOutput_GetQueuedSamples_Impl },
                { "Schmix.Audio.WindowAudioOutput", "GetSampleRate_Impl",
                  (void*)WindowAudioOutput_GetSampleRate_Impl },
                { "Schmix.Audio.WindowAudioOutput", "GetChannels_Impl",
                  (void*)WindowAudioOutput_GetChannels_Impl },
                { "Schmix.Audio.WindowAudioOutput", "PutAudio_Impl",
                  (void*)WindowAudioOutput_PutAudio_Impl },
            });
    }
} // namespace schmix
