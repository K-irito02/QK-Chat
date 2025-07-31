# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\QKChatClient_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\QKChatClient_autogen.dir\\ParseCache.txt"
  "QKChatClient_autogen"
  )
endif()
