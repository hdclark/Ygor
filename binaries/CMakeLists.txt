

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

add_executable(ygor_stochastic_forest_train
    Ygor_Stochastic_Forest_Train.cc
)
target_include_directories(ygor_stochastic_forest_train
    SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(ygor_stochastic_forest_train
    ygor
    m
    Threads::Threads
)

add_executable(ygor_stochastic_forest_predict
    Ygor_Stochastic_Forest_Predict.cc
)
target_include_directories(ygor_stochastic_forest_predict
    SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(ygor_stochastic_forest_predict
    ygor
    m
    Threads::Threads
)

add_executable(ygor_conditional_forest_train
    Ygor_Conditional_Forest_Train.cc
)
target_include_directories(ygor_conditional_forest_train
    SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(ygor_conditional_forest_train
    ygor
    m
    Threads::Threads
)

add_executable(ygor_conditional_forest_predict
    Ygor_Conditional_Forest_Predict.cc
)
target_include_directories(ygor_conditional_forest_predict
    SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(ygor_conditional_forest_predict
    ygor
    m
    Threads::Threads
)

add_executable(ygor_ci_tree_train
    Ygor_CI_Tree_Train.cc
)
target_include_directories(ygor_ci_tree_train
    SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(ygor_ci_tree_train
    ygor
    m
    Threads::Threads
)

add_executable(ygor_ci_tree_predict
    Ygor_CI_Tree_Predict.cc
)
target_include_directories(ygor_ci_tree_predict
    SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(ygor_ci_tree_predict
    ygor
    m
    Threads::Threads
)

install(TARGETS fits_replace_nans
                twot_pvalue
                regex_tester
                parse_TAR_files
                ygor_stochastic_forest_train
                ygor_stochastic_forest_predict
                ygor_conditional_forest_train
                ygor_conditional_forest_predict
                ygor_ci_tree_train
                ygor_ci_tree_predict
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

