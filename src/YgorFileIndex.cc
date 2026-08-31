//YgorFileIndex.cc

#include "YgorFileIndex.h"
#include "YgorFileIndexPrivate.hpp"

#include <iterator>
#include <stdexcept>
#include <tuple>
#include <utility>

#include "YgorFilesDirs.h"


namespace {

std::string file_extension(const std::string &filename){
    const auto slash = filename.find_last_of("/\\");
    const auto dot = filename.find_last_of('.');
    if((dot == std::string::npos) || ((slash != std::string::npos) && (dot < slash))){
        return {};
    }
    return filename.substr(dot);
}

} // namespace


bool file_index_measure::operator==(const file_index_measure &rhs) const {
    return std::tie(name, revision, kind, configuration)
        == std::tie(rhs.name, rhs.revision, rhs.kind, rhs.configuration);
}

bool file_index_measure::operator<(const file_index_measure &rhs) const {
    return std::tie(name, revision, kind, configuration)
         < std::tie(rhs.name, rhs.revision, rhs.kind, rhs.configuration);
}

file_index_predicate File_Index_Exists(const file_index_measure &measure){
    return {measure, file_index_predicate_kind::exists, {}};
}

file_index_predicate File_Index_Equal(const file_index_measure &measure, file_index_value value){
    return {measure, file_index_predicate_kind::equal, {std::move(value)}};
}

file_index_predicate File_Index_Between(const file_index_measure &measure,
                                        file_index_value lower,
                                        file_index_value upper){
    return {measure, file_index_predicate_kind::between, {std::move(lower), std::move(upper)}};
}

file_index_predicate File_Index_Overlaps(const file_index_measure &measure,
                                         double lower,
                                         double upper){
    return {measure, file_index_predicate_kind::overlaps, {file_index_range{lower, upper}}};
}

file_index_predicate File_Index_Any_Of(const file_index_measure &measure,
                                       std::vector<file_index_value> values){
    return {measure, file_index_predicate_kind::any_of, std::move(values)};
}


sqlite_file_index::sqlite_file_index(const std::string &database_filename)
    : impl(std::make_unique<implementation>(database_filename)) {}

sqlite_file_index::~sqlite_file_index() = default;
sqlite_file_index::sqlite_file_index(sqlite_file_index &&) noexcept = default;
sqlite_file_index & sqlite_file_index::operator=(sqlite_file_index &&) noexcept = default;

void sqlite_file_index::register_measure(const file_index_measure &measure,
                                         file_index_measure_extractor extractor,
                                         file_index_measure_applicability applicability){
    using namespace ygor_file_index_detail;
    validate_measure(measure);
    if(!extractor){
        throw std::invalid_argument("File index measures require an extractor");
    }

    const auto key = measure_identity(measure);
    const auto existing = impl->registrations.find(key);
    if(existing != impl->registrations.end()){
        if(existing->second.measure.kind != measure.kind){
            throw std::invalid_argument("File index measure identity reused with a different value kind");
        }
        existing->second = implementation::registration{measure, std::move(extractor), std::move(applicability)};
        return;
    }

    impl->ensure_measure(measure);
    impl->registrations.emplace(key,
        implementation::registration{measure, std::move(extractor), std::move(applicability)});
}

std::vector<file_index_update_report>
sqlite_file_index::update_file(const std::string &filename,
                               const std::vector<file_index_measure> &measures,
                               file_index_refresh_policy policy){
    using namespace ygor_file_index_detail;
    const auto path = normalized_filename(filename);
    if(!Does_File_Exist_And_Can_Be_Read(path)){
        throw std::runtime_error("Unable to read file for indexing: " + filename);
    }

    const auto size = static_cast<int64_t>(Size_of_File(path));
    const auto mtime = static_cast<int64_t>(Last_Modification_Time(path));
    if((size < 0) || (mtime < 0)){
        throw std::runtime_error("Unable to stat file for indexing: " + filename);
    }
    const auto file_id = impl->upsert_file(path, size, mtime);

    std::vector<file_index_update_report> reports;
    reports.reserve(measures.size());
    for(const auto &measure : measures){
        file_index_update_report report;
        report.filename = path;
        report.measure = measure;

        const auto registration_it = impl->registrations.find(measure_identity(measure));
        if(registration_it == impl->registrations.end()){
            throw std::invalid_argument("No extractor registered for file index measure: " + measure.name);
        }
        if(registration_it->second.measure.kind != measure.kind){
            throw std::invalid_argument("Registered file index measure has a different value kind");
        }

        const auto measure_id = impl->ensure_measure(measure);
        const auto current_state = impl->state_for(file_id, measure_id, size, mtime);
        const bool should_refresh =
            (policy == file_index_refresh_policy::all)
         || ((policy == file_index_refresh_policy::missing)
             && (current_state == file_index_measure_status::missing))
         || ((policy == file_index_refresh_policy::stale)
             && (current_state != file_index_measure_status::current)
             && (current_state != file_index_measure_status::known_empty));

        if(!should_refresh){
            report.result = file_index_update_result::skipped;
            reports.push_back(std::move(report));
            continue;
        }

        try{
            std::vector<file_index_fact> facts;
            if(!registration_it->second.applicability
            || registration_it->second.applicability(path, measure)){
                facts = registration_it->second.extractor(path, measure);
            }
            for(const auto &fact : facts){
                validate_fact(measure, fact);
            }
            impl->replace_facts(file_id, measure_id, size, mtime, measure, facts);
            report.result = facts.empty() ? file_index_update_result::known_empty
                                          : file_index_update_result::updated;
        }catch(const std::exception &e){
            report.result = file_index_update_result::failed;
            report.error = e.what();
            impl->store_failed_state(file_id, measure_id, size, mtime, report.error);
        }catch(...){
            report.result = file_index_update_result::failed;
            report.error = "Unknown extractor failure";
            impl->store_failed_state(file_id, measure_id, size, mtime, report.error);
        }
        reports.push_back(std::move(report));
    }
    return reports;
}

std::vector<file_index_update_report>
sqlite_file_index::update_directory(const std::string &directory,
                                    const std::vector<file_index_measure> &measures,
                                    file_index_refresh_policy policy){
    std::vector<file_index_update_report> reports;
    const auto files = Get_Recursive_List_of_Full_Path_File_Names_in_Dir(directory);
    for(const auto &filename : files){
        auto file_reports = update_file(filename, measures, policy);
        reports.insert(reports.end(),
                       std::make_move_iterator(file_reports.begin()),
                       std::make_move_iterator(file_reports.end()));
    }
    return reports;
}

void sqlite_file_index::erase_file(const std::string &filename){
    using namespace ygor_file_index_detail;
    sqlite_statement statement(impl->db, "DELETE FROM files WHERE path = ?;");
    statement.bind(1, normalized_filename(filename));
    statement.step();
}


file_index_measure File_Index_FS_Extension_Measure(){
    return {"fs.extension", 1, file_index_value_kind::text, {}};
}

file_index_measure File_Index_FS_Size_Measure(){
    return {"fs.size", 1, file_index_value_kind::integer, {}};
}

file_index_measure File_Index_FS_MTime_Measure(){
    return {"fs.mtime", 1, file_index_value_kind::integer, {}};
}

void Register_Builtin_File_Index_Measures(sqlite_file_index &index){
    index.register_measure(File_Index_FS_Extension_Measure(),
        [](const std::string &filename, const file_index_measure &){
            return std::vector<file_index_fact>{{file_extension(filename)}};
        });

    index.register_measure(File_Index_FS_Size_Measure(),
        [](const std::string &filename, const file_index_measure &){
            const auto size = static_cast<int64_t>(Size_of_File(filename));
            if(size < 0){
                throw std::runtime_error("Unable to determine file size");
            }
            return std::vector<file_index_fact>{{size}};
        });

    index.register_measure(File_Index_FS_MTime_Measure(),
        [](const std::string &filename, const file_index_measure &){
            const auto mtime = static_cast<int64_t>(Last_Modification_Time(filename));
            if(mtime < 0){
                throw std::runtime_error("Unable to determine file modification time");
            }
            return std::vector<file_index_fact>{{mtime}};
        });
}
