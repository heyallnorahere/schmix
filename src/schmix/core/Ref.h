#pragma once

namespace schmix {
    class RefCounted {
    public:
        RefCounted() = default;
        virtual ~RefCounted() = default;

    private:
        std::atomic<uint64_t> m_RefCount;

        template <typename _Ty>
        friend class Ref;
    };

    template <typename _Ty>
    class Ref {
    public:
        static void IncreaseRefCount(_Ty* instance) {
            if (instance == nullptr) {
                return;
            }

            instance->m_RefCount++;
        }

        static void DecreaseRefCount(_Ty* instance) {
            if (instance == nullptr) {
                return;
            }

            if (--instance->m_RefCount == 0) {
                delete instance;
                instance = nullptr;
            }
        }

        Ref() : m_Instance(nullptr) {}
        Ref(std::nullptr_t) : m_Instance(nullptr) {}

        Ref(_Ty* instance) : m_Instance(instance) {
            static_assert(std::is_base_of_v<RefCounted, _Ty>, "Type is not ref-counted!");

            IncreaseRefCount();
        }

        template <typename _Ty2>
        Ref(const Ref<_Ty2>& other) : m_Instance((_Ty*)other.m_Instance) {
            static_assert(std::is_base_of_v<_Ty, _Ty2>, "Invalid use of polymorphism!");

            IncreaseRefCount();
        }

        template <typename _Ty2>
        Ref(Ref<_Ty2>&& other) : m_Instance((_Ty*)other.m_Instance) {
            static_assert(std::is_base_of_v<_Ty, _Ty2>, "Invalid use of polymorphism!");

            other.m_Instance = nullptr;
        }

        Ref(const Ref<_Ty>& other) : m_Instance(other.m_Instance) { IncreaseRefCount(); }
        Ref(Ref<_Ty>&& other) : m_Instance(other.m_Instance) { other.m_Instance = nullptr; }

        ~Ref() { DecreaseRefCount(); }

        Ref& operator=(std::nullptr_t) {
            DecreaseRefCount();

            m_Instance = nullptr;
            return *this;
        }

        Ref& operator=(const Ref<_Ty>& other) {
            other.IncreaseRefCount();
            DecreaseRefCount();

            m_Instance = other.m_Instance;
            return *this;
        }

        template <typename _Ty2>
        Ref& operator=(const Ref<_Ty2>& other) {
            static_assert(std::is_base_of_v<_Ty, _Ty2>, "Invalid use of polymorphism!");

            other.IncreaseRefCount();
            DecreaseRefCount();

            m_Instance = (_Ty*)other.m_Instance;
            return *this;
        }

        template <typename _Ty2>
        Ref& operator=(Ref<_Ty2>&& other) {
            static_assert(std::is_base_of_v<_Ty, _Ty2>, "Invalid use of polymorphism!");

            DecreaseRefCount();
            m_Instance = (_Ty*)other.m_Instance;

            other.m_Instance = nullptr;
            return *this;
        }

        bool IsPresent() const { return m_Instance != nullptr; }
        bool IsEmpty() const { return m_Instance == nullptr; }

        operator bool() const { return IsPresent(); }

        _Ty* operator->() const { return m_Instance; }
        _Ty& operator*() const { return *m_Instance; }
        _Ty* Raw() const { return m_Instance; }

        void Reset(_Ty* instance = nullptr) {
            DecreaseRefCount();
            m_Instance = instance;

            if (m_Instance != nullptr) {
                IncreaseRefCount();
            }
        }

        template <typename _Ty2>
        Ref<_Ty2> As() const {
            return Ref<_Ty2>((_Ty2*)m_Instance);
        }

        template <typename... Args>
        static Ref<_Ty> Create(Args&&... args) {
            _Ty* instance = new _Ty(std::forward<Args>(args)...);
            return Ref<_Ty>(instance);
        }

        bool operator==(const Ref<_Ty>& other) const { return m_Instance == other.m_Instance; }
        bool operator!=(const Ref<_Ty>& other) const { return !(*this == other); }

    private:
        mutable _Ty* m_Instance;

        template <typename _Ty2>
        friend class Ref;
    };
} // namespace schmix
