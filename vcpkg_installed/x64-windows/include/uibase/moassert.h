#ifndef UIBASE_MOASSERT_INCLUDED
#define UIBASE_MOASSERT_INCLUDED

#include "log.h"

namespace MOBase
{

template <class T>
inline void MOAssert(T&& t, const char* exp, const char* file, int line,
                     const char* func)
{
  if (!t) {
    log::error("assertion failed: {}:{} {}: '{}'", file, line, func, exp);

    if (IsDebuggerPresent()) {
      DebugBreak();
    }
  }
}

}  // namespace MOBase

#define MO_ASSERT(v) MOAssert(v, #v, __FILE__, __LINE__, __FUNCSIG__)

#endif  // UIBASE_MOASSERT_INCLUDED
