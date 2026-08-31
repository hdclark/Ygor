#ifndef YGOR_FILE_INDEX_PRIVATE_HPP
#define YGOR_FILE_INDEX_PRIVATE_HPP

#include "YgorFileIndex.h"

#include <sqlite3.h>

#include <cmath>
#include <map>
#include <stdexcept>
#include <string>
#include <variant>

#include "YgorFilesDirs.h"


namespace ygor_file_index_detail {

constexpr int64_t state_current = 1;
constexpr int64_t state_empty = 2;
constexpr int64_t state_failed = 3;

inline std::string sqlite_error(sqlite3 *db, const std::string &context){
    return context + ": " + ((db == nullptr) ? std::string("unknown sqlite error")
                                             : std::string(sqlite3_errmsg(db)));
}

inline void sqlite_exec(sqlite3 *db, const std::string &sql){
    char *message = nullptr;
    const auto rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &message);
    if(rc != SQLITE_OK){
        std::string detail = (message == nullptr) ? sqlite3_errmsg(db) : message;
        sqlite3_free(message);
        throw std::runtime_error("SQLite error: " + detail);
    }
}

class sqlite_statement {
    public:
        sqlite_statement(sqlite3 *db_in, const std::string &sql) : db(db_in) {
            if(sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK){
                const auto message = sqlite_error(db, "Unable to prepare SQLite statement");
                if(stmt != nullptr){
                    sqlite3_finalize(stmt);
                    stmt = nullptr;
                }
                throw std::runtime_error(message);
            }
        }

        ~sqlite_statement(){
            if(stmt != nullptr){
                sqlite3_finalize(stmt);
            }
        }

        sqlite_statement(const sqlite_statement &) = delete;
        sqlite_statement & operator=(const sqlite_statement &) = delete;

        void bind(int index, int64_t value){
            if(sqlite3_bind_int64(stmt, index, value) != SQLITE_OK){
                throw std::runtime_error(sqlite_error(db, "Unable to bind SQLite integer"));
            }
        }

        void bind(int index, double value){
            if(sqlite3_bind_double(stmt, index, value) != SQLITE_OK){
                throw std::runtime_error(sqlite_error(db, "Unable to bind SQLite real"));
            }
        }

        void bind(int index, const std::string &value){
            if(sqlite3_bind_text(stmt, index, value.c_str(), static_cast<int>(value.size()), SQLITE_TRANSIENT) != SQLITE_OK){
                throw std::runtime_error(sqlite_error(db, "Unable to bind SQLite text"));
            }
        }

        bool step(){
            const auto rc = sqlite3_step(stmt);
            if(rc == SQLITE_ROW){
                return true;
            }
            if(rc == SQLITE_DONE){
                return false;
            }
            throw std::runtime_error(sqlite_error(db, "Unable to execute SQLite statement"));
        }

        void reset(){
            if(sqlite3_reset(stmt) != SQLITE_OK){
                throw std::runtime_error(sqlite_error(db, "Unable to reset SQLite statement"));
            }
            if(sqlite3_clear_bindings(stmt) != SQLITE_OK){
                throw std::runtime_error(sqlite_error(db, "Unable to clear SQLite bindings"));
            }
        }

        int64_t column_int64(int column) const {
            return sqlite3_column_int64(stmt, column);
        }

        std::string column_text(int column) const {
            const auto *text = sqlite3_column_text(stmt, column);
            return (text == nullptr) ? std::string{} : reinterpret_cast<const char *>(text);
        }

    private:
        sqlite3 *db = nullptr;
        sqlite3_stmt *stmt = nullptr;
};

inline int64_t kind_to_integer(file_index_value_kind kind){
    switch(kind){
        case file_index_value_kind::integer: return 1;
        case file_index_value_kind::real:    return 2;
        case file_index_value_kind::text:    return 3;
        case file_index_value_kind::range:   return 4;
    }
    throw std::logic_error("Unknown file index value kind");
}

inline bool value_matches_kind(const file_index_value &value, file_index_value_kind kind){
    switch(kind){
        case file_index_value_kind::integer: return std::holds_alternative<int64_t>(value);
        case file_index_value_kind::real:    return std::holds_alternative<double>(value);
        case file_index_value_kind::text:    return std::holds_alternative<std::string>(value);
        case file_index_value_kind::range:   return std::holds_alternative<file_index_range>(value);
    }
    return false;
}

inline void validate_measure(const file_index_measure &measure){
    if(measure.name.empty()){
        throw std::invalid_argument("File index measure names cannot be empty");
    }
    if(measure.revision < 0){
        throw std::invalid_argument("File index measure revisions cannot be negative");
    }
}

inline void validate_fact(const file_index_measure &measure, const file_index_fact &fact){
    if(!value_matches_kind(fact.value, measure.kind)){
        throw std::invalid_argument("File index extractor emitted a fact with the wrong value kind");
    }
    if(const auto *range = std::get_if<file_index_range>(&fact.value); range != nullptr){
        if(!std::isfinite(range->min) || !std::isfinite(range->max) || (range->max < range->min)){
            throw std::invalid_argument("File index extractor emitted an invalid range");
        }
    }
    if(const auto *real = std::get_if<double>(&fact.value); real != nullptr){
        if(!std::isfinite(*real)){
            throw std::invalid_argument("File index extractor emitted a non-finite real value");
        }
    }
}

inline std::string measure_identity(const file_index_measure &measure){
    std::string out;
    out.reserve(measure.name.size() + measure.configuration.size() + 32);
    out += measure.name;
    out.push_back('\0');
    out += std::to_string(measure.revision);
    out.push_back('\0');
    out += measure.configuration;
    return out;
}

inline std::string normalized_filename(const std::string &filename){
    const auto expanded = Fully_Expand_Filename(filename);
    return expanded.empty() ? filename : expanded;
}

} // namespace ygor_file_index_detail


class sqlite_file_index::implementation {
    public:
        struct registration {
            file_index_measure measure;
            file_index_measure_extractor extractor;
            file_index_measure_applicability applicability;
        };

        explicit implementation(const std::string &database_filename);
        ~implementation();

        void initialise_schema();
        int64_t ensure_measure(const file_index_measure &measure) const;
        int64_t upsert_file(const std::string &path, int64_t size, int64_t mtime);
        file_index_measure_status state_for(int64_t file_id,
                                            int64_t measure_id,
                                            int64_t current_size,
                                            int64_t current_mtime) const;
        void store_failed_state(int64_t file_id,
                                int64_t measure_id,
                                int64_t size,
                                int64_t mtime,
                                const std::string &error);
        void replace_facts(int64_t file_id,
                           int64_t measure_id,
                           int64_t size,
                           int64_t mtime,
                           const file_index_measure &measure,
                           const std::vector<file_index_fact> &facts);

        sqlite3 *db = nullptr;
        std::map<std::string, registration> registrations;
};

#endif
