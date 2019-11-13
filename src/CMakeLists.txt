

configure_file(YgorDefinitions.h.in 
               ${CMAKE_CURRENT_SOURCE_DIR}/YgorDefinitions.h)

FILE(GLOB ygor_sources "*cc")
FILE(GLOB ygor_headers "*.h")

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
    Threads::Threads
)
if(WITH_GNU_GSL)
target_link_libraries(ygor
    gsl
    gslcblas
)
endif()

# Installation info.
install(TARGETS 
    ygor
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
)

install(FILES 
    ${ygor_headers} DESTINATION include
)

