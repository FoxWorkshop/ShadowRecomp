
#pragma once
#include <cstdint>

struct KernelObject {
    virtual ~KernelObject() = default;  // Required for proper cleanup

    // Minimal wait stub (needed for sync objects like events)
    virtual uint32_t Wait(uint32_t timeout_ms) {
        return 0; // STATUS_SUCCESS
    }

    // All kernel objects get a unique handle value
    uint32_t handle_id = 0;
};