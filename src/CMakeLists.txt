

configure_file(YgorDefinitions.h.in 
               ${CMAKE_CURRENT_BINARY_DIR}/YgorDefinitions.h)

FILE(GLOB ygor_sources "*cc")
FILE(GLOB ygor_headers "*.h"
                       ${CMAKE_CURRENT_BINARY_DIR}/YgorDefinitions.h)

add_library(ygor
    ${ygor_sources}
    External/SpookyHash/SpookyV2.cpp
    External/MD5/md5.cc
)
#set_target_properties(ygor PROPERTIES
#    POSITION_INDEPENDENT_CODE TRUE
#)
target_include_directories(ygor
    SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ygor
    m
    stdc++fs # Workaround for gcc v8 -- can be removed when newer compilers are expected.
    Threads::Threads
)
if(WITH_GNU_GSL)
target_link_libraries(ygor
    gsl
    gslcblas
)
endif()

# Installation info.
install(TARGETS ygor
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES ${ygor_headers}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

