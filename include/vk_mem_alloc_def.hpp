#pragma once

#ifdef DEBUG
#define VMA_DEBUG_INITIALIZE_ALLOCATIONS 1
#define VMA_DEBUG_MARGIN 8
#define VMA_DEBUG_DETECT_CORRUPTION 1
#endif

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"
