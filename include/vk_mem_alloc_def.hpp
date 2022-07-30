#pragma once

#ifdef DEBUG
#define VMA_DEBUG_INITIALIZE_ALLOCATIONS 1
#define VMA_DEBUG_MARGIN 16
#define VMA_DEBUG_DETECT_CORRUPTION 1

#define VMA_DEBUG_LOG(format, ...) do { \
        /*printf(format, #__VA_ARGS__); \
        printf("\n");*/ \
    } while(false)

#endif

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"
