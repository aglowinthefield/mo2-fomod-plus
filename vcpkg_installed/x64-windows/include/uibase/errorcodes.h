#ifndef UIBASE_ERRORCODES_H
#define UIBASE_ERRORCODES_H

#include "dllimport.h"
#include <Windows.h>

namespace MOBase
{

QDLLEXPORT const wchar_t* errorCodeName(DWORD code);

}  // namespace MOBase

#endif UIBASE_ERRORCODES_H
