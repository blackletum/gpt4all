#include "llmodel_provider.h"

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

ModelProviderCustom::~ModelProviderCustom() noexcept
{
    if (auto res = m_store->release(m_id); !res)
        res.error().raise(); // should not happen - will terminate program
}

auto ModelProviderCustom::load() -> const ModelProviderData::Details &
{
    auto data = m_store->acquire(m_id);
    if (!data)
        data.error().raise();
    m_name    = (*data)->name;
    m_baseUrl = (*data)->base_url;
    return (*data)->details;
}

ProviderRegistry::ProviderRegistry(fs::path path)
    : m_store(std::move(path))
{
    auto *mysettings = MySettings::globalInstance();
    connect(mysettings, &MySettings::modelPathChanged, this, &ProviderRegistry::onModelPathChanged);
}

Q_INVOKABLE void ProviderRegistry::registerBuiltinProvider(ModelProviderBuiltin *provider)
{
    auto [_, unique] = m_providers.emplace(provider->id(), provider->asQObject());
    if (!unique)
        qWarning() << "ignoring duplicate provider:" << provider->id();
}

[[nodiscard]]
bool ProviderRegistry::registerCustomProvider(std::unique_ptr<ModelProviderCustom> provider)
{
    auto [_, unique] = m_providers.emplace(provider->id(), provider->asQObject());
    if (unique) {
        m_customProviders.push_back(std::move(provider));
        emit customProviderAdded(m_customProviders.size() - 1);
    }
    return unique;
}

fs::path ProviderRegistry::getSubdir()
{
    auto *mysettings = MySettings::globalInstance();
    return toFSPath(mysettings->modelPath()) / "providers";
}

void ProviderRegistry::onModelPathChanged()
{
    auto path = getSubdir();
    if (path != m_store.path()) {
        emit aboutToBeCleared();
        m_customProviders.clear(); // delete custom providers to release store locks
        if (auto res = m_store.setPath(path); !res)
            res.error().raise(); // should not happen
    }
}

CustomProviderList::CustomProviderList(QPointer<ProviderRegistry> registry)
    : m_registry(std::move(registry))
    , m_size(m_registry->customProviderCount())
{
    connect(m_registry, &ProviderRegistry::customProviderAdded, this, &CustomProviderList::onCustomProviderAdded);
    connect(m_registry, &ProviderRegistry::aboutToBeCleared, this, &CustomProviderList::onAboutToBeCleared,
            Qt::DirectConnection);
}

QVariant CustomProviderList::data(const QModelIndex &index, int role) const
{
    if (index.isValid() && index.row() < rowCount() && role == Qt::DisplayRole)
        return QVariant::fromValue(m_registry->customProviderAt(index.row()));
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

bool CustomProviderListSort::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    auto *leftData  = sourceModel()->data(left ).value<ModelProviderCustom *>();
    auto *rightData = sourceModel()->data(right).value<ModelProviderCustom *>();
    if (leftData && rightData)
        return QString::localeAwareCompare(leftData->name(), rightData->name()) < 0;
    return true;
}


} // namespace gpt4all::ui
