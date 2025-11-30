if(NOT DEFINED PUGIXML_CMAKELISTS)
  message(FATAL_ERROR "PUGIXML_CMAKELISTS not set; cannot patch pugixml.")
endif()

if(NOT EXISTS "${PUGIXML_CMAKELISTS}")
  message(FATAL_ERROR "Pugixml CMakeLists.txt not found at '${PUGIXML_CMAKELISTS}'.")
endif()

file(READ "${PUGIXML_CMAKELISTS}" _pugixml_content)
string(REGEX REPLACE
       "cmake_minimum_required\\(VERSION [^)]+\\)"
       "cmake_minimum_required(VERSION 3.10)"
       _pugixml_patched "${_pugixml_content}")

if(_pugixml_content STREQUAL _pugixml_patched)
  message(STATUS "pugixml CMakeLists already uses a modern cmake_minimum_required.")
else()
  file(WRITE "${PUGIXML_CMAKELISTS}" "${_pugixml_patched}")
  message(STATUS "Updated pugixml cmake_minimum_required to 3.10 to silence deprecation warnings.")
endif()
