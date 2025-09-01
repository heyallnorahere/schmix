#include "schmixpch.h"
#include "schmix/script/ScriptRuntime.h"

#include <Coral/GC.hpp>

namespace schmix {
    ScriptRuntime::ScriptRuntime(const std::filesystem::path& runtimeDir) {
        m_Initialized = false;

        m_RuntimeDirectory = runtimeDir.lexically_normal();
        if (!std::filesystem::is_directory(m_RuntimeDirectory)) {
            // todo: error message
            return;
        }

        Coral::HostSettings settings;
        settings.CoralDirectory = m_RuntimeDirectory;

        if (m_Host.Initialize(settings) != Coral::CoralInitStatus::Success) {
            // todo: error message
            return;
        }

        m_LoadContext = m_Host.CreateAssemblyLoadContext("Schmix");

        if (!LoadCore()) {
            // todo: error message
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
            // todo: error message
            return false;
        }

        m_CoreAssembly = &loadedAssembly;
        return true;
    }
} // namespace schmix
