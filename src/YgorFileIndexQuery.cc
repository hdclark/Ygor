//YgorFileIndexQuery.cc

#include "YgorFileIndex.h"
#include "YgorFileIndexPrivate.hpp"

#include <cmath>
#include <set>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

#include "YgorFilesDirs.h"


namespace {

using ygor_file_index_detail::sqlite_statement;

struct query_binding {
    std::variant<int64_t, double, std::string> value;
};

void append_binding(std::vector<query_binding> &bindings, int64_t value){
    bindings.push_back({value});
}

void append_binding(std::vector<query_binding> &bindings, double value){
    bindings.push_back({value});
}

void append_binding(std::vector<query_binding> &bindings, const std::string &value){
    bindings.push_back({value});
}

void bind_all(sqlite_statement &statement, const std::vector<query_binding> &bindings){
    int index = 1;
    for(const auto &binding : bindings){
        if(const auto *v = std::get_if<int64_t>(&binding.value); v != nullptr){
            statement.bind(index, *v);
        }else if(const auto *v = std::get_if<double>(&binding.value); v != nullptr){
            statement.bind(index, *v);
        }else{
            statement.bind(index, std::get<std::string>(binding.value));
        }
        ++index;
    }
}

std::string repeat_placeholders(std::size_t count){
    std::string out;
    for(std::size_t i = 0; i < count; ++i){
        if(i != 0){
            out += ',';
        }
        out += '?';
    }
    return out;
}

std::string predicate_condition_sql(const file_index_predicate &predicate,
                                    std::vector<query_binding> &bindings){
    using ygor_file_index_detail::value_matches_kind;
    const auto &values = predicate.values;
    switch(predicate.kind){
        case file_index_predicate_kind::exists:
            if(!values.empty()){
                throw std::invalid_argument("exists predicates do not take values");
            }
            return "1";

        case file_index_predicate_kind::equal:
            if((values.size() != 1) || !value_matches_kind(values[0], predicate.measure.kind)){
                throw std::invalid_argument("equal predicate has incompatible values");
            }
            switch(predicate.measure.kind){
                case file_index_value_kind::integer:
                    append_binding(bindings, std::get<int64_t>(values[0]));
                    return "x.integer_value = ?";
                case file_index_value_kind::real:
                    append_binding(bindings, std::get<double>(values[0]));
                    return "x.real_value = ?";
                case file_index_value_kind::text:
                    append_binding(bindings, std::get<std::string>(values[0]));
                    return "x.text_value = ?";
                case file_index_value_kind::range: {
                    const auto range = std::get<file_index_range>(values[0]);
                    append_binding(bindings, range.min);
                    append_binding(bindings, range.max);
                    return "x.range_min = ? AND x.range_max = ?";
                }
            }
            break;

        case file_index_predicate_kind::between:
            if((values.size() != 2)
            || !value_matches_kind(values[0], predicate.measure.kind)
            || !value_matches_kind(values[1], predicate.measure.kind)){
                throw std::invalid_argument("between predicate has incompatible values");
            }
            if(predicate.measure.kind == file_index_value_kind::integer){
                append_binding(bindings, std::get<int64_t>(values[0]));
                append_binding(bindings, std::get<int64_t>(values[1]));
                return "x.integer_value >= ? AND x.integer_value <= ?";
            }
            if(predicate.measure.kind == file_index_value_kind::real){
                append_binding(bindings, std::get<double>(values[0]));
                append_binding(bindings, std::get<double>(values[1]));
                return "x.real_value >= ? AND x.real_value <= ?";
            }
            throw std::invalid_argument("between predicates require integer or real measures");

        case file_index_predicate_kind::overlaps:
            if((predicate.measure.kind != file_index_value_kind::range)
            || (values.size() != 1)
            || !std::holds_alternative<file_index_range>(values[0])){
                throw std::invalid_argument("overlaps predicates require one range value");
            }else{
                const auto range = std::get<file_index_range>(values[0]);
                if(!std::isfinite(range.min) || !std::isfinite(range.max) || (range.max < range.min)){
                    throw std::invalid_argument("overlaps predicate range is invalid");
                }
                append_binding(bindings, range.max);
                append_binding(bindings, range.min);
                return "x.range_min <= ? AND x.range_max >= ?";
            }

        case file_index_predicate_kind::any_of:
            if(values.empty()){
                return "0";
            }
            if(predicate.measure.kind == file_index_value_kind::range){
                throw std::invalid_argument("any_of predicates do not support range measures");
            }
            for(const auto &value : values){
                if(!value_matches_kind(value, predicate.measure.kind)){
                    throw std::invalid_argument("any_of predicate has incompatible values");
                }
            }
            if(predicate.measure.kind == file_index_value_kind::integer){
                std::string out = "x.integer_value IN (";
                for(std::size_t i = 0; i < values.size(); ++i){
                    if(i != 0){
                        out += ',';
                    }
                    out += std::to_string(std::get<int64_t>(values[i]));
                }
                out += ')';
                return out;
            }
            if(predicate.measure.kind == file_index_value_kind::real){
                for(const auto &value : values){
                    append_binding(bindings, std::get<double>(value));
                }
                return "x.real_value IN (" + repeat_placeholders(values.size()) + ")";
            }
            for(const auto &value : values){
                append_binding(bindings, std::get<std::string>(value));
            }
            return "x.text_value IN (" + repeat_placeholders(values.size()) + ")";
    }
    throw std::logic_error("Unknown file index predicate kind");
}

} // namespace


std::vector<std::string>
sqlite_file_index::query(const file_index_query &query_in,
                         file_index_completeness_policy policy){
    if(query_in.predicates.empty()){
        sqlite_statement statement(impl->db, "SELECT path FROM files ORDER BY path;");
        std::vector<std::string> out;
        while(statement.step()){
            out.push_back(statement.column_text(0));
        }
        return out;
    }

    std::vector<int64_t> measure_ids;
    measure_ids.reserve(query_in.predicates.size());
    for(const auto &predicate : query_in.predicates){
        measure_ids.push_back(impl->ensure_measure(predicate.measure));
    }

    if(policy == file_index_completeness_policy::require_complete){
        std::set<int64_t> unique_measure_ids(measure_ids.begin(), measure_ids.end());
        for(const auto measure_id : unique_measure_ids){
            sqlite_statement statement(impl->db,
                "SELECT COUNT(*) FROM files WHERE NOT EXISTS ("
                " SELECT 1 FROM file_measure_state s"
                " WHERE s.file_id = files.file_id AND s.measure_id = ?"
                " AND s.status IN (1, 2)"
                " AND s.indexed_size = files.size AND s.indexed_mtime = files.mtime"
                ");");
            statement.bind(1, measure_id);
            if(!statement.step() || (statement.column_int64(0) != 0)){
                throw std::runtime_error("File index query requires complete measures, but the index is incomplete");
            }
        }
        policy = file_index_completeness_policy::known_only;
    }

    std::string sql = "SELECT DISTINCT files.path FROM files WHERE ";
    std::vector<query_binding> bindings;

    for(std::size_t i = 0; i < query_in.predicates.size(); ++i){
        if(i != 0){
            sql += " AND ";
        }
        const auto &predicate = query_in.predicates[i];
        const auto measure_id = measure_ids[i];

        const std::string current =
            "EXISTS (SELECT 1 FROM file_measure_state s WHERE s.file_id = files.file_id"
            " AND s.measure_id = ? AND s.status IN (1, 2)"
            " AND s.indexed_size = files.size AND s.indexed_mtime = files.mtime)";

        std::vector<query_binding> fact_bindings;
        const auto condition = predicate_condition_sql(predicate, fact_bindings);
        const std::string match =
            "EXISTS (SELECT 1 FROM facts x WHERE x.file_id = files.file_id"
            " AND x.measure_id = ? AND (" + condition + "))";

        sql += '(';
        if(policy == file_index_completeness_policy::include_unknown){
            sql += "(NOT " + current + ") OR " + match;
        }else{
            sql += current + " AND " + match;
        }
        append_binding(bindings, measure_id);
        append_binding(bindings, measure_id);
        bindings.insert(bindings.end(), fact_bindings.begin(), fact_bindings.end());
        sql += ')';
    }
    sql += " ORDER BY files.path;";

    sqlite_statement statement(impl->db, sql);
    bind_all(statement, bindings);
    std::vector<std::string> out;
    while(statement.step()){
        out.push_back(statement.column_text(0));
    }
    return out;
}

file_index_measure_status sqlite_file_index::status(const std::string &filename,
                                                    const file_index_measure &measure) const {
    using namespace ygor_file_index_detail;
    const auto path = normalized_filename(filename);
    sqlite_statement file_statement(impl->db,
        "SELECT file_id, size, mtime FROM files WHERE path = ?;");
    file_statement.bind(1, path);
    if(!file_statement.step()){
        return file_index_measure_status::missing;
    }

    auto size = file_statement.column_int64(1);
    auto mtime = file_statement.column_int64(2);
    if(Does_File_Exist_And_Can_Be_Read(path)){
        const auto disk_size = static_cast<int64_t>(Size_of_File(path));
        const auto disk_mtime = static_cast<int64_t>(Last_Modification_Time(path));
        if((disk_size >= 0) && (disk_mtime >= 0)){
            size = disk_size;
            mtime = disk_mtime;
        }
    }
    const auto measure_id = impl->ensure_measure(measure);
    return impl->state_for(file_statement.column_int64(0), measure_id, size, mtime);
}

double sqlite_file_index::completion(const file_index_measure &measure) const {
    sqlite_statement total(impl->db, "SELECT COUNT(*) FROM files;");
    if(!total.step()){
        throw std::runtime_error("Unable to count indexed files");
    }
    const auto total_count = total.column_int64(0);
    if(total_count == 0){
        return 1.0;
    }

    const auto measure_id = impl->ensure_measure(measure);
    sqlite_statement current(impl->db,
        "SELECT COUNT(*) FROM files WHERE EXISTS ("
        " SELECT 1 FROM file_measure_state s"
        " WHERE s.file_id = files.file_id AND s.measure_id = ?"
        " AND s.status IN (1, 2)"
        " AND s.indexed_size = files.size AND s.indexed_mtime = files.mtime"
        ");");
    current.bind(1, measure_id);
    if(!current.step()){
        throw std::runtime_error("Unable to count current file index measures");
    }
    return static_cast<double>(current.column_int64(0)) / static_cast<double>(total_count);
}
