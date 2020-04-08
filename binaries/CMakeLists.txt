

add_executable(fits_replace_nans
    Replace_NaNs.cc
)
target_include_directories(fits_replace_nans
    SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(fits_replace_nans
    ygor
    m
    Threads::Threads
)

add_executable(twot_pvalue
    TwoT_PValue.cc
)
target_include_directories(twot_pvalue
    SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(twot_pvalue
    ygor
    m
    Threads::Threads
)

add_executable(regex_tester
    Regex_Tester.cc
)
target_include_directories(regex_tester
    SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(regex_tester
    ygor
    m
    Threads::Threads
)

add_executable(parse_TAR_files
    Parse_TAR_Files.cc
)
target_include_directories(parse_TAR_files
    SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(parse_TAR_files
    ygor
    m
    Threads::Threads
)

install(TARGETS 
    fits_replace_nans
    twot_pvalue
    regex_tester
    parse_TAR_files
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
)


