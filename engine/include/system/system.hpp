#pragma once

#include "world/world.hpp"

namespace hex {

/* 
 * Define as: void function(WorldView& view)
 * It's a raw function pointer to reduce overhead
 */
using SystemFuncPtr = void(*)(WorldView&);

class System {
public:
    System(const SystemDescriptor& descriptor, SystemFuncPtr func)
        : m_descriptor(descriptor), m_func(func) {}

    inline const SystemDescriptor& getDescriptor() const { return m_descriptor; }
    inline void run(WorldView& view) { m_func(view); }
protected:
    SystemDescriptor m_descriptor;
    SystemFuncPtr m_func;
};

};