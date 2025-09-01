#pragma once
#include "schmix/script/ScriptRuntime.h"

namespace schmix {
    class Bindings {
    public:
        Bindings() = delete;

        static void Get(std::vector<ScriptBinding>& bindings);
    };
} // namespace schmix
