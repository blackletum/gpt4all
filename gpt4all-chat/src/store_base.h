#pragma once

#include "utils.h" // IWYU pragma: keep

#include <boost/json.hpp> // IWYU pragma: keep
#include <boost/system.hpp> // IWYU pragma: keep
#include <tl/generator.hpp>

#include <QFile>
#include <QFileDevice>
#include <QString>
#include <QUuid>
#include <QtTypes> // IWYU pragma: keep

#include <expected>
#include <filesystem>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>

class QByteArray;
class QSaveFile;


namespace gpt4all::ui {


class DataStoreError {
public:
    using ErrorCode = std::variant<
        QFileDevice::FileError,
        boost::system::error_code,
        std::monostate
    >;

    DataStoreError(const QFileDevice *file);
    DataStoreError(const boost::system::system_error &e);
    DataStoreError(QString e);

    [[nodiscard]] const ErrorCode &error      () const { return m_error;       }
    [[nodiscard]] const QString   &errorString() const { return m_errorString; }

    [[noreturn]] void raise() const;

private:
    ErrorCode m_error;
    QString   m_errorString;
};

template <typename T = void>
using DataStoreResult = std::expected<T, DataStoreError>;

class DataStoreBase {
protected:
    explicit DataStoreBase(std::filesystem::path path)
        : m_path(std::move(path))
        {}

public:
    auto path() const -> const std::filesystem::path & { return m_path; }
    auto setPath(std::filesystem::path path) -> DataStoreResult<>;

protected:
    auto reload() -> DataStoreResult<>;
    virtual auto clear() -> DataStoreResult<> = 0;
    struct InsertResult { bool unique; QUuid uuid; };
    virtual InsertResult insert(const boost::json::value &jv) = 0;

    // helpers
    auto getFilePath(const QString &name) -> std::filesystem::path;
    auto openNew(const QString &name) -> DataStoreResult<std::unique_ptr<QFile>>;
    auto openExisting(const QString &name) -> DataStoreResult<std::unique_ptr<QSaveFile>>;
    static auto read(QFileDevice &file, boost::json::stream_parser &parser) -> DataStoreResult<boost::json::value>;
    auto write(const boost::json::value &value, QFileDevice &file) -> DataStoreResult<>;

private:
    static constexpr uint JSON_BUFSIZ = 16384; // default QFILE_WRITEBUFFER_SIZE

    static QByteArray normalizeName(const QString &name);

protected:
    std::filesystem::path m_path;

private:
    boost::json::serializer m_serializer;
};

template <typename T>
class DataStore : public DataStoreBase {
public:
    explicit DataStore(std::filesystem::path path);

    auto list() -> tl::generator<const T &>;
    auto setData(T data) -> DataStoreResult<>;
    auto remove(const QUuid &id) -> DataStoreResult<>;

    auto acquire(QUuid        id) -> DataStoreResult<const T *>;
    auto release(const QUuid &id) -> DataStoreResult<>;

    [[nodiscard]]
    auto operator[](const QUuid &id) const -> const T &
    { return m_entries.at(id); }

protected:
    auto createImpl(T data, const QString &name) -> DataStoreResult<const T *>;
    auto clear() -> DataStoreResult<> final;
    InsertResult insert(const boost::json::value &jv) override;

private:
    std::unordered_map<QUuid, T> m_entries;
    std::unordered_set<QUuid>    m_acquired;
};


} // namespace gpt4all::ui

#include "store_base.inl" // IWYU pragma: export
