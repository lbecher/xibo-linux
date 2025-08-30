#include "stat/storage/DatabaseProvider.hpp"
#include "stat/records/RecordType.hpp"

#include "common/logger/Logging.hpp"
#include "config/AppConfig.hpp"

#include <charconv>
#include <cstdint>

using namespace Stats;
using namespace std::string_literals;

struct DatabaseProvider::PrivateData
{
    PrivateData(const std::string& path) : db(path, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE) 
    {
        // Create table if it doesn't exist
        db.exec(R"(
            CREATE TABLE IF NOT EXISTS stats (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                type TEXT NOT NULL,
                started INTEGER NOT NULL,
                finished INTEGER NOT NULL,
                scheduleId INTEGER NOT NULL,
                layoutId INTEGER NOT NULL,
                mediaId INTEGER NOT NULL,
                duration INTEGER NOT NULL,
                count INTEGER NOT NULL
            )
        )");
    }

    SQLite::Database db;
};

DatabaseProvider::DatabaseProvider() : data_(std::make_unique<PrivateData>(AppConfig::statsCache().string()))
{
}

DatabaseProvider::~DatabaseProvider() {}

void DatabaseProvider::save(const RecordDto& record)
{
    try
    {
        SQLite::Statement query(data_->db, 
            "INSERT INTO stats (type, started, finished, scheduleId, layoutId, mediaId, duration, count) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?)");
        
        query.bind(1, recordTypeToString(record.type));
        query.bind(2, static_cast<int64_t>(record.started.timestamp()));
        query.bind(3, static_cast<int64_t>(record.finished.timestamp()));
        query.bind(4, record.scheduleId);
        query.bind(5, record.layoutId);
        
        // Handle optional mediaId
        if (record.mediaId.has_value()) {
            query.bind(6, record.mediaId.value());
        } else {
            query.bind(6); // bind NULL
        }
        
        query.bind(7, record.duration);
        query.bind(8, record.count);
        
        query.exec();
    }
    catch (const std::exception& e)
    {
        throw Error{"database insert failed: "s + e.what()};
    }
}

void DatabaseProvider::save(PlayingRecordDtoCollection&& records)
{
    try
    {
        SQLite::Transaction transaction(data_->db);
        
        for (auto&& record : records)
        {
            save(record);
        }
        
        transaction.commit();
    }
    catch (const std::exception& e)
    {
        throw Error{"database transaction failed: "s + e.what()};
    }
}

PlayingRecordDtoCollection DatabaseProvider::retrieve(size_t count) const
{
    PlayingRecordDtoCollection records;
    
    try
    {
        SQLite::Statement query(data_->db, "SELECT id, type, started, finished, scheduleId, layoutId, mediaId, duration, count FROM stats LIMIT ?");
        query.bind(1, static_cast<int>(count));
        
        while (query.executeStep())
        {
            RecordDto record;
            record.id = query.getColumn(0).getInt();
            
            std::string typeStr = query.getColumn(1).getString();
            if (auto type = recordTypeFromSting(typeStr))
            {
                record.type = *type;
            }
            else
            {
                continue; // Skip invalid record
            }
            
            record.started = DateTime::utcFromTimestamp(query.getColumn(2).getInt64());
            record.finished = DateTime::utcFromTimestamp(query.getColumn(3).getInt64());
            record.scheduleId = query.getColumn(4).getInt();
            record.layoutId = query.getColumn(5).getInt();
            
            // Handle optional mediaId
            if (!query.getColumn(6).isNull()) {
                record.mediaId = query.getColumn(6).getInt();
            } else {
                record.mediaId = std::nullopt;
            }
            
            record.duration = query.getColumn(7).getInt();
            record.count = query.getColumn(8).getInt();
            
            records.push_back(record);
        }
    }
    catch (const std::exception& e)
    {
        throw Error{"database retrieve failed: "s + e.what()};
    }
    
    return records;
}

void DatabaseProvider::removeAll()
{
    try
    {
        data_->db.exec("DELETE FROM stats");
    }
    catch (const std::exception& e)
    {
        throw Error{"database removeAll failed: "s + e.what()};
    }
}

void DatabaseProvider::remove(size_t count)
{
    try
    {
        SQLite::Statement query(data_->db, "DELETE FROM stats WHERE id IN (SELECT id FROM stats ORDER BY id LIMIT ?)");
        query.bind(1, static_cast<int>(count));
        query.exec();
    }
    catch (const std::exception& e)
    {
        throw Error{"database remove failed: "s + e.what()};
    }
}

size_t DatabaseProvider::recordsCount() const
{
    try
    {
        SQLite::Statement query(data_->db, "SELECT COUNT(*) FROM stats");
        if (query.executeStep())
        {
            return query.getColumn(0).getInt();
        }
        return 0;
    }
    catch (const std::exception& e)
    {
        throw Error{"database recordsCount failed: "s + e.what()};
    }
}
