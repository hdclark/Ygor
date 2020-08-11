
#include <limits>
#include <utility>
#include <iostream>
#include <filesystem>
#include <list>

#include <YgorFilesDirs.h>

#include "doctest/doctest.h"


TEST_CASE( "directory free functions" ){

    SUBCASE("Does_Dir_Exist_And_Can_Be_Read"){
        REQUIRE( Does_Dir_Exist_And_Can_Be_Read(".") );
        REQUIRE( Does_Dir_Exist_And_Can_Be_Read("/") );

        const std::string f_missing("nothing with this name should exist in the filesystem");
        std::filesystem::remove_all( std::filesystem::path(f_missing) ); // Setup.
        REQUIRE( !std::filesystem::exists(std::filesystem::path(f_missing)) ); // Precondition.
        REQUIRE( !Does_Dir_Exist_And_Can_Be_Read(f_missing) );
        
        const std::string f("files_dirs_test_file");
        REQUIRE( TouchFile(f) );
        REQUIRE( std::filesystem::exists(std::filesystem::path(f)) ); // Precondition.
        REQUIRE( !Does_Dir_Exist_And_Can_Be_Read(f) );
        REQUIRE( RemoveFile(f) ); // Cleanup.
        REQUIRE( !Does_Dir_Exist_And_Can_Be_Read(f) ); // Postcondition.
    }

    SUBCASE("Create_Dir_and_Necessary_Parents"){
        const std::string wdir("files_dirs_working_dir");
        std::filesystem::remove_all( std::filesystem::path(wdir) ); // Setup.
        REQUIRE( !Does_Dir_Exist_And_Can_Be_Read(wdir) ); // Precondition.
        REQUIRE( Create_Dir_and_Necessary_Parents(wdir) ); // Setup.
        REQUIRE( Does_Dir_Exist_And_Can_Be_Read(wdir) );
        REQUIRE( std::filesystem::remove( std::filesystem::path(wdir)) ); // Cleanup.
        REQUIRE( !Does_Dir_Exist_And_Can_Be_Read(wdir) ); // Postcondition.
    }

    SUBCASE("Create_Dir_and_Necessary_Parents with nested directories"){
        const std::string wdir("files_dirs_working_dir/nested_lvl1/nested_lvl2");
        std::filesystem::remove_all( std::filesystem::path(wdir) ); // Setup.
        REQUIRE( !Does_Dir_Exist_And_Can_Be_Read(wdir) ); // Precondition.
        REQUIRE( Create_Dir_and_Necessary_Parents(wdir) ); // Setup.
        REQUIRE( Does_Dir_Exist_And_Can_Be_Read(wdir) );
        REQUIRE( std::filesystem::remove_all( std::filesystem::path("files_dirs_working_dir")) == 3 ); // Cleanup.
        REQUIRE( !Does_Dir_Exist_And_Can_Be_Read(wdir) ); // Postcondition.
    }

    SUBCASE("directory lists"){
        const std::string wdir("files_dirs_working_dir/");
        const std::string f1("file1");
        const std::string f2("file2");
        const std::string d1("dir1/");
        const std::string f3(d1 + "file3");
        const std::string d2(d1 + "dir2/");

        std::filesystem::remove_all( std::filesystem::path(wdir) ); // Setup.
        REQUIRE( !Does_Dir_Exist_And_Can_Be_Read(wdir) ); // Precondition.
        REQUIRE( Create_Dir_and_Necessary_Parents(wdir) ); // Setup.
        REQUIRE( TouchFile(wdir + f1) ); // Setup.
        REQUIRE( TouchFile(wdir + f2) ); // Setup.
        REQUIRE( Create_Dir_and_Necessary_Parents(wdir + d1) ); // Setup.
        REQUIRE( Create_Dir_and_Necessary_Parents(wdir + d2) ); // Setup.
        REQUIRE( TouchFile(wdir + f3) ); // Setup.

        SUBCASE("Get_List_of_File_and_Dir_Names_in_Dir and Append_List_of_File_and_Dir_Names_in_Dir"){
            auto wdir_files_dirs_names = Get_List_of_File_and_Dir_Names_in_Dir(wdir);
            REQUIRE(wdir_files_dirs_names.size() == 3);

            auto d1_files_dirs_names = Get_List_of_File_and_Dir_Names_in_Dir(wdir + d1);
            REQUIRE(d1_files_dirs_names.size() == 2);

            auto d2_files_dirs_names = Get_List_of_File_and_Dir_Names_in_Dir(wdir + d2);
            REQUIRE(d2_files_dirs_names.size() == 0);

            auto empty_names = Get_List_of_File_and_Dir_Names_in_Dir(wdir + "nonexistent");
            REQUIRE(empty_names.empty());
        }

        SUBCASE("Get_List_of_Full_Path_File_and_Dir_Names_in_Dir and Append_List_of_Full_Path_File_and_Dir_Names_in_Dir"){
            auto wdir_files_dirs_names = Get_List_of_Full_Path_File_and_Dir_Names_in_Dir(wdir);
            REQUIRE(wdir_files_dirs_names.size() == 3);

            auto d1_files_dirs_names = Get_List_of_Full_Path_File_and_Dir_Names_in_Dir(wdir + d1);
            REQUIRE(d1_files_dirs_names.size() == 2);

            auto d2_files_dirs_names = Get_List_of_Full_Path_File_and_Dir_Names_in_Dir(wdir + d2);
            REQUIRE(d2_files_dirs_names.size() == 0);

            auto empty_names = Get_List_of_Full_Path_File_and_Dir_Names_in_Dir(wdir + "nonexistent");
            REQUIRE(empty_names.empty());
        }

        SUBCASE("Get_List_of_File_Names_in_Dir and Append_List_of_File_Names_in_Dir"){
            auto wdir_files_dirs_names = Get_List_of_File_Names_in_Dir(wdir);
            REQUIRE(wdir_files_dirs_names.size() == 2);

            auto d1_files_dirs_names = Get_List_of_File_Names_in_Dir(wdir + d1);
            REQUIRE(d1_files_dirs_names.size() == 1);

            auto d2_files_dirs_names = Get_List_of_File_Names_in_Dir(wdir + d2);
            REQUIRE(d2_files_dirs_names.size() == 0);

            auto empty_names = Get_List_of_File_Names_in_Dir(wdir + "nonexistent");
            REQUIRE(empty_names.empty());
        }

        SUBCASE("Get_List_of_Full_Path_File_Names_in_Dir and Append_List_of_Full_Path_File_Names_in_Dir"){
            auto wdir_files_dirs_names = Get_List_of_Full_Path_File_Names_in_Dir(wdir);
            REQUIRE(wdir_files_dirs_names.size() == 2);

            auto d1_files_dirs_names = Get_List_of_Full_Path_File_Names_in_Dir(wdir + d1);
            REQUIRE(d1_files_dirs_names.size() == 1);

            auto d2_files_dirs_names = Get_List_of_Full_Path_File_Names_in_Dir(wdir + d2);
            REQUIRE(d2_files_dirs_names.size() == 0);

            auto empty_names = Get_List_of_Full_Path_File_Names_in_Dir(wdir + "nonexistent");
            REQUIRE(empty_names.empty());
        }

        SUBCASE("Get_Recursive_List_of_Full_Path_File_Names_in_Dir"){
            auto all_files_dirs_names = Get_Recursive_List_of_Full_Path_File_Names_in_Dir(wdir);
            REQUIRE(all_files_dirs_names.size() == 3);
        }

        REQUIRE( std::filesystem::remove_all( std::filesystem::path(wdir)) != 0 ); // Cleanup.
    }
}

