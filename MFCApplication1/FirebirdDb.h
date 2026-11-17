// FirebirdDb.h: небольшой C++-слой для работы с RedDatabase / Firebird
// через официальный клиентский API (fbclient.dll / fbclient_ms.lib).

#pragma once

#include <string>
#include <vector>
#include <variant>

#include <ibase.h>

class FirebirdDb
{
public:
    // Параметр SQL-запроса: NULL, INTEGER, DOUBLE или строка (UTF-8).
    using Param = std::variant<std::nullptr_t, int, double, std::string>;

    FirebirdDb() = default;
    ~FirebirdDb();

    FirebirdDb(const FirebirdDb&) = delete;
    FirebirdDb& operator=(const FirebirdDb&) = delete;

    bool Connect(const std::string& dbPath,
                 const std::string& user,
                 const std::string& password,
                 std::string& error);

    bool IsConnected() const { return db_ != 0; }

    // Выполняет INSERT / UPDATE / DELETE с параметрами.
    bool Execute(const std::string& sql,
                 const std::vector<Param>& params,
                 std::string& error);

    // Возвращает первое поле FIRST-строки как INTEGER.
    bool QueryInt(const std::string& sql,
                  const std::vector<Param>& params,
                  int& value,
                  std::string& error);

    // Возвращает первое поле FIRST-строки как строку (UTF-8).
    bool QueryString(const std::string& sql,
                     const std::vector<Param>& params,
                     std::string& value,
                     std::string& error);

private:
    isc_db_handle db_{ 0 };
    isc_tr_handle tr_{ 0 };
    ISC_STATUS status_[ISC_STATUS_LENGTH]{ 0 };

    std::string StatusMessage() const;
    bool Begin(std::string& error);
    bool Commit(std::string& error);
    void Rollback();
};
