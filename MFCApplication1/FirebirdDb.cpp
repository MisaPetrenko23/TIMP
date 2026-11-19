// FirebirdDb.cpp: реализация обёртки над Firebird/RedDatabase API.

#include "pch.h"
#include "FirebirdDb.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>

namespace
{
    // Выделяет память под XSQLDA с местом под n переменных.
    std::unique_ptr<XSQLDA, decltype(&std::free)> MakeSqlda(int n)
    {
        if (n < 1) n = 1;
        std::unique_ptr<XSQLDA, decltype(&std::free)> sq(
            static_cast<XSQLDA*>(std::malloc(XSQLDA_LENGTH(n))), &std::free);
        std::memset(sq.get(), 0, XSQLDA_LENGTH(n));
        sq->version = SQLDA_VERSION1;
        sq->sqln = static_cast<short>(n);
        sq->sqld = 0;
        return sq;
    }

    constexpr ISC_STATUS FETCH_END = 100;
}

FirebirdDb::~FirebirdDb()
{
    Rollback();
    if (db_ != 0)
    {
        isc_detach_database(status_, &db_);
        db_ = 0;
    }
}

std::string FirebirdDb::StatusMessage() const
{
    std::string result;
    const ISC_STATUS* p = status_;
    ISC_SCHAR buf[512]{};

    while (fb_interpret(buf, sizeof(buf), &p) != 0)
    {
        if (!result.empty()) result += " | ";
        result += reinterpret_cast<const char*>(buf);
    }

    if (result.empty()) result = "неизвестная ошибка Firebird/RedDatabase";
    return result;
}

bool FirebirdDb::Begin(std::string& error)
{
    if (tr_ != 0) return true;

    if (isc_start_transaction(status_, &tr_, 1, &db_, 0, nullptr) != 0)
    {
        error = "Не удалось начать транзакцию: " + StatusMessage();
        return false;
    }
    return true;
}

bool FirebirdDb::Commit(std::string& error)
{
    if (tr_ == 0) return true;

    if (isc_commit_transaction(status_, &tr_) != 0)
    {
        error = "Не удалось зафиксировать транзакцию: " + StatusMessage();
        Rollback();
        return false;
    }
    return true;
}

void FirebirdDb::Rollback()
{
    if (tr_ != 0)
    {
        isc_rollback_transaction(status_, &tr_);
        tr_ = 0;
    }
}

bool FirebirdDb::Connect(const std::string& dbPath,
                         const std::string& user,
                         const std::string& password,
                         std::string& error)
{
    if (db_ != 0)
    {
        error = "БД уже подключена";
        return true;
    }

    char dpb[256]{};
    int len = 0;

    dpb[len++] = isc_dpb_version1;

    auto addParam = [&](char code, const std::string& value)
    {
        if (len + 2 + static_cast<int>(value.size()) > static_cast<int>(sizeof(dpb)))
            return false;
        dpb[len++] = code;
        dpb[len++] = static_cast<char>(value.size());
        std::memcpy(dpb + len, value.data(), value.size());
        len += static_cast<int>(value.size());
        return true;
    };

    if (!addParam(isc_dpb_user_name, user) ||
        !addParam(isc_dpb_password, password) ||
        !addParam(isc_dpb_lc_ctype, "UTF8"))
    {
        error = "Слишком длинные параметры подключения";
        return false;
    }

    if (isc_attach_database(status_,
                            static_cast<short>(dbPath.size()),
                            const_cast<char*>(dbPath.c_str()),
                            &db_,
                            static_cast<short>(len),
                            dpb) != 0)
    {
        db_ = 0;
        error = "Не удалось открыть БД '" + dbPath + "': " + StatusMessage();
        return false;
    }

    return true;
}

bool FirebirdDb::Execute(const std::string& sql,
                         const std::vector<Param>& params,
                         std::string& error)
{
    if (db_ == 0)
    {
        error = "БД не подключена";
        return false;
    }
    if (!Begin(error)) return false;

    isc_stmt_handle stmt = 0;
    if (isc_dsql_allocate_statement(status_, &db_, &stmt) != 0)
    {
        error = "Ошибка выделения запроса: " + StatusMessage();
        Rollback();
        return false;
    }

    auto out = MakeSqlda(1);

    if (isc_dsql_prepare(status_, &tr_, &stmt,
                         static_cast<short>(sql.size()), sql.c_str(),
                         3, out.get()) != 0)
    {
        error = "Ошибка подготовки SQL: " + StatusMessage();
        isc_dsql_free_statement(status_, &stmt, DSQL_drop);
        Rollback();
        return false;
    }

    auto in = MakeSqlda(static_cast<int>(params.size()) + 1);

    if (!params.empty())
    {
        if (isc_dsql_describe_bind(status_, &stmt, 1, in.get()) != 0)
        {
            error = "Ошибка описания параметров: " + StatusMessage();
            isc_dsql_free_statement(status_, &stmt, DSQL_drop);
            Rollback();
            return false;
        }

        if (in->sqld != static_cast<short>(params.size()))
        {
            error = "Несовпадение числа параметров SQL (" +
                    std::to_string(in->sqld) + " и " +
                    std::to_string(params.size()) + ")";
            isc_dsql_free_statement(status_, &stmt, DSQL_drop);
            Rollback();
            return false;
        }

        std::vector<short> indicators(params.size(), 0);
        std::vector<std::vector<char>> strBuffers;
        std::vector<int> intValues;
        std::vector<double> doubleValues;
        strBuffers.reserve(params.size());
        intValues.reserve(params.size());
        doubleValues.reserve(params.size());

        for (size_t i = 0; i < params.size(); ++i)
        {
            XSQLVAR& v = in->sqlvar[i];
            v.sqlscale = 0;
            v.sqlsubtype = 0;
            v.sqlind = &indicators[i];

            if (std::holds_alternative<std::nullptr_t>(params[i]))
            {
                indicators[i] = -1;
                v.sqltype = SQL_VARYING;
                v.sqldata = nullptr;
                v.sqllen = 0;
            }
            else if (const auto* s = std::get_if<std::string>(&params[i]))
            {
                std::vector<char> buf(2 + s->size(), 0);
                short strLen = static_cast<short>(std::min<size_t>(s->size(), 32760));
                std::memcpy(buf.data(), &strLen, sizeof(strLen));
                if (s->size()) std::memcpy(buf.data() + 2, s->data(), s->size());
                strBuffers.push_back(std::move(buf));

                v.sqltype = SQL_VARYING;
                v.sqldata = strBuffers.back().data();
                v.sqllen = static_cast<short>(2 + s->size());
            }
            else if (const auto* n = std::get_if<int>(&params[i]))
            {
                intValues.push_back(*n);
                v.sqltype = SQL_LONG;
                v.sqldata = reinterpret_cast<char*>(&intValues.back());
                v.sqllen = sizeof(int);
            }
            else if (const auto* d = std::get_if<double>(&params[i]))
            {
                doubleValues.push_back(*d);
                v.sqltype = SQL_DOUBLE;
                v.sqldata = reinterpret_cast<char*>(&doubleValues.back());
                v.sqllen = sizeof(double);
            }
        }

        if (isc_dsql_execute(status_, &tr_, &stmt, 1, in.get()) != 0)
        {
            error = "Ошибка выполнения SQL: " + StatusMessage();
            isc_dsql_free_statement(status_, &stmt, DSQL_drop);
            Rollback();
            return false;
        }
    }
    else
    {
        if (isc_dsql_execute(status_, &tr_, &stmt, 1, nullptr) != 0)
        {
            error = "Ошибка выполнения SQL: " + StatusMessage();
            isc_dsql_free_statement(status_, &stmt, DSQL_drop);
            Rollback();
            return false;
        }
    }

    isc_dsql_free_statement(status_, &stmt, DSQL_drop);
    return Commit(error);
}

bool FirebirdDb::QueryInt(const std::string& sql,
                          const std::vector<Param>& params,
                          int& value,
                          std::string& error)
{
    if (db_ == 0)
    {
        error = "БД не подключена";
        return false;
    }
    if (!Begin(error)) return false;

    isc_stmt_handle stmt = 0;
    if (isc_dsql_allocate_statement(status_, &db_, &stmt) != 0)
    {
        error = "Ошибка выделения запроса: " + StatusMessage();
        Rollback();
        return false;
    }

    auto out = MakeSqlda(4);

    if (isc_dsql_prepare(status_, &tr_, &stmt,
                         static_cast<short>(sql.size()), sql.c_str(),
                         3, out.get()) != 0)
    {
        error = "Ошибка подготовки запроса: " + StatusMessage();
        isc_dsql_free_statement(status_, &stmt, DSQL_drop);
        Rollback();
        return false;
    }

    // Параметры для SELECT в этой программе не используются, но поддержка оставлена.
    if (!params.empty())
    {
        error = "Параметры SELECT не поддерживаются";
        isc_dsql_free_statement(status_, &stmt, DSQL_drop);
        Rollback();
        return false;
    }

    if (isc_dsql_describe(status_, &stmt, 1, out.get()) != 0)
    {
        error = "Ошибка описания результата: " + StatusMessage();
        isc_dsql_free_statement(status_, &stmt, DSQL_drop);
        Rollback();
        return false;
    }

    if (out->sqld < 1)
    {
        error = "Запрос не вернул ни одного столбца";
        isc_dsql_free_statement(status_, &stmt, DSQL_drop);
        Rollback();
        return false;
    }

    XSQLVAR& col = out->sqlvar[0];
    short indicator = 0;
    int intValue = 0;
    ISC_INT64 int64Value = 0;
    // Младший бит sqltype — признак NULL; сам тип выделяем маской.
    const short baseType = col.sqltype & ~1;

    col.sqlind = &indicator;
    if (baseType == SQL_LONG)
    {
        col.sqldata = reinterpret_cast<char*>(&intValue);
        col.sqllen = sizeof(int);
    }
    else if (baseType == SQL_INT64)
    {
        col.sqldata = reinterpret_cast<char*>(&int64Value);
        col.sqllen = sizeof(ISC_INT64);
    }
    else
    {
        error = "Запрос вернул не целочисленное значение";
        isc_dsql_free_statement(status_, &stmt, DSQL_drop);
        Rollback();
        return false;
    }

    if (isc_dsql_execute(status_, &tr_, &stmt, 1, nullptr) != 0)
    {
        error = "Ошибка выполнения запроса: " + StatusMessage();
        isc_dsql_free_statement(status_, &stmt, DSQL_drop);
        Rollback();
        return false;
    }

    ISC_STATUS rc = isc_dsql_fetch(status_, &stmt, 1, out.get());
    if (rc != 0)
    {
        isc_dsql_free_statement(status_, &stmt, DSQL_drop);
        Rollback();
        error = (rc == FETCH_END) ? "Запрос не вернул ни одной строки" : StatusMessage();
        return false;
    }

    if (indicator == -1)
    {
        isc_dsql_free_statement(status_, &stmt, DSQL_drop);
        Rollback();
        error = "Запрос вернул NULL";
        return false;
    }

    value = (baseType == SQL_LONG)
        ? intValue
        : static_cast<int>(int64Value);

    isc_dsql_free_statement(status_, &stmt, DSQL_drop);
    return Commit(error);
}

bool FirebirdDb::QueryString(const std::string& sql,
                             const std::vector<Param>& params,
                             std::string& value,
                             std::string& error)
{
    if (db_ == 0)
    {
        error = "БД не подключена";
        return false;
    }
    if (!Begin(error)) return false;

    isc_stmt_handle stmt = 0;
    if (isc_dsql_allocate_statement(status_, &db_, &stmt) != 0)
    {
        error = "Ошибка выделения запроса: " + StatusMessage();
        Rollback();
        return false;
    }

    auto out = MakeSqlda(4);

    if (isc_dsql_prepare(status_, &tr_, &stmt,
                         static_cast<short>(sql.size()), sql.c_str(),
                         3, out.get()) != 0)
    {
        error = "Ошибка подготовки запроса: " + StatusMessage();
        isc_dsql_free_statement(status_, &stmt, DSQL_drop);
        Rollback();
        return false;
    }

    if (!params.empty())
    {
        error = "Параметры SELECT не поддерживаются";
        isc_dsql_free_statement(status_, &stmt, DSQL_drop);
        Rollback();
        return false;
    }

    if (isc_dsql_describe(status_, &stmt, 1, out.get()) != 0)
    {
        error = "Ошибка описания результата: " + StatusMessage();
        isc_dsql_free_statement(status_, &stmt, DSQL_drop);
        Rollback();
        return false;
    }

    if (out->sqld < 1)
    {
        error = "Запрос не вернул ни одного столбца";
        isc_dsql_free_statement(status_, &stmt, DSQL_drop);
        Rollback();
        return false;
    }

    XSQLVAR& col = out->sqlvar[0];
    short indicator = 0;
    std::vector<char> buf(static_cast<size_t>(col.sqllen) + 2, 0);
    const short baseType = col.sqltype & ~1;

    col.sqlind = &indicator;
    col.sqldata = buf.data();

    if (isc_dsql_execute(status_, &tr_, &stmt, 1, nullptr) != 0)
    {
        error = "Ошибка выполнения запроса: " + StatusMessage();
        isc_dsql_free_statement(status_, &stmt, DSQL_drop);
        Rollback();
        return false;
    }

    ISC_STATUS rc = isc_dsql_fetch(status_, &stmt, 1, out.get());
    if (rc != 0)
    {
        isc_dsql_free_statement(status_, &stmt, DSQL_drop);
        Rollback();
        error = (rc == FETCH_END) ? "Запрос не вернул ни одной строки" : StatusMessage();
        return false;
    }

    if (indicator == -1)
    {
        isc_dsql_free_statement(status_, &stmt, DSQL_drop);
        Rollback();
        error = "Запрос вернул NULL";
        return false;
    }

    if (baseType == SQL_VARYING || baseType == SQL_TEXT)
    {
        if (baseType == SQL_VARYING && buf.size() >= 2)
        {
            short len = 0;
            std::memcpy(&len, buf.data(), sizeof(len));
            len = static_cast<short>(std::min<size_t>(len, buf.size() - 2));
            value.assign(buf.data() + 2, static_cast<size_t>(len));
        }
        else
        {
            value.assign(buf.data());
            while (!value.empty() && value.back() == ' ') value.pop_back();
        }
    }
    else if (baseType == SQL_LONG)
    {
        int n = 0;
        std::memcpy(&n, buf.data(), sizeof(n));
        value = std::to_string(n);
    }
    else
    {
        isc_dsql_free_statement(status_, &stmt, DSQL_drop);
        Rollback();
        error = "Неподдерживаемый тип строкового результата";
        return false;
    }

    isc_dsql_free_statement(status_, &stmt, DSQL_drop);
    return Commit(error);
}

bool FirebirdDb::QueryRows(const std::string& sql,
                           std::vector<std::vector<std::string>>& rows,
                           std::string& error)
{
    rows.clear();

    if (db_ == 0)
    {
        error = "БД не подключена";
        return false;
    }
    if (!Begin(error)) return false;

    isc_stmt_handle stmt = 0;
    auto fail = [&](const std::string& message) -> bool
    {
        error = message;
        if (stmt != 0) isc_dsql_free_statement(status_, &stmt, DSQL_drop);
        Rollback();
        return false;
    };

    if (isc_dsql_allocate_statement(status_, &db_, &stmt) != 0)
        return fail("Ошибка выделения запроса: " + StatusMessage());

    auto out = MakeSqlda(16);

    if (isc_dsql_prepare(status_, &tr_, &stmt,
                         static_cast<short>(sql.size()), sql.c_str(),
                         3, out.get()) != 0)
        return fail("Ошибка подготовки запроса: " + StatusMessage());

    if (isc_dsql_describe(status_, &stmt, 1, out.get()) != 0)
        return fail("Ошибка описания результата: " + StatusMessage());

    const int cols = out->sqld;
    if (cols < 1) return fail("Запрос не вернул ни одного столбца");

    std::vector<short> indicators(cols, 0);
    std::vector<std::vector<char>> buffers;
    buffers.reserve(cols);

    for (int i = 0; i < cols; ++i)
    {
        XSQLVAR& col = out->sqlvar[i];
        const short baseType = col.sqltype & ~1;

        if (baseType != SQL_VARYING && baseType != SQL_TEXT)
            return fail("История операций: неподдерживаемый тип столбца");

        const int dataLen = std::max<short>(col.sqllen, 1);
        buffers.emplace_back(static_cast<size_t>(dataLen) + 2, 0);
        col.sqlind = &indicators[i];
        col.sqldata = buffers.back().data();
    }

    if (isc_dsql_execute(status_, &tr_, &stmt, 1, nullptr) != 0)
        return fail("Ошибка выполнения запроса: " + StatusMessage());

    while (true)
    {
        ISC_STATUS rc = isc_dsql_fetch(status_, &stmt, 1, out.get());
        if (rc == FETCH_END) break;
        if (rc != 0) return fail(StatusMessage());

        std::vector<std::string> row;
        row.reserve(cols);

        for (int i = 0; i < cols; ++i)
        {
            if (indicators[i] == -1)
            {
                row.emplace_back();
                continue;
            }

            const short baseType = out->sqlvar[i].sqltype & ~1;
            const char* data = buffers[i].data();

            if (baseType == SQL_VARYING)
            {
                short len = 0;
                std::memcpy(&len, data, sizeof(len));
                len = static_cast<short>(std::min<size_t>(len, buffers[i].size() - 2));
                row.emplace_back(data + 2, static_cast<size_t>(len));
            }
            else
            {
                std::string value(data);
                while (!value.empty() &&
                       (value.back() == ' ' || value.back() == '\0'))
                    value.pop_back();
                row.push_back(value);
            }
        }
        rows.push_back(std::move(row));
    }

    if (isc_dsql_free_statement(status_, &stmt, DSQL_drop) != 0)
    {
        stmt = 0;
        return fail("Ошибка освобождения запроса: " + StatusMessage());
    }
    stmt = 0;
    return Commit(error);
}
