/*
Mod Organizer shared UI functionality

Copyright (C) 2012 Sebastian Herbord. All rights reserved.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 3 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef DLLIMPORT_H
#define DLLIMPORT_H

namespace MOBase
{

#if defined(UIBASE_EXPORT)
#define QDLLEXPORT __declspec(dllexport)
#elif defined(_NODLL)
#define QDLLEXPORT
#else
#undef DLLEXPORT
#define QDLLEXPORT __declspec(dllimport)
#endif

}  // namespace MOBase

#endif  // DLLIMPORT_H
