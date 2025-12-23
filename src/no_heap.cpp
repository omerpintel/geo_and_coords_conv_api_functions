#if defined(_DEBUG) || !defined(NDEBUG)

#include <new>
#include <cstdlib> 
#include <cassert> 


#ifdef NDEBUG
extern void LINKER_ERROR_DYNAMIC_MEMORY_ALLOCATION_IS_FORBIDDEN();
#else

#endif


void* operator new(std::size_t) {
#ifdef NDEBUG
    LINKER_ERROR_DYNAMIC_MEMORY_ALLOCATION_IS_FORBIDDEN();
#else
    assert(false && "CRITICAL SAFETY VIOLATION: Dynamic memory allocation detected in Core Logic!");
    std::abort();
#endif
    return nullptr;
}

void* operator new[](std::size_t) {
#ifdef NDEBUG
    LINKER_ERROR_DYNAMIC_MEMORY_ALLOCATION_IS_FORBIDDEN();
#else
    assert(false && "CRITICAL SAFETY VIOLATION: Dynamic array allocation detected!");
    std::abort();
#endif
    return nullptr;
}

void operator delete(void*) noexcept {
#ifdef NDEBUG
    LINKER_ERROR_DYNAMIC_MEMORY_ALLOCATION_IS_FORBIDDEN();
#else
    assert(false && "CRITICAL SAFETY VIOLATION: Delete operator called!");
    std::abort();
#endif
}

void operator delete[](void*) noexcept {
#ifdef NDEBUG
    LINKER_ERROR_DYNAMIC_MEMORY_ALLOCATION_IS_FORBIDDEN();
#else
    assert(false && "CRITICAL SAFETY VIOLATION: Delete[] operator called!");
    std::abort();
#endif
}

void operator delete(void*, std::size_t) noexcept {
#ifdef NDEBUG
    LINKER_ERROR_DYNAMIC_MEMORY_ALLOCATION_IS_FORBIDDEN();
#else
    std::abort();
#endif
}

void operator delete[](void*, std::size_t) noexcept {
#ifdef NDEBUG
    LINKER_ERROR_DYNAMIC_MEMORY_ALLOCATION_IS_FORBIDDEN();
#else
    std::abort();
#endif
}

#endif // DEBUG