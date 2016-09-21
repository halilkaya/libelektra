/**
 * @file
 *
 * @brief Assertions macros.
 *
 * @copyright BSD License (see doc/COPYING or http://www.libelektra.org)
 */

void elektraAbort (const char * expression, const char * function, const char * file, const int line, const char * msg, ...);

#define STRINGIFY(x) STRINGIFY2 (x)
#define STRINGIFY2(x) #x

#ifdef ELEKTRA_BMC
#undef NDEBUG
#include <assert.h>
#define ELEKTRA_ASSERT(EXPR, ...) assert (EXPR)
#else
#define ELEKTRA_ASSERT(EXPR, ...) ((EXPR)) ? (void)(0) : elektraAbort (STRINGIFY (EXPR), __func__, __FILE__, __LINE__, ##__VA_ARGS__)
#endif
