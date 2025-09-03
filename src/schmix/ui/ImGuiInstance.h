#pragma once
#include "schmix/core/Ref.h"

#include "schmix/ui/Window.h"

struct ImGuiContext;
struct ImNodesContext;

namespace schmix {
    class ImGuiInstance : public RefCounted {
    public:
        static void* MemAlloc(std::size_t sz, void* user_data);
        static void MemFree(void* ptr, void* user_data);

        ImGuiInstance(const Ref<Window>& window);
        virtual ~ImGuiInstance() override;

        void ProcessEvent(const SDL_Event& event);

        void MakeContextCurrent() const;

        bool NewFrame();
        bool RenderAndPresent();

        const Ref<Window>& GetWindow() const { return m_Window; }

        ImGuiContext* GetContext() const { return m_Context; }
        ImNodesContext* GetNodesContext() const { return m_NodesContext; }

        bool IsInitialized() const { return m_Initialized; }

    private:
        Ref<Window> m_Window;

        ImGuiContext* m_Context;
        ImNodesContext* m_NodesContext;
        bool m_Initialized;

        bool m_PlatformInitialized, m_RendererInitialized;
    };
} // namespace schmix
