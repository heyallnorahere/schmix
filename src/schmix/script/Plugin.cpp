#include "schmixpch.h"
#include "schmix/script/Plugin.h"

#include <Coral/GC.hpp>

namespace schmix {
    struct PluginData {
        Ref<ScriptRuntime> Runtime;
        Coral::Type* PluginType;

        bool PluginsLoaded;
        std::filesystem::path PluginDirectory;
        Coral::AssemblyLoadContext LoadContext;
    };

    static std::unique_ptr<PluginData> s_Data;

    bool Plugin::Init(const Ref<ScriptRuntime>& runtime) {
        if (s_Data || !runtime || !runtime->IsInitialized()) {
            return false;
        }

        s_Data = std::make_unique<PluginData>();
        s_Data->Runtime = runtime;

        auto core = runtime->GetCore();
        auto& pluginType = core->GetType("Schmix.Extension.Plugin");

        if (!pluginType) {
            SCHMIX_ERROR("Failed to find plugin type!");

            s_Data.reset();
            return false;
        }

        s_Data->PluginType = &pluginType;
        s_Data->PluginsLoaded = false;

        return true;
    }

    void Plugin::Cleanup() {
        if (!s_Data) {
            return;
        }

        if (s_Data->PluginsLoaded) {
            UnloadPlugins();
        }

        s_Data.reset();
    }

    bool Plugin::LoadPlugins(const std::filesystem::path& directory) {
        if (!s_Data || s_Data->PluginsLoaded) {
            return false;
        }

        auto& host = s_Data->Runtime->GetHost();

        s_Data->PluginsLoaded = true;
        s_Data->PluginDirectory = directory;
        s_Data->LoadContext = host.CreateAssemblyLoadContext("Plugins");

        SCHMIX_INFO("Loading plugins in directory: {}", directory.string().c_str());

        std::size_t pluginCount = 0;
        if (std::filesystem::is_directory(directory)) {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
                if (!entry.is_regular_file()) {
                    continue;
                }

                auto path = entry.path().lexically_normal();
                auto extension = path.extension();

                if (extension != ".dll") {
                    continue;
                }

                std::string pathStr = path.string();
                SCHMIX_INFO("Loading plugin library: {}", pathStr.c_str());

                auto& assembly = s_Data->LoadContext.LoadAssembly(pathStr);
                if (assembly.GetLoadStatus() != Coral::AssemblyLoadStatus::Success) {
                    SCHMIX_ERROR("Failed to load plugin library: {}", pathStr.c_str());

                    UnloadPlugins();
                    return false;
                }

                std::int32_t id = assembly.GetAssemblyID();
                Coral::String errorMessage =
                    s_Data->PluginType->InvokeStaticMethod<Coral::String, std::int32_t>(
                        "LoadPluginsFromAssembly_Native", std::move(id));

                if (errorMessage.Data() != nullptr) {
                    SCHMIX_ERROR("Failed to load plugins from assembly {}: {}", pathStr.c_str(),
                                 errorMessage.Data());

                    Coral::String::Free(errorMessage);

                    UnloadPlugins();
                    return false;
                }

                pluginCount++;
            }
        }

        SCHMIX_INFO("Loaded {} plugins", pluginCount);
        return true;
    }

    void Plugin::UnloadPlugins() {
        if (!s_Data) {
            return;
        }

        if (!s_Data->PluginsLoaded) {
            return;
        }

        Coral::GC::Collect();

        s_Data->PluginType->InvokeStaticMethod("UnloadPlugins");

        auto& host = s_Data->Runtime->GetHost();
        host.UnloadAssemblyLoadContext(s_Data->LoadContext);

        s_Data->PluginsLoaded = false;
    }
} // namespace schmix
