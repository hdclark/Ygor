#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

#include <YgorFileIndex.h>
#include <YgorFileIndexGPX.h>
#include <YgorFilesDirs.h>

#include "doctest/doctest.h"


namespace {

struct temporary_files {
    std::vector<std::string> filenames;

    ~temporary_files(){
        for(const auto &filename : filenames){
            RemoveFile(filename);
        }
    }

    std::string make(const std::string &suffix){
        auto filename = Get_Unique_Filename("ygor_file_index_", 16, suffix);
        filenames.push_back(filename);
        return filename;
    }
};

} // namespace


TEST_CASE( "file index generic measures and queries" ){
    temporary_files temporary;
    const auto database = temporary.make(".sqlite");
    const auto gpx_file = temporary.make(".gpx");
    const auto text_file = temporary.make(".txt");
    REQUIRE(OverwriteStringToFile("alpha", gpx_file));
    REQUIRE(OverwriteStringToFile("bravo", text_file));

    sqlite_file_index index(database);
    Register_Builtin_File_Index_Measures(index);

    const auto extension = File_Index_FS_Extension_Measure();
    const auto size = File_Index_FS_Size_Measure();
    index.update_file(gpx_file, {extension, size});
    index.update_file(text_file, {extension, size});

    const file_index_query gpx_query{{File_Index_Equal(extension, std::string(".gpx"))}};
    const auto matches = index.query(gpx_query);
    REQUIRE(matches.size() == 1);
    REQUIRE(matches.front() == Fully_Expand_Filename(gpx_file));

    const file_index_query size_query{{File_Index_Between(size, int64_t{5}, int64_t{5})}};
    REQUIRE(index.query(size_query).size() == 2);
    REQUIRE(index.completion(extension) == doctest::Approx(1.0));
}

TEST_CASE( "file index permits gradual measure population" ){
    temporary_files temporary;
    const auto database = temporary.make(".sqlite");
    const auto first_file = temporary.make(".dat");
    const auto second_file = temporary.make(".dat");
    REQUIRE(OverwriteStringToFile("a", first_file));
    REQUIRE(OverwriteStringToFile("b", second_file));

    sqlite_file_index index(database);
    Register_Builtin_File_Index_Measures(index);
    const file_index_measure first_byte{"test.first_byte", 1, file_index_value_kind::integer, {}};
    index.register_measure(first_byte,
        [](const std::string &filename, const file_index_measure &){
            const auto content = LoadBinaryFileToString(filename);
            if(content.empty()){
                return std::vector<file_index_fact>{};
            }
            return std::vector<file_index_fact>{{static_cast<int64_t>(static_cast<unsigned char>(content.front()))}};
        });

    index.update_file(first_file, {first_byte});
    index.update_file(second_file, {File_Index_FS_Extension_Measure()}); // Discover only.

    const file_index_query query{{File_Index_Equal(first_byte, int64_t{'a'})}};
    REQUIRE(index.query(query, file_index_completeness_policy::known_only).size() == 1);
    REQUIRE(index.query(query, file_index_completeness_policy::include_unknown).size() == 2);
    REQUIRE_THROWS(index.query(query, file_index_completeness_policy::require_complete));
    REQUIRE(index.completion(first_byte) == doctest::Approx(0.5));

    index.update_file(second_file, {first_byte});
    REQUIRE(index.completion(first_byte) == doctest::Approx(1.0));
    REQUIRE(index.query(query, file_index_completeness_policy::require_complete).size() == 1);

    REQUIRE(OverwriteStringToFile("alpha", first_file));
    REQUIRE(index.status(first_file, first_byte) == file_index_measure_status::stale);
}

TEST_CASE( "GPX file index measures rasterize sparse track segments" ){
    temporary_files temporary;
    const auto database = temporary.make(".sqlite");
    const auto track_file = temporary.make(".gpx");

    const std::string gpx =
        "<?xml version=\"1.0\"?>\n"
        "<gpx version=\"1.1\">\n"
        "  <trk><trkseg>\n"
        "    <trkpt lat=\"0.1\" lon=\"-1.1\"/>\n"
        "    <trkpt lat=\"0.1\" lon=\"1.1\"/>\n"
        "  </trkseg></trk>\n"
        "</gpx>\n";
    REQUIRE(OverwriteStringToFile(gpx, track_file));

    sqlite_file_index index(database);
    Register_GPX_File_Index_Measures(index, 1.0);
    Register_GPX_File_Index_Measures(index, 0.5); // A second configured measure can coexist.
    const auto latitude = File_Index_GPX_Latitude_Extent_Measure();
    const auto longitude = File_Index_GPX_Longitude_Extent_Measure();
    const auto coverage = File_Index_GPX_Coverage_Cell_Measure(1.0);
    const auto fine_coverage = File_Index_GPX_Coverage_Cell_Measure(0.5);
    index.update_file(track_file, {latitude, longitude, coverage, fine_coverage});

    const file_index_query bounds_query{{
        File_Index_Overlaps(latitude, 0.0, 0.2),
        File_Index_Overlaps(longitude, -0.25, 0.25)
    }};
    REQUIRE(index.query(bounds_query).size() == 1);

    // Neither GPX vertex is in this AOI. The result therefore verifies that the
    // segment itself was walked through the coverage grid.
    const file_index_query coverage_query{{
        GPX_Coverage_Predicate(0.0, 0.5, -0.25, 0.25, 1.0)
    }};
    REQUIRE(index.query(coverage_query).size() == 1);
    REQUIRE(index.query({{GPX_Coverage_Predicate(0.0, 0.5, -0.25, 0.25, 0.5)}}).size() == 1);
}

TEST_CASE( "GPX longitude extents remain useful across the antimeridian" ){
    temporary_files temporary;
    const auto database = temporary.make(".sqlite");
    const auto track_file = temporary.make(".gpx");
    REQUIRE(OverwriteStringToFile(
        "<gpx><trk><trkseg>"
        "<trkpt lat=\"10\" lon=\"179.5\"/>"
        "<trkpt lat=\"10\" lon=\"-179.5\"/>"
        "</trkseg></trk></gpx>", track_file));

    sqlite_file_index index(database);
    Register_GPX_File_Index_Measures(index, 1.0);
    const auto longitude = File_Index_GPX_Longitude_Extent_Measure();
    index.update_file(track_file, {longitude});

    REQUIRE(index.query({{File_Index_Overlaps(longitude, 179.0, 180.0)}}).size() == 1);
    REQUIRE(index.query({{File_Index_Overlaps(longitude, -180.0, -179.0)}}).size() == 1);
    REQUIRE(index.query({{File_Index_Overlaps(longitude, -1.0, 1.0)}}).empty());
}
