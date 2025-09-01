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

    void Bindings::Get(std::vector<ScriptBinding>& bindings) {
        bindings.insert(
            bindings.end(),
            {
                { "Schmix.RefCounted", "AddRef_Impl", (void*)AddRef_Impl },
                { "Schmix.RefCounted", "RemoveRef_Impl", (void*)RemoveRef_Impl },

                { "Schmix.Mixer", "AddSignalToChannel_Impl", (void*)AddSignalToChannel_Impl },
            });
    }
} // namespace schmix
