SET(SRCS
      fd.cpp
    )
if (NOT CMAKE_TESTING_ENABLED)
  list(APPEND SRCS
    file_fd.cpp
  )
endif()
add_library(posix OBJECT ${SRCS})
