#include "schmixpch.h"
#include "schmix/script/Bindings.h"

#include "schmix/core/Ref.h"

#include "schmix/audio/AudioDevice.h"

#include "schmix/encoding/FormatStream.h"
#include "schmix/encoding/CodecStream.h"

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

    static void* MemoryAllocator_Allocate_Impl(std::size_t size) { return Memory::Allocate(size); }
    static void MemoryAllocator_Free_Impl(void* block) { Memory::Free(block); }

    static std::uint32_t AudioDevice_GetDummy_Impl() { return AudioDevice::GetDummyID(); }

    static std::uint32_t AudioDevice_GetDefaultInput_Impl() {
        return AudioDevice::GetDefaultInputID();
    }

    static std::uint32_t AudioDevice_GetDefaultOutput_Impl() {
        return AudioDevice::GetDefaultOutputID();
    }

    static Coral::Array<std::uint32_t> AudioDevice_GetInputDevices_Impl() {
        Coral::Array<std::uint32_t> ids;

        auto devices = AudioDevice::GetInputDevices();
        if (!devices.empty()) {
            ids = Coral::Array<std::uint32_t>::New(devices.size());

            for (std::size_t i = 0; i < devices.size(); i++) {
                ids[i] = (std::uint32_t)devices[i];
            }
        }

        return ids;
    }

    static Coral::Array<std::uint32_t> AudioDevice_GetOutputDevices_Impl() {
        Coral::Array<std::uint32_t> ids;

        auto devices = AudioDevice::GetOutputDevices();
        if (!devices.empty()) {
            ids = Coral::Array<std::uint32_t>::New(devices.size());

            for (std::size_t i = 0; i < devices.size(); i++) {
                ids[i] = (std::uint32_t)devices[i];
            }
        }

        return ids;
    }

    static Coral::String AudioDevice_GetDeviceName_Impl(std::uint32_t id) {
        Coral::String result;

        auto name = AudioDevice::GetDeviceName((std::size_t)id);
        if (name.has_value()) {
            result = Coral::String::New(name.value());
        }

        return result;
    }

    static AudioDevice* AudioDevice_ctor_Impl(std::uint32_t deviceID, std::int32_t sampleRate,
                                              std::int32_t channels) {
        auto device = new AudioDevice(deviceID, sampleRate, channels);
        if (!device->IsInitialized()) {
            delete device;
            device = nullptr;
        }

        return device;
    }

    static std::uint32_t AudioDevice_GetDeviceID_Impl(AudioDevice* device) {
        return device->GetDeviceID();
    }

    static std::int32_t AudioDevice_GetAvailableSamples_Impl(AudioDevice* device) {
        return device->GetAvailableSamples();
    }

    static std::int32_t AudioDevice_GetQueuedSamples_Impl(AudioDevice* device) {
        return device->GetQueuedSamples();
    }

    static std::int32_t AudioDevice_GetSampleRate_Impl(AudioDevice* device) {
        return device->GetSampleRate();
    }

    static std::int32_t AudioDevice_GetChannels_Impl(AudioDevice* device) {
        return device->GetChannels();
    }

    static std::int32_t AudioDevice_GetAudio_Impl(AudioDevice* device, std::int32_t requested,
                                                  Coral::Array<double> interleaved) {
        std::size_t channels = device->GetChannels();
        std::size_t totalSamples = (std::size_t)requested * channels;

        float src[totalSamples];
        auto result = device->GetInterleavedAudio(src, requested);

        if (!result.has_value()) {
            return -1;
        }

        std::size_t sampleCount = result.value();
        for (std::size_t i = 0; i < sampleCount * channels; i++) {
            interleaved[i] = (double)src[i];
        }

        return (std::int32_t)sampleCount;
    }

    static Coral::Bool32 AudioDevice_PutAudio_Impl(AudioDevice* device, std::int32_t length,
                                                   Coral::Array<double> interleaved) {
        std::size_t channels = device->GetChannels();
        std::size_t totalSamples = (std::size_t)length * channels;

        float result[totalSamples];
        for (std::size_t i = 0; i < totalSamples; i++) {
            result[i] = (float)interleaved[i];
        }

        return device->PutInterleavedAudio(result, length);
    }

    static Coral::Bool32 AudioDevice_Flush_Impl(AudioDevice* device) { return device->Flush(); }

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

    struct ManagedStreamCallbacks {
        std::int32_t (*ReadPacket)(void* buffer, std::int32_t bufferSize, void* userData);
        std::int32_t (*WritePacket)(const void* data, std::int32_t dataSize, void* userData);
        std::int64_t (*Seek)(std::int64_t offset, std::int32_t whence, void* userData);

        void* UserData;
    };

    static IO::Callbacks FromManagedCallbacks(const ManagedStreamCallbacks* srcCallbacks) {
        IO::Callbacks callbacks;
        void* userData = srcCallbacks->UserData;

        auto readPacket = srcCallbacks->ReadPacket;
        if (readPacket != nullptr) {
            callbacks.ReadPacket = [readPacket, userData](void* buffer, std::int32_t bufferSize) {
                return readPacket(buffer, bufferSize, userData);
            };
        }

        auto writePacket = srcCallbacks->WritePacket;
        if (writePacket != nullptr) {
            callbacks.WritePacket = [writePacket, userData](const void* data,
                                                            std::size_t dataSize) {
                return writePacket(data, dataSize, userData);
            };
        }

        auto seek = srcCallbacks->Seek;
        if (seek != nullptr) {
            callbacks.Seek = [seek, userData](std::int64_t offset, std::int32_t whence) {
                return seek(offset, whence, userData);
            };
        }

        return callbacks;
    }

    static const AVOutputFormat* FormatStream_GuessOutputFormat_Impl(Coral::String path) {
        return FormatStream::GuessOutputFormat(path.Data());
    }

    static FormatStream* FormatStream_ctor_Impl(const ManagedStreamCallbacks* srcCallbacks,
                                                IO::Mode mode, const AVOutputFormat* outputFormat) {
        auto callbacks = FromManagedCallbacks(srcCallbacks);
        auto stream = new FormatStream(callbacks, mode, outputFormat);

        if (!stream->IsOpen()) {
            SCHMIX_ERROR("Failed to open a formatting stream from managed code!");

            delete stream;
            stream = nullptr;
        }

        return stream;
    }

    static void FormatStream_Close_Impl(FormatStream* stream) { delete stream; }

    static IO::Mode FormatStream_GetMode_Impl(FormatStream* stream) { return stream->GetMode(); }

    static std::int32_t FormatStream_GetStreamIndex_Impl(FormatStream* stream) {
        return stream->GetAudioStreamIndex();
    }

    static const CodecParameters* FormatStream_GetCodecParameters_Impl(FormatStream* stream) {
        return &stream->GetCodecParameters();
    }

    static std::int32_t FormatStream_ReadPacket_Impl(FormatStream* stream, void** data) {
        auto result = stream->ReadPacket(data);
        if (!result.has_value()) {
            // assume eof
            return -1;
        }

        std::size_t bytesRead = result.value();
        return (std::int32_t)bytesRead;
    }

    static Coral::Bool32 FormatStream_WritePacket_Impl(FormatStream* stream, const void* data,
                                                       std::int32_t dataSize) {
        return stream->WritePacket(data, (std::int32_t)dataSize);
    }

    static Coral::Bool32 FormatStream_Flush_Impl(FormatStream* stream) { return stream->Flush(); }

    static CodecStream* CodecStream_ctor_Impl(const ManagedStreamCallbacks* srcCallbacks,
                                              IO::Mode mode, const CodecParameters* parameters,
                                              std::int32_t streamIndex) {
        auto callbacks = FromManagedCallbacks(srcCallbacks);
        auto stream = new CodecStream(callbacks, mode, *parameters, streamIndex);

        if (!stream->IsOpen()) {
            SCHMIX_ERROR("Failed to open a codec stream from managed code!");

            delete stream;
            stream = nullptr;
        }

        return stream;
    }

    static void CodecStream_Close_Impl(CodecStream* stream) { delete stream; }

    static IO::Mode CodecStream_GetMode_Impl(CodecStream* stream) { return stream->GetMode(); }

    static const CodecParameters* CodecStream_GetParameters_Impl(CodecStream* stream) {
        return &stream->GetParameters();
    }

    static std::int32_t CodecStream_GetStreamIndex_Impl(CodecStream* stream) {
        return (std::int32_t)stream->GetStreamIndex();
    }

    static std::int32_t CodecStream_GetFrameSize_Impl(CodecStream* stream) {
        return (std::int32_t)stream->GetFrameSize();
    }

    static std::int32_t CodecStream_ReadFrame_Impl(CodecStream* stream, void** data) {
        auto result = stream->ReadFrame(data);
        if (!result.has_value()) {
            // assume eof
            return -1;
        }

        std::size_t samplesRead = result.value();
        return (std::int32_t)samplesRead;
    }

    static Coral::Bool32 CodecStream_WriteFrame_Impl(CodecStream* stream, const void* data,
                                                     std::int32_t samples) {
        return stream->WriteFrame(data, samples);
    }

    static Coral::Bool32 CodecStream_Flush_Impl(CodecStream* stream) { return stream->Flush(); }

    void Bindings::Get(std::vector<ScriptBinding>& bindings) {
        bindings.insert(
            bindings.end(),
            {
                { "Schmix.Core.RefCounted", "AddRef_Impl", (void*)RefCounted_AddRef_Impl },
                { "Schmix.Core.RefCounted", "RemoveRef_Impl", (void*)RefCounted_RemoveRef_Impl },

                { "Schmix.Core.Log", "Print_Impl", (void*)Log_Print_Impl },

                { "Schmix.Core.MemoryAllocator", "Allocate_Impl", (void*)MemoryAllocator_Allocate_Impl },
                { "Schmix.Core.MemoryAllocator", "Free_Impl", (void*)MemoryAllocator_Free_Impl },

                { "Schmix.Audio.AudioDevice", "GetDummy_Impl", (void*)AudioDevice_GetDummy_Impl },
                { "Schmix.Audio.AudioDevice", "GetDefaultInput_Impl",
                  (void*)AudioDevice_GetDefaultInput_Impl },
                { "Schmix.Audio.AudioDevice", "GetDefaultOutput_Impl",
                  (void*)AudioDevice_GetDefaultOutput_Impl },
                { "Schmix.Audio.AudioDevice", "GetInputDevices_Impl",
                  (void*)AudioDevice_GetInputDevices_Impl },
                { "Schmix.Audio.AudioDevice", "GetOutputDevices_Impl",
                  (void*)AudioDevice_GetOutputDevices_Impl },
                { "Schmix.Audio.AudioDevice", "GetDeviceName_Impl",
                  (void*)AudioDevice_GetDeviceName_Impl },
                { "Schmix.Audio.AudioDevice", "ctor_Impl", (void*)AudioDevice_ctor_Impl },
                { "Schmix.Audio.AudioDevice", "GetDeviceID_Impl",
                  (void*)AudioDevice_GetDeviceID_Impl },
                { "Schmix.Audio.AudioDevice", "GetAvailableSamples_Impl",
                  (void*)AudioDevice_GetAvailableSamples_Impl },
                { "Schmix.Audio.AudioDevice", "GetQueuedSamples_Impl",
                  (void*)AudioDevice_GetQueuedSamples_Impl },
                { "Schmix.Audio.AudioDevice", "GetSampleRate_Impl",
                  (void*)AudioDevice_GetSampleRate_Impl },
                { "Schmix.Audio.AudioDevice", "GetChannels_Impl",
                  (void*)AudioDevice_GetChannels_Impl },
                { "Schmix.Audio.AudioDevice", "GetAudio_Impl", (void*)AudioDevice_GetAudio_Impl },
                { "Schmix.Audio.AudioDevice", "PutAudio_Impl", (void*)AudioDevice_PutAudio_Impl },
                { "Schmix.Audio.AudioDevice", "Flush_Impl", (void*)AudioDevice_Flush_Impl },

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

                { "Schmix.Encoding.FormatStream", "GuessOutputFormat_Impl",
                  (void*)FormatStream_GuessOutputFormat_Impl },
                { "Schmix.Encoding.FormatStream", "ctor_Impl", (void*)FormatStream_ctor_Impl },
                { "Schmix.Encoding.FormatStream", "Close_Impl", (void*)FormatStream_Close_Impl },
                { "Schmix.Encoding.FormatStream", "GetMode_Impl",
                  (void*)FormatStream_GetMode_Impl },
                { "Schmix.Encoding.FormatStream", "GetStreamIndex_Impl",
                  (void*)FormatStream_GetStreamIndex_Impl },
                { "Schmix.Encoding.FormatStream", "GetCodecParameters_Impl",
                  (void*)FormatStream_GetCodecParameters_Impl },
                { "Schmix.Encoding.FormatStream", "ReadPacket_Impl",
                  (void*)FormatStream_ReadPacket_Impl },
                { "Schmix.Encoding.FormatStream", "WritePacket_Impl",
                  (void*)FormatStream_WritePacket_Impl },
                { "Schmix.Encoding.FormatStream", "Flush_Impl", (void*)FormatStream_Flush_Impl },

                { "Schmix.Encoding.CodecStream", "ctor_Impl", (void*)CodecStream_ctor_Impl },
                { "Schmix.Encoding.CodecStream", "Close_Impl", (void*)CodecStream_Close_Impl },
                { "Schmix.Encoding.CodecStream", "GetMode_Impl", (void*)CodecStream_GetMode_Impl },
                { "Schmix.Encoding.CodecStream", "GetParameters_Impl",
                  (void*)CodecStream_GetParameters_Impl },
                { "Schmix.Encoding.CodecStream", "GetStreamIndex_Impl",
                  (void*)CodecStream_GetStreamIndex_Impl },
                { "Schmix.Encoding.CodecStream", "GetFrameSize_Impl",
                  (void*)CodecStream_GetFrameSize_Impl },
                { "Schmix.Encoding.CodecStream", "ReadFrame_Impl",
                  (void*)CodecStream_ReadFrame_Impl },
                { "Schmix.Encoding.CodecStream", "WriteFrame_Impl",
                  (void*)CodecStream_WriteFrame_Impl },
                { "Schmix.Encoding.CodecStream", "Flush_Impl", (void*)CodecStream_Flush_Impl },
            });
    }
} // namespace schmix
