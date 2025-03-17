#include "json-helpers.h" // IWYU pragma: keep

#include <boost/json.hpp> // IWYU pragma: keep
#include <gpt4all-backend/json-helpers.h> // IWYU pragma: keep

#include <QSaveFile>
#include <QtAssert>

#include <system_error>


namespace gpt4all::ui {


template <typename T>
DataStore<T>::DataStore(std::filesystem::path path)
    : DataStoreBase(std::move(path))
{
    if (auto res = reload(); !res)
        res.error().raise(); // should be impossible
}

template <typename T>
auto DataStore<T>::list() -> tl::generator<const T &>
{
    for (auto &[_, value] : m_entries)
        co_yield value;
}

template <typename T>
auto DataStore<T>::createImpl(T data, const QString &name) -> DataStoreResult<const T *>
{
    // acquire path
    auto file = openNew(name);
    if (!file)
        return std::unexpected(file.error());

    // serialize
    if (auto res = write(boost::json::value_from(data), **file); !res)
        return std::unexpected(res.error());

    // insert
    auto [it, unique] = m_entries.emplace(data.id, std::move(data));
    Q_ASSERT(unique);

    // acquire data ownership
    if (auto res = acquire(data.id); !res)
        return std::unexpected(res.error());

    return &it->second;
}

template <typename T>
auto DataStore<T>::setData(T data, std::optional<QString> createName) -> DataStoreResult<>
{
    const QString *openName;
    auto name_it = m_names.find(data.id);
    if (name_it != m_names.end()) {
        openName = &name_it->second;
    } else if (createName) {
        openName = &*createName;
    } else
        return std::unexpected(QStringLiteral("id not found: %1").arg(data.id.toString()));

    // acquire path
    auto file = openExisting(*openName, !!createName);
    if (!file)
        return std::unexpected(file.error());

    // serialize
    if (auto res = write(boost::json::value_from(data), **file); !res)
        return std::unexpected(res.error());
    if (!(*file)->commit())
        return std::unexpected(file->get());

    // update
    m_entries[data.id] = std::move(data);

    // rename if necessary
    if (name_it == m_names.end()) {
        m_names.emplace(data.id, std::move(*createName));
    } else if (*createName != name_it->second) {
        std::error_code ec;
        auto newPath = getFilePath(*createName);
        std::filesystem::rename(getFilePath(name_it->second), newPath, ec);
        if (ec)
            return std::unexpected(ec);
        m_names.at(data.id) = std::move(*createName);
    }

    return {};
}

template <typename T>
auto DataStore<T>::remove(const QUuid &id) -> DataStoreResult<>
{
    // acquire UUID
    auto it = m_entries.find(id);
    if (it == m_entries.end())
        return std::unexpected(QStringLiteral("id not found: %1").arg(id.toString()));

    auto &[_, data] = *it;

    // remove the path
    auto path = getFilePath(data.name);
    QFile file(path);
    if (!file.remove())
        throw std::unexpected(&file);

    // update cache
    m_entries.erase(it);
    return {};
}

template <typename T>
auto DataStore<T>::acquire(QUuid id) -> DataStoreResult<std::optional<const T *>>
{
    auto [it, unique] = m_acquired.insert(std::move(id));
    if (!unique)
        return std::unexpected(QStringLiteral("id already acquired: %1").arg(id.toString()));
    return find(*it);
}

template <typename T>
auto DataStore<T>::release(const QUuid &id) -> DataStoreResult<>
{
    if (!m_acquired.erase(id))
        return std::unexpected(QStringLiteral("id not acquired: %1").arg(id.toString()));
    return {};
}

template <typename T>
auto DataStore<T>::clear() -> DataStoreResult<>
{
    if (!m_acquired.empty())
        return std::unexpected(QStringLiteral("cannot clear data store with living references"));
    m_entries.clear();
    return {};
}

template <typename T>
auto DataStore<T>::cacheInsert(const boost::json::value &jv) -> CacheInsertResult
{
    auto data = boost::json::value_to<T>(jv);
    auto id = data.id;
    auto [_, ok] = m_entries.emplace(id, std::move(data));
    return { ok, std::move(id) };
}


} // namespace gpt4all::ui
