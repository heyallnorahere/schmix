#pragma once
#include "schmix/core/Ref.h"

#include <Coral/HostInstance.hpp>

namespace schmix {
    struct ScriptBinding {
        std::string ClassName;
        std::string CallbackName;
        void* CallbackPtr;
    };

    class ScriptRuntime : public RefCounted {
    public:
        ScriptRuntime(const std::filesystem::path& runtimeDir);
        virtual ~ScriptRuntime() override;

        ScriptRuntime(const ScriptRuntime&) = delete;
        ScriptRuntime& operator=(const ScriptRuntime&) = delete;

        Coral::ManagedAssembly& LoadAssembly(const std::filesystem::path& path);

        bool RegisterCoreBindings(const std::vector<ScriptBinding>& bindings);

        Coral::HostInstance& GetHost() { return m_Host; }
        const Coral::HostInstance& GetHost() const { return m_Host; }

        Coral::AssemblyLoadContext& GetLoadContext() { return m_LoadContext; }

        bool IsInitialized() const { return m_Initialized; }

        const Coral::ManagedAssembly* GetCore() const { return m_CoreAssembly; }

    private:
        bool LoadCore();

        Coral::HostInstance m_Host;
        Coral::AssemblyLoadContext m_LoadContext;
        Coral::ManagedAssembly* m_CoreAssembly;

        std::filesystem::path m_RuntimeDirectory;
        bool m_Initialized;
    };
} // namespace schmix
