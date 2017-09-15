#pragma once

#ifdef __cplusplus
extern "C" {
#else // __cplusplus
#include <stdbool.h>
#endif // __cplusplus

bool output(const char *filename, const unsigned int *freqlist, const unsigned long *beeptimelist, const unsigned long *sleeptimelist, size_t notecount);

#ifdef __cplusplus
}
#endif // __cplusplus
