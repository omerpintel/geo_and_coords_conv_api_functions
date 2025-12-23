#pragma once
#include "api_functions.h"
#include "cov_spy.h"

#if defined(_DEBUG) || !defined(NDEBUG)

extern "C" {
    API_FUNCTIONS bool* GetCoverageArray(ECovFuncID funcId);

    API_FUNCTIONS void ResetCoverage();
}

#endif