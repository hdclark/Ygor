

add_executable(fits_replace_nans
    Replace_NaNs.cc
)
target_include_directories(fits_replace_nans
    SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(fits_replace_nans
    ygor
    m
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
)


add_executable(getredditurls
    GetRedditUrls.cc
)
target_include_directories(getredditurls
    SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(getredditurls
    ygor
    htmlcxx
    m
)

add_executable(getredditmusic
    GetRedditMusic.cc
)
target_include_directories(getredditmusic
    SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(getredditmusic
    ygor
    htmlcxx
    m
)

add_executable(getredditmovies
    GetRedditMovies.cc
)
target_include_directories(getredditmovies
    SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(getredditmovies
    ygor
    htmlcxx
    m
)

add_executable(getvideosifturls
    GetVideosiftUrls.cc
)
target_include_directories(getvideosifturls
    SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(getvideosifturls
    ygor
    htmlcxx
    m
)

add_executable(getvimeourls
    GetVimeoUrls.cc
)
target_include_directories(getvimeourls
    SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(getvimeourls
    ygor
    htmlcxx
    m
)


add_executable(geturls
    GetUrls.cc
)
target_include_directories(geturls
    SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(geturls
    ygor
    htmlcxx
    m
)


add_executable(getnutritionfactsurls
    GetNutritionfactsUrls.cc
)
target_include_directories(getnutritionfactsurls
    SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(getnutritionfactsurls
    ygor
    htmlcxx
    m
)


install(TARGETS 
    fits_replace_nans
    twot_pvalue
    regex_tester
    getredditurls         
    getredditmusic        
    getredditmovies       
    getvideosifturls      
    getvimeourls          
    geturls               
    getnutritionfactsurls 
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
)


