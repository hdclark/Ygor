//YgorFileIndex.h

#ifndef YGOR_FILE_INDEX_HDR_GRD_H
#define YGOR_FILE_INDEX_HDR_GRD_H

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <variant>
#include <vector>


enum class file_index_value_kind {
    integer,
    real,
    text,
    range
};

struct file_index_range {
    double min = 0.0;
    double max = 0.0;
};

using file_index_value = std::variant<int64_t, double, std::string, file_index_range>;

struct file_index_measure {
    std::string name;
    int64_t revision = 1;
    file_index_value_kind kind = file_index_value_kind::text;
    std::string configuration;

    bool operator==(const file_index_measure &rhs) const;
    bool operator<(const file_index_measure &rhs) const;
};

struct file_index_fact {
    file_index_value value;
};

enum class file_index_measure_status {
    missing,
    current,
    stale,
    failed,
    known_empty
};

enum class file_index_refresh_policy {
    missing,
    stale,
    all
};

enum class file_index_completeness_policy {
    known_only,
    include_unknown,
    require_complete
};

enum class file_index_predicate_kind {
    exists,
    equal,
    between,
    overlaps,
    any_of
};

struct file_index_predicate {
    file_index_measure measure;
    file_index_predicate_kind kind = file_index_predicate_kind::exists;
    std::vector<file_index_value> values;
};

struct file_index_query {
    std::vector<file_index_predicate> predicates; // Predicates are ANDed.
};

enum class file_index_update_result {
    skipped,
    updated,
    known_empty,
    failed
};

struct file_index_update_report {
    std::string filename;
    file_index_measure measure;
    file_index_update_result result = file_index_update_result::skipped;
    std::string error;
};

using file_index_measure_extractor = std::function<std::vector<file_index_fact>(
    const std::string &filename,
    const file_index_measure &measure)>;

using file_index_measure_applicability = std::function<bool(
    const std::string &filename,
    const file_index_measure &measure)>;


file_index_predicate File_Index_Exists(const file_index_measure &measure);
file_index_predicate File_Index_Equal(const file_index_measure &measure, file_index_value value);
file_index_predicate File_Index_Between(const file_index_measure &measure,
                                        file_index_value lower,
                                        file_index_value upper);
file_index_predicate File_Index_Overlaps(const file_index_measure &measure,
                                         double lower,
                                         double upper);
file_index_predicate File_Index_Any_Of(const file_index_measure &measure,
                                       std::vector<file_index_value> values);


class sqlite_file_index {
    public:
        explicit sqlite_file_index(const std::string &database_filename);
        ~sqlite_file_index();

        sqlite_file_index(const sqlite_file_index &) = delete;
        sqlite_file_index & operator=(const sqlite_file_index &) = delete;
        sqlite_file_index(sqlite_file_index &&) noexcept;
        sqlite_file_index & operator=(sqlite_file_index &&) noexcept;

        void register_measure(const file_index_measure &measure,
                              file_index_measure_extractor extractor,
                              file_index_measure_applicability applicability = {});

        std::vector<file_index_update_report>
        update_file(const std::string &filename,
                    const std::vector<file_index_measure> &measures,
                    file_index_refresh_policy policy = file_index_refresh_policy::stale);

        std::vector<file_index_update_report>
        update_directory(const std::string &directory,
                         const std::vector<file_index_measure> &measures,
                         file_index_refresh_policy policy = file_index_refresh_policy::stale);

        std::vector<std::string>
        query(const file_index_query &query,
              file_index_completeness_policy policy = file_index_completeness_policy::known_only);

        file_index_measure_status status(const std::string &filename,
                                         const file_index_measure &measure) const;

        double completion(const file_index_measure &measure) const;

        void erase_file(const std::string &filename);

    private:
        class implementation;
        std::unique_ptr<implementation> impl;
};


file_index_measure File_Index_FS_Extension_Measure();
file_index_measure File_Index_FS_Size_Measure();
file_index_measure File_Index_FS_MTime_Measure();

void Register_Builtin_File_Index_Measures(sqlite_file_index &index);


#endif
