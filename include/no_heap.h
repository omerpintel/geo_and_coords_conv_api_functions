#pragma once
#if defined(_DEBUG) || !defined(NDEBUG)

/**
 * Heap Allocation Prevention Mechanism.
 *
 * This header enforces a strict "No Heap" policy for the project. It declares
 * the global `new` and `delete` operators but does not provide definitions (or marks them deleted).
 *
 * When this header is forced into compilation units (via the CMake /FI or -include flags),
 * any code attempting to perform dynamic memory allocation will fail to build.
 */

#include <cstddef>

void* operator new(std::size_t);
void* operator new[](std::size_t);
void operator delete(void*) noexcept;
void operator delete[](void*) noexcept;

void operator delete(void*, std::size_t) noexcept;
void operator delete[](void*, std::size_t) noexcept;

#endif // DEBUG