#include "file_system.h"
#include <cpu/guest_thread.h>
#include <kernel/kernel.h>
#include <kernel/function.h>
#include <stdafx.h>

struct FileHandle : KernelObject
{
    std::fstream stream;
    std::filesystem::path path;
};
