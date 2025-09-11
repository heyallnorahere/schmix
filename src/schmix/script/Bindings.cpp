#include "schmixpch.h"
#include "schmix/script/Bindings.h"

#include "schmix/core/Ref.h"

#include "schmix/audio/AudioDevice.h"

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

    static std::uint32_t OutputDevice_GetDefault_Impl() {
        return AudioDevice::GetDefaultDeviceID();
    }

    static AudioDevice* OutputDevice_ctor_Impl(std::uint32_t deviceID,
                                                          std::int32_t sampleRate,
                                                          std::int32_t channels) {
        auto output = new AudioDevice(deviceID, sampleRate, channels);
        if (!output->IsInitialized()) {
            delete output;
            output = nullptr;
        }

        return output;
    }

    static std::int32_t OutputDevice_GetQueuedSamples_Impl(AudioDevice* output) {
        return output->GetQueuedSamples();
    }

    static std::int32_t OutputDevice_GetSampleRate_Impl(AudioDevice* output) {
        return output->GetSampleRate();
    }

    static std::int32_t OutputDevice_GetChannels_Impl(AudioDevice* output) {
        return output->GetChannels();
    }

    static Coral::Bool32 OutputDevice_PutAudio_Impl(AudioDevice* output,
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

    static ImGuiContext* ImGuiInstance_GetContext_Impl(ImGuiInstance* instance) {
        return instance->GetContext();
    }

    static ImNodesContext* ImGuiInstance_GetNodesContext_Impl(ImGuiInstance* instance) {
        return instance->GetNodesContext();
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

                { "Schmix.Audio.OutputDevice", "GetDefault_Impl",
                  (void*)OutputDevice_GetDefault_Impl },
                { "Schmix.Audio.OutputDevice", "ctor_Impl",
                  (void*)OutputDevice_ctor_Impl },
                { "Schmix.Audio.OutputDevice", "GetQueuedSamples_Impl",
                  (void*)OutputDevice_GetQueuedSamples_Impl },
                { "Schmix.Audio.OutputDevice", "GetSampleRate_Impl",
                  (void*)OutputDevice_GetSampleRate_Impl },
                { "Schmix.Audio.OutputDevice", "GetChannels_Impl",
                  (void*)OutputDevice_GetChannels_Impl },
                { "Schmix.Audio.OutputDevice", "PutAudio_Impl",
                  (void*)OutputDevice_PutAudio_Impl },

                { "Schmix.UI.Application", "IsRunning_Impl", (void*)Application_IsRunning_Impl },
                { "Schmix.UI.Application", "Quit_Impl", (void*)Application_Quit_Impl },
                { "Schmix.UI.Application", "GetImGuiInstance_Impl",
                  (void*)Application_GetImGuiInstance_Impl },

                { "Schmix.UI.ImGuiInstance", "GetAllocatorFunctions_Impl",
                  (void*)ImGuiInstance_GetAllocatorFunctions_Impl },
                { "Schmix.UI.ImGuiInstance", "GetContext_Impl",
                  (void*)ImGuiInstance_GetContext_Impl },
                { "Schmix.UI.ImGuiInstance", "GetNodesContext_Impl",
                  (void*)ImGuiInstance_GetNodesContext_Impl },
                { "Schmix.UI.ImGuiInstance", "NewFrame_Impl", (void*)ImGuiInstance_NewFrame_Impl },
                { "Schmix.UI.ImGuiInstance", "RenderAndPresent_Impl",
                  (void*)ImGuiInstance_RenderAndPresent_Impl },
            });
    }
} // namespace schmix
