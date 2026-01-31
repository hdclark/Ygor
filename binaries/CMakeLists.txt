
# Helper function to create a binary target
function(ygor_add_executable target_name source_file)
    add_executable(${target_name} ${source_file})

    target_link_libraries(${target_name}
        PRIVATE
            Ygor::ygor
    )

    set_target_properties(${target_name} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )
endfunction()

# Create executables
ygor_add_executable(fits_replace_nans Replace_NaNs.cc)
ygor_add_executable(twot_pvalue TwoT_PValue.cc)
ygor_add_executable(regex_tester Regex_Tester.cc)
ygor_add_executable(parse_TAR_files Parse_TAR_Files.cc)

# Install targets
install(TARGETS
    fits_replace_nans
    twot_pvalue
    regex_tester
    parse_TAR_files
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)


