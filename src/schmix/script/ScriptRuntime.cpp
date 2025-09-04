#include "schmixpch.h"
#include "schmix/script/ScriptRuntime.h"

#include <Coral/GC.hpp>

namespace schmix {
    static void CoralMessageCallback(std::string_view message, Coral::MessageLevel level) {
        auto messageStr = std::string(message);

        switch (level) {
        case Coral::MessageLevel::Error:
            SCHMIX_ERROR("Coral: {}", messageStr.c_str());
            break;
        case Coral::MessageLevel::Warning:
            SCHMIX_WARN("Coral: {}", messageStr.c_str());
            break;
        case Coral::MessageLevel::Info:
        default:
            SCHMIX_INFO("Coral: {}", messageStr.c_str());
            break;
        }
    }

    static void CoralExceptionCallback(std::string_view message) {
        auto messageStr = std::string(message);
        SCHMIX_ERROR("Managed exception: {}", messageStr.c_str());
    }

    ScriptRuntime::ScriptRuntime(const std::filesystem::path& runtimeDir) {
        m_Initialized = false;

        m_RuntimeDirectory = runtimeDir.lexically_normal();
        if (!std::filesystem::is_directory(m_RuntimeDirectory)) {
            SCHMIX_ERROR("No such directory: {}", m_RuntimeDirectory.string().c_str());
            return;
        }

        Coral::HostSettings settings;
        settings.CoralDirectory = m_RuntimeDirectory;
        settings.MessageCallback = CoralMessageCallback;
        settings.MessageFilter = Coral::MessageLevel::All;
        settings.ExceptionCallback = CoralExceptionCallback;

        if (m_Host.Initialize(settings) != Coral::CoralInitStatus::Success) {
            SCHMIX_ERROR("Failed to initialize Coral host instance!");
            return;
        }

        m_LoadContext = m_Host.CreateAssemblyLoadContext("Schmix");

        if (!LoadCore()) {
            SCHMIX_ERROR("Failed to load core runtime assembly!");
            return;
        }

        m_Initialized = true;
    }

    ScriptRuntime::~ScriptRuntime() {
        if (!m_Initialized) {
            return;
        }

        Coral::GC::Collect();

        m_Host.UnloadAssemblyLoadContext(m_LoadContext);
    }

    Coral::ManagedAssembly& ScriptRuntime::LoadAssembly(const std::filesystem::path& path) {
        return m_LoadContext.LoadAssembly(path.string());
    }

    bool ScriptRuntime::RegisterCoreBindings(const std::vector<ScriptBinding>& bindings) {
        if (!m_Initialized) {
            SCHMIX_WARN("Script runtime not initialized; skipping binding registration...");
            return false;
        }

        for (const auto& binding : bindings) {
            m_CoreAssembly->AddInternalCall(binding.ClassName, binding.CallbackName,
                                            binding.CallbackPtr);
        }

        m_CoreAssembly->UploadInternalCalls();
        return true;
    }

    bool ScriptRuntime::LoadCore() {
        std::filesystem::path corePath = m_RuntimeDirectory / "Schmix.dll";
        auto& loadedAssembly = LoadAssembly(corePath);

        if (loadedAssembly.GetLoadStatus() != Coral::AssemblyLoadStatus::Success) {
            SCHMIX_ERROR("Failed to load core assembly at path: {}", corePath.string().c_str());
            return false;
        }

        m_CoreAssembly = &loadedAssembly;
        return true;
    }
} // namespace schmix
