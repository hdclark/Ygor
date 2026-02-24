

configure_file(YgorDefinitions.h.in 
               ${CMAKE_CURRENT_BINARY_DIR}/YgorDefinitions.h)

FILE(GLOB ygor_sources "*cc")
FILE(GLOB ygor_headers "*.h"
                       ${CMAKE_CURRENT_BINARY_DIR}/YgorDefinitions.h)

list(FIND ygor_sources "${CMAKE_CURRENT_SOURCE_DIR}/YgorImagesDithering.cc" ygor_images_dithering_src_idx)
if(ygor_images_dithering_src_idx EQUAL -1)
    list(APPEND ygor_sources "${CMAKE_CURRENT_SOURCE_DIR}/YgorImagesDithering.cc")
endif()

list(FIND ygor_headers "${CMAKE_CURRENT_SOURCE_DIR}/YgorImagesDithering.h" ygor_images_dithering_hdr_idx)
if(ygor_images_dithering_hdr_idx EQUAL -1)
    list(APPEND ygor_headers "${CMAKE_CURRENT_SOURCE_DIR}/YgorImagesDithering.h")
endif()

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
    "${STD_FS_LIB}"
    Threads::Threads
)
if(WITH_GNU_GSL)
target_link_libraries(ygor
    "$<$<BOOL:${WITH_GNU_GSL}>:${GNU_GSL_LIBRARIES}>"
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
