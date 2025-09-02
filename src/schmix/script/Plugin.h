#pragma once
#include "schmix/script/ScriptRuntime.h"

namespace schmix {
    class Plugin {
    public:
        static bool Init(const Ref<ScriptRuntime>& runtime);
        static void Cleanup();

        static bool LoadPlugins(const std::filesystem::path& directory);
        static void UnloadPlugins();
    };
} // namespace schmix
