#include "llmodel_provider.h"

#include "llmodel_ollama.h"
#include "llmodel_openai.h"

#include "mysettings.h"

#include <fmt/format.h>
#include <gpt4all-backend/formatters.h> // IWYU pragma: keep

#include <QModelIndex> // IWYU pragma: keep
#include <QVariant>

namespace fs = std::filesystem;


namespace gpt4all::ui {


GenerationParams::~GenerationParams() noexcept = default;

void GenerationParams::parse(QMap<GenerationParam, QVariant> values)
{
    parseInner(values);
    if (!values.isEmpty()) {
        auto gparamsMeta = QMetaEnum::fromType<GenerationParam>();
        throw std::invalid_argument(fmt::format(
            " unsupported param: {}", gparamsMeta.valueToKey(int(values.keys().constFirst()))
        ));
    }
}

QVariant GenerationParams::tryParseValue(QMap<GenerationParam, QVariant> &values, GenerationParam key,
                                         const QMetaType &type)
{
    auto value = values.take(key);
    if (value.isValid() && !value.canConvert(type)) {
        auto gparamsMeta = QMetaEnum::fromType<GenerationParam>();
        throw std::invalid_argument(fmt::format(
            "expected {} of type {}, got {}", gparamsMeta.valueToKey(int(key)), type.name(), value.typeName()
        ));
    }
    return value;
}

ModelProvider::~ModelProvider() noexcept = default;

ModelProviderMutable::~ModelProviderMutable() noexcept
{
    if (auto res = m_store->release(m_id); !res)
        res.error().raise(); // should not happen - will terminate program
}

ProviderRegistry::ProviderRegistry(PathSet paths)
    : m_customStore (std::move(paths.custom ))
    , m_builtinStore(std::move(paths.builtin))
{
    auto *mysettings = MySettings::globalInstance();
    connect(mysettings, &MySettings::modelPathChanged, this, &ProviderRegistry::onModelPathChanged);
    load();
}

namespace {
    class ProviderRegistryInternal : public ProviderRegistry {};
    Q_GLOBAL_STATIC(ProviderRegistryInternal, providerRegistry)
}

ProviderRegistry *ProviderRegistry::globalInstance()
{ return providerRegistry(); }

void ProviderRegistry::load()
{
    size_t i = 0;
    for (auto &p : s_builtinProviders) { // (not all builtin providers are stored)
        auto provider = std::make_shared<OpenaiProviderBuiltin>(
            &m_builtinStore, p.id, p.name, p.icon, p.base_url,
            QStringList(p.model_whitelist.begin(), p.model_whitelist.end())
        );
        auto [_, unique] = m_providers.emplace(p.id, std::move(provider));
        if (!unique)
            throw std::logic_error(fmt::format("duplicate builtin provider id: {}", p.id.toString()));
        m_builtinProviders[i++] = p.id;
    }
    for (auto &p : m_customStore.list()) { // disk is source of truth for custom providers
        if (!p.custom_details) {
            qWarning() << "ignoring builtin provider in custom store:" << p.id;
            continue;
        }
        auto &cust = *p.custom_details;
        std::shared_ptr<ModelProviderCustom> provider;
        switch (p.type()) {
            using enum ProviderType;
        case ollama:
            provider = std::make_shared<OllamaProviderCustom>(
                &m_customStore, p.id, cust.name, cust.base_url
            );
            break;
        case openai:
            provider = std::make_shared<OpenaiProviderCustom>(
                &m_customStore, p.id, cust.name, cust.base_url,
                std::get<size_t(ProviderType::openai)>(p.provider_details).api_key
            );
        }
        auto [_, unique] = m_providers.emplace(p.id, std::move(provider));
        if (!unique)
            qWarning() << "ignoring duplicate custom provider with id:" << p.id;
        m_customProviders.push_back(std::make_unique<QUuid>(p.id));
    }
}

[[nodiscard]]
bool ProviderRegistry::add(std::shared_ptr<ModelProviderCustom> provider)
{
    auto [it, unique] = m_providers.emplace(provider->id(), std::move(provider));
    if (unique) {
        m_customProviders.push_back(std::make_unique<QUuid>(it->first));
        emit customProviderAdded(m_customProviders.size() - 1);
    }
    return unique;
}

auto ProviderRegistry::customProviderAt(size_t i) const -> ModelProviderCustom *
{
    auto it = m_providers.find(*m_customProviders.at(i));
    Q_ASSERT(it != m_providers.end());
    return &dynamic_cast<ModelProviderCustom &>(*it->second);
}

auto ProviderRegistry::builtinProviderAt(size_t i) const -> ModelProviderBuiltin *
{
    auto it = m_providers.find(m_builtinProviders.at(i));
    Q_ASSERT(it != m_providers.end());
    return &dynamic_cast<ModelProviderBuiltin &>(*it->second);

}

auto ProviderRegistry::getSubdirs() -> PathSet
{
    auto *mysettings = MySettings::globalInstance();
    auto parent = toFSPath(mysettings->modelPath()) / "providers";
    return { .builtin = parent, .custom  = parent / "custom" };
}

void ProviderRegistry::onModelPathChanged()
{
    auto paths = getSubdirs();
    if (paths.builtin != m_builtinStore.path()) {
        emit aboutToBeCleared();
        // delete providers to release store locks
        m_customProviders.clear();
        m_providers.clear();
        if (auto res = m_builtinStore.setPath(paths.builtin); !res)
            res.error().raise(); // should not happen
        if (auto res = m_customStore.setPath(paths.custom); !res)
            res.error().raise(); // should not happen
        load();
    }
}

auto BuiltinProviderList::roleNames() const -> QHash<int, QByteArray>
{ return { { Qt::DisplayRole, "data"_ba } }; }

QVariant BuiltinProviderList::data(const QModelIndex &index, int role) const
{
    auto *registry = ProviderRegistry::globalInstance();
    if (index.isValid() && index.row() < rowCount() && role == Qt::DisplayRole)
        return QVariant::fromValue(registry->builtinProviderAt(index.row())->asQObject());
    return {};
}

CustomProviderList::CustomProviderList()
    : m_size(ProviderRegistry::globalInstance()->customProviderCount())
{
    auto *registry = ProviderRegistry::globalInstance();
    connect(registry, &ProviderRegistry::customProviderAdded, this, &CustomProviderList::onCustomProviderAdded);
    connect(registry, &ProviderRegistry::aboutToBeCleared, this, &CustomProviderList::onAboutToBeCleared,
            Qt::DirectConnection);
}

QVariant CustomProviderList::data(const QModelIndex &index, int role) const
{
    auto *registry = ProviderRegistry::globalInstance();
    if (index.isValid() && index.row() < rowCount() && role == Qt::DisplayRole)
        return QVariant::fromValue(registry->customProviderAt(index.row())->asQObject());
    return {};
}

void CustomProviderList::onCustomProviderAdded(size_t index)
{
    beginInsertRows({}, m_size, m_size);
    m_size++;
    endInsertRows();
}

void CustomProviderList::onAboutToBeCleared()
{
    beginResetModel();
    m_size = 0;
    endResetModel();
}

bool ProviderListSort::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    auto *leftData  = sourceModel()->data(left ).value<ModelProvider *>();
    auto *rightData = sourceModel()->data(right).value<ModelProvider *>();
    if (leftData && rightData)
        return QString::localeAwareCompare(leftData->name(), rightData->name()) < 0;
    return true;
}


} // namespace gpt4all::ui
