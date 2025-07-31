# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\QKChatServer_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\QKChatServer_autogen.dir\\ParseCache.txt"
  "QKChatServer_autogen"
  )
endif()
