set(IJBI_HEADER_FILES console.h)

set(IJBI_SOURCE_FILES
    console.cpp
    main.cpp
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_executable(ijbi ${IJBI_HEADER_FILES} ${IJBI_SOURCE_FILES})
target_link_libraries(ijbi jbi)
