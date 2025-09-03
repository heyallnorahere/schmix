#include "schmixpch.h"
#include "schmix/script/Bindings.h"

#include "schmix/core/Ref.h"

#include "schmix/audio/Mixer.h"
#include "schmix/audio/WindowAudioOutput.h"

#include "schmix/ui/Application.h"
#include "schmix/ui/ImGuiInstance.h"

#include <Coral/Array.hpp>

namespace schmix {
    static void RefCounted_AddRef_Impl(RefCounted* object) {
        Ref<RefCounted>::IncreaseRefCount(object);
    }

    static void RefCounted_RemoveRef_Impl(RefCounted* object) {
        Ref<RefCounted>::DecreaseRefCount(object);
    }

    static void Log_Print_Impl(spdlog::level::level_enum level, Coral::String msg,
                               Coral::String memberName, Coral::String file, std::int32_t line) {
        spdlog::source_loc loc;
        loc.line = (int)line;
        loc.filename = file.Data();
        loc.funcname = memberName.Data();

        g_Logger->log(loc, level, msg.Data());
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

    static Coral::Bool32 Application_IsRunning_Impl() {
        auto& app = Application::Get();
        return app.IsRunning();
    }

    static void Application_Quit_Impl(std::int32_t status) {
        auto& app = Application::Get();
        app.Quit(status);
    }

    static ImGuiInstance* Application_GetImGuiInstance_Impl() {
        auto& app = Application::Get();
        const auto& instance = app.GetImGuiInstance();

        return instance.Raw();
    }

    static void ImGuiInstance_GetAllocatorFunctions_Impl(void** allocPtr, void** freePtr) {
        *allocPtr = (void*)ImGuiInstance::MemAlloc;
        *freePtr = (void*)ImGuiInstance::MemFree;
    }

    static void* ImGuiInstance_GetContext_Impl(ImGuiInstance* instance) {
        return instance->GetContext();
    }

    static Coral::Bool32 ImGuiInstance_NewFrame_Impl(ImGuiInstance* instance) {
        return instance->NewFrame();
    }

    static Coral::Bool32 ImGuiInstance_RenderAndPresent_Impl(ImGuiInstance* instance) {
        return instance->RenderAndPresent();
    }

    void Bindings::Get(std::vector<ScriptBinding>& bindings) {
        bindings.insert(
            bindings.end(),
            {
                { "Schmix.Core.RefCounted", "AddRef_Impl", (void*)RefCounted_AddRef_Impl },
                { "Schmix.Core.RefCounted", "RemoveRef_Impl", (void*)RefCounted_RemoveRef_Impl },

                { "Schmix.Core.Log", "Print_Impl", (void*)Log_Print_Impl },

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

                { "Schmix.UI.Application", "IsRunning_Impl", (void*)Application_IsRunning_Impl },
                { "Schmix.UI.Application", "Quit_Impl", (void*)Application_Quit_Impl },
                { "Schmix.UI.Application", "GetImGuiInstance_Impl",
                  (void*)Application_GetImGuiInstance_Impl },

                { "Schmix.UI.ImGuiInstance", "GetAllocatorFunctions_Impl",
                  (void*)ImGuiInstance_GetAllocatorFunctions_Impl },
                { "Schmix.UI.ImGuiInstance", "GetContext_Impl",
                  (void*)ImGuiInstance_GetContext_Impl },
                { "Schmix.UI.ImGuiInstance", "NewFrame_Impl", (void*)ImGuiInstance_NewFrame_Impl },
                { "Schmix.UI.ImGuiInstance", "RenderAndPresent_Impl",
                  (void*)ImGuiInstance_RenderAndPresent_Impl },
            });
    }
} // namespace schmix
