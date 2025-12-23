#pragma once

#include <cstdint>
/**
 * Internal instrumentation for manual code coverage tracking.
 *
 * This file defines macros used to verify that specific code paths are executed
 * during testing. This is a lightweight alternative to external coverage tools,
 * designed for specific logic verification.
 *
 * @note These macros are active ONLY in _DEBUG builds. In Release builds,
 * they resolve to no-ops (void)0 to ensure zero performance impact.
 */

enum ECovFuncID {
    IsInside = 0,
    Intersect = 1,
    MAX_FUNCS
};

#define MAX_POINTS_PER_FUNC 20 

#if defined(_DEBUG) || !defined(NDEBUG)
    extern bool g_cov_map[(int)ECovFuncID::MAX_FUNCS][MAX_POINTS_PER_FUNC];
    #define COV_POINT(id) g_cov_map[(int)current_func_id][id] = true
#else
    #define COV_POINT(id) ((void)0)
#endif