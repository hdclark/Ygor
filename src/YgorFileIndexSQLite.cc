//YgorFileIndexSQLite.cc

#include "YgorFileIndexPrivate.hpp"

#include <stdexcept>


namespace {

constexpr int64_t file_index_schema_version = 1;
constexpr int64_t file_index_application_id = 1497843273; // ASCII "YGFI".

class sqlite_transaction {
    public:
        explicit sqlite_transaction(sqlite3 *db_in) : db(db_in) {
            ygor_file_index_detail::sqlite_exec(db, "BEGIN IMMEDIATE;");
        }

        ~sqlite_transaction(){
            if(!finished){
                sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
            }
        }

        void commit(){
            ygor_file_index_detail::sqlite_exec(db, "COMMIT;");
            finished = true;
        }

    private:
        sqlite3 *db = nullptr;
        bool finished = false;
};

} // namespace


sqlite_file_index::implementation::implementation(const std::string &database_filename){
    const auto rc = sqlite3_open_v2(database_filename.c_str(), &db,
                                    SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX,
                                    nullptr);
    if(rc != SQLITE_OK){
        const auto message = ygor_file_index_detail::sqlite_error(db, "Unable to open file index database");
        if(db != nullptr){
            sqlite3_close(db);
            db = nullptr;
        }
        throw std::runtime_error(message);
    }
    sqlite3_busy_timeout(db, 5000);
    try{
        initialise_schema();
    }catch(...){
        sqlite3_close(db);
        db = nullptr;
        throw;
    }
}

sqlite_file_index::implementation::~implementation(){
    if(db != nullptr){
        sqlite3_close(db);
    }
}

void sqlite_file_index::implementation::initialise_schema(){
    using namespace ygor_file_index_detail;
    sqlite_exec(db, "PRAGMA foreign_keys = ON;");

    sqlite_statement application_statement(db, "PRAGMA application_id;");
    if(!application_statement.step()){
        throw std::runtime_error("Unable to read SQLite application_id");
    }
    const auto application_id = application_statement.column_int64(0);

    sqlite_statement version_statement(db, "PRAGMA user_version;");
    if(!version_statement.step()){
        throw std::runtime_error("Unable to read SQLite user_version");
    }
    const auto version = version_statement.column_int64(0);
    if((application_id != 0) && (application_id != file_index_application_id)){
        throw std::runtime_error("SQLite database is not a Ygor file index");
    }
    if((application_id == 0) && (version != 0)){
        throw std::runtime_error("Refusing to initialize a non-empty-versioned SQLite database as a Ygor file index");
    }
    if((version != 0) && (version != file_index_schema_version)){
        throw std::runtime_error("Unsupported file index schema version " + std::to_string(version));
    }

    sqlite_transaction transaction(db);
    sqlite_exec(db,
        "CREATE TABLE IF NOT EXISTS files ("
        " file_id INTEGER PRIMARY KEY,"
        " path TEXT NOT NULL UNIQUE,"
        " size INTEGER NOT NULL,"
        " mtime INTEGER NOT NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS measures ("
        " measure_id INTEGER PRIMARY KEY,"
        " name TEXT NOT NULL,"
        " revision INTEGER NOT NULL,"
        " value_kind INTEGER NOT NULL,"
        " configuration TEXT NOT NULL,"
        " UNIQUE(name, revision, configuration)"
        ");"
        "CREATE TABLE IF NOT EXISTS facts ("
        " fact_id INTEGER PRIMARY KEY,"
        " file_id INTEGER NOT NULL REFERENCES files(file_id) ON DELETE CASCADE,"
        " measure_id INTEGER NOT NULL REFERENCES measures(measure_id) ON DELETE CASCADE,"
        " ordinal INTEGER NOT NULL,"
        " integer_value INTEGER,"
        " real_value REAL,"
        " text_value TEXT,"
        " range_min REAL,"
        " range_max REAL,"
        " UNIQUE(file_id, measure_id, ordinal)"
        ");"
        "CREATE TABLE IF NOT EXISTS file_measure_state ("
        " file_id INTEGER NOT NULL REFERENCES files(file_id) ON DELETE CASCADE,"
        " measure_id INTEGER NOT NULL REFERENCES measures(measure_id) ON DELETE CASCADE,"
        " indexed_size INTEGER NOT NULL,"
        " indexed_mtime INTEGER NOT NULL,"
        " status INTEGER NOT NULL,"
        " error_text TEXT NOT NULL DEFAULT '',"
        " PRIMARY KEY(file_id, measure_id)"
        ");"
        "CREATE INDEX IF NOT EXISTS facts_integer_idx ON facts(measure_id, integer_value, file_id);"
        "CREATE INDEX IF NOT EXISTS facts_real_idx ON facts(measure_id, real_value, file_id);"
        "CREATE INDEX IF NOT EXISTS facts_text_idx ON facts(measure_id, text_value, file_id);"
        "CREATE INDEX IF NOT EXISTS facts_range_idx ON facts(measure_id, range_min, range_max, file_id);"
        "CREATE INDEX IF NOT EXISTS state_measure_idx ON file_measure_state(measure_id, file_id);"
    );
    sqlite_exec(db, "PRAGMA application_id = 1497843273;");
    sqlite_exec(db, "PRAGMA user_version = 1;");
    transaction.commit();
}

int64_t sqlite_file_index::implementation::ensure_measure(const file_index_measure &measure) const {
    using namespace ygor_file_index_detail;
    validate_measure(measure);
    {
        sqlite_statement insert(db,
            "INSERT OR IGNORE INTO measures(name, revision, value_kind, configuration) VALUES(?, ?, ?, ?);");
        insert.bind(1, measure.name);
        insert.bind(2, measure.revision);
        insert.bind(3, kind_to_integer(measure.kind));
        insert.bind(4, measure.configuration);
        insert.step();
    }

    sqlite_statement select(db,
        "SELECT measure_id, value_kind FROM measures WHERE name = ? AND revision = ? AND configuration = ?;");
    select.bind(1, measure.name);
    select.bind(2, measure.revision);
    select.bind(3, measure.configuration);
    if(!select.step()){
        throw std::runtime_error("Unable to resolve file index measure");
    }
    if(select.column_int64(1) != kind_to_integer(measure.kind)){
        throw std::runtime_error("File index measure was previously registered with a different value kind");
    }
    return select.column_int64(0);
}

int64_t sqlite_file_index::implementation::upsert_file(const std::string &path,
                                                        int64_t size,
                                                        int64_t mtime){
    using namespace ygor_file_index_detail;
    sqlite_transaction transaction(db);
    {
        sqlite_statement insert(db,
            "INSERT OR IGNORE INTO files(path, size, mtime) VALUES(?, ?, ?);");
        insert.bind(1, path);
        insert.bind(2, size);
        insert.bind(3, mtime);
        insert.step();
    }
    {
        sqlite_statement update(db, "UPDATE files SET size = ?, mtime = ? WHERE path = ?;");
        update.bind(1, size);
        update.bind(2, mtime);
        update.bind(3, path);
        update.step();
    }
    sqlite_statement select(db, "SELECT file_id FROM files WHERE path = ?;");
    select.bind(1, path);
    if(!select.step()){
        throw std::runtime_error("Unable to resolve indexed file");
    }
    const auto file_id = select.column_int64(0);
    transaction.commit();
    return file_id;
}

file_index_measure_status sqlite_file_index::implementation::state_for(int64_t file_id,
                                                                        int64_t measure_id,
                                                                        int64_t current_size,
                                                                        int64_t current_mtime) const {
    using namespace ygor_file_index_detail;
    sqlite_statement state(db,
        "SELECT indexed_size, indexed_mtime, status FROM file_measure_state "
        "WHERE file_id = ? AND measure_id = ?;");
    state.bind(1, file_id);
    state.bind(2, measure_id);
    if(!state.step()){
        return file_index_measure_status::missing;
    }
    if((state.column_int64(0) != current_size) || (state.column_int64(1) != current_mtime)){
        return file_index_measure_status::stale;
    }
    switch(state.column_int64(2)){
        case state_current: return file_index_measure_status::current;
        case state_empty:   return file_index_measure_status::known_empty;
        case state_failed:  return file_index_measure_status::failed;
        default: throw std::runtime_error("Invalid file index state value");
    }
}

void sqlite_file_index::implementation::store_failed_state(int64_t file_id,
                                                            int64_t measure_id,
                                                            int64_t size,
                                                            int64_t mtime,
                                                            const std::string &error){
    using namespace ygor_file_index_detail;
    sqlite_transaction transaction(db);
    sqlite_statement statement(db,
        "INSERT OR REPLACE INTO file_measure_state"
        "(file_id, measure_id, indexed_size, indexed_mtime, status, error_text) "
        "VALUES(?, ?, ?, ?, ?, ?);");
    statement.bind(1, file_id);
    statement.bind(2, measure_id);
    statement.bind(3, size);
    statement.bind(4, mtime);
    statement.bind(5, state_failed);
    statement.bind(6, error);
    statement.step();
    transaction.commit();
}

void sqlite_file_index::implementation::replace_facts(int64_t file_id,
                                                       int64_t measure_id,
                                                       int64_t size,
                                                       int64_t mtime,
                                                       const file_index_measure &measure,
                                                       const std::vector<file_index_fact> &facts){
    using namespace ygor_file_index_detail;
    sqlite_transaction transaction(db);
    {
        sqlite_statement erase(db, "DELETE FROM facts WHERE file_id = ? AND measure_id = ?;");
        erase.bind(1, file_id);
        erase.bind(2, measure_id);
        erase.step();
    }

    sqlite_statement insert(db,
        "INSERT INTO facts(file_id, measure_id, ordinal, integer_value, real_value, text_value, range_min, range_max) "
        "VALUES(?, ?, ?, ?, ?, ?, ?, ?);");
    int64_t ordinal = 0;
    for(const auto &fact : facts){
        validate_fact(measure, fact);
        insert.bind(1, file_id);
        insert.bind(2, measure_id);
        insert.bind(3, ordinal++);

        if(const auto *v = std::get_if<int64_t>(&fact.value); v != nullptr){
            insert.bind(4, *v);
        }else if(const auto *v = std::get_if<double>(&fact.value); v != nullptr){
            insert.bind(5, *v);
        }else if(const auto *v = std::get_if<std::string>(&fact.value); v != nullptr){
            insert.bind(6, *v);
        }else{
            const auto range = std::get<file_index_range>(fact.value);
            insert.bind(7, range.min);
            insert.bind(8, range.max);
        }
        insert.step();
        insert.reset();
    }

    sqlite_statement state(db,
        "INSERT OR REPLACE INTO file_measure_state"
        "(file_id, measure_id, indexed_size, indexed_mtime, status, error_text) "
        "VALUES(?, ?, ?, ?, ?, '');");
    state.bind(1, file_id);
    state.bind(2, measure_id);
    state.bind(3, size);
    state.bind(4, mtime);
    state.bind(5, facts.empty() ? state_empty : state_current);
    state.step();
    transaction.commit();
}
