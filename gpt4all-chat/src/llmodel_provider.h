#pragma once

#include "store_provider.h"

#include "utils.h" // IWYU pragma: keep

#include <QAbstractListModel>
#include <QObject>
#include <QQmlEngine> // IWYU pragma: keep
#include <QSortFilterProxyModel>
#include <QString>
#include <QUrl>
#include <QUuid>
#include <QtPreprocessorSupport>

#include <array>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

class QByteArray;
class QJSEngine;
template <typename Key, typename T> class QHash;


namespace gpt4all::ui {


Q_NAMESPACE

enum class GenerationParam  {
    NPredict,
    Temperature,
    TopP,
    TopK,
    MinP,
    RepeatPenalty,
    RepeatLastN,
};
Q_ENUM_NS(GenerationParam)

class GenerationParams {
public:
    virtual ~GenerationParams() noexcept = 0;

    virtual QMap<QLatin1StringView, QVariant> toMap() const = 0;
    virtual bool isNoop() const = 0;

protected:
    void parse(QMap<GenerationParam, QVariant> values);
    virtual void parseInner(QMap<GenerationParam, QVariant> &values) = 0;

    static QVariant tryParseValue(QMap<GenerationParam, QVariant> &values, GenerationParam key, const QMetaType &type);

    template <typename T, typename S, typename C>
    void tryParseValue(this S &self, QMap<GenerationParam, QVariant> &values, GenerationParam key, T C::* dest);
};

class ModelProvider {
protected:
    explicit ModelProvider(QUuid id, QString name, QUrl baseUrl) // create built-in or load
        : m_id(std::move(id)), m_name(std::move(name)), m_baseUrl(std::move(baseUrl)) {}
    explicit ModelProvider(QString name, QUrl baseUrl) // create custom
        : m_name(std::move(name)), m_baseUrl(std::move(baseUrl)) {}

public:
    virtual ~ModelProvider() noexcept = 0;

    virtual       QObject *asQObject() = 0;
    virtual const QObject *asQObject() const = 0;

    // getters
    [[nodiscard]] const QUuid   &id     () const { return m_id;      }
    [[nodiscard]] const QString &name   () const { return m_name;    }
    [[nodiscard]] const QUrl    &baseUrl() const { return m_baseUrl; }

    virtual auto supportedGenerationParams() const -> QSet<GenerationParam> = 0;
    virtual auto makeGenerationParams(const QMap<GenerationParam, QVariant> &values) const -> GenerationParams * = 0;

    friend bool operator==(const ModelProvider &a, const ModelProvider &b)
    { return a.m_id == b.m_id; }

protected:
    QUuid   m_id;
    QString m_name;
    QUrl    m_baseUrl;
};

class ModelProviderBuiltin : public virtual ModelProvider {
protected:
    explicit ModelProviderBuiltin(QUrl icon)
        : m_icon(std::move(icon)) {}

public:
    [[nodiscard]] const QUrl &icon() const { return m_icon; }

protected:
    QUrl m_icon;
};

// Mixin with no public interface providing basic load/save
class ModelProviderMutable : public virtual ModelProvider {
protected:
    explicit ModelProviderMutable(ProviderStore *store)
        : m_store(store) {}

public:
    ~ModelProviderMutable() noexcept override;

protected:
    virtual auto asData() -> ModelProviderData = 0;

    template <typename T, typename S, typename C>
    [[nodiscard]] DataStoreResult<> setMemberProp(this S &self, T C::* member, std::string_view name, T value,
                                                  std::optional<QString> createName = {});

    ProviderStore *m_store;
};

class ModelProviderCustom : public ModelProviderMutable {
protected:
    explicit ModelProviderCustom(ProviderStore *store)
        : ModelProviderMutable(store) {}

public:
    // setters
    [[nodiscard]] DataStoreResult<> setName   (QString value)
    { return setMemberProp<QString>(&ModelProviderCustom::m_name,    "name",    std::move(value)); }
    [[nodiscard]] DataStoreResult<> setBaseUrl(QUrl    value)
    { return setMemberProp<QUrl   >(&ModelProviderCustom::m_baseUrl, "baseUrl", std::move(value)); }
};

class ProviderRegistry : public QObject {
    Q_OBJECT

private:
    struct PathSet { std::filesystem::path builtin, custom; };

    struct BuiltinProviderData {
        QUuid                    id;
        QString                  name;
        QUrl                     icon;
        QUrl                     base_url;
        std::span<const QString> model_whitelist;
    };

protected:
    explicit ProviderRegistry(PathSet paths);
    explicit ProviderRegistry(): ProviderRegistry(getSubdirs()) {}

public:
    static ProviderRegistry *globalInstance();

    [[nodiscard]] bool add(std::shared_ptr<ModelProviderCustom> provider);

    auto operator[](const QUuid &id) -> const ModelProvider * { return m_providers.at(id).get(); }
    // TODO(jared): implement a way to remove custom providers via the model
    [[nodiscard]] size_t customProviderCount ()         const { return m_customProviders.size(); }
    [[nodiscard]] auto   customProviderAt    (size_t i) const -> ModelProviderCustom *;
    [[nodiscard]] size_t builtinProviderCount()         const { return m_builtinProviders.size(); }
    [[nodiscard]] auto   builtinProviderAt   (size_t i) const -> ModelProviderBuiltin *;

Q_SIGNALS:
    void customProviderAdded(size_t index);
    void aboutToBeCleared();

private:
    void load();
    static PathSet getSubdirs();

private Q_SLOTS:
    void onModelPathChanged();

private:
    static constexpr size_t N_BUILTIN = 3;
    static const std::array<BuiltinProviderData, N_BUILTIN> s_builtinProviders;

    ProviderStore                                             m_customStore;
    ProviderStore                                             m_builtinStore;
    std::unordered_map<QUuid, std::shared_ptr<ModelProvider>> m_providers;
    std::vector<std::unique_ptr<QUuid>>                       m_customProviders;
    std::array<QUuid, N_BUILTIN>                              m_builtinProviders;
};

// TODO: api keys are allowed to change for here and also below. That should emit dataChanged.
class BuiltinProviderList : public QAbstractListModel {
    Q_OBJECT
    QML_SINGLETON
    QML_ELEMENT

public:
    explicit BuiltinProviderList()
        : m_size(ProviderRegistry::globalInstance()->builtinProviderCount()) {}

    static BuiltinProviderList *create(QQmlEngine *, QJSEngine *) { return new BuiltinProviderList(); }

    auto roleNames() const -> QHash<int, QByteArray> override;
    int rowCount(const QModelIndex &parent = {}) const override
    { Q_UNUSED(parent) return int(m_size); }
    QVariant data(const QModelIndex &index, int role) const override;

private:
    size_t m_size;
};

class CustomProviderList : public QAbstractListModel {
    Q_OBJECT

public:
    explicit CustomProviderList();

    int rowCount(const QModelIndex &parent = {}) const override
    { Q_UNUSED(parent) return int(m_size); }
    QVariant data(const QModelIndex &index, int role) const override;

private Q_SLOTS:
    void onCustomProviderAdded(size_t index);
    void onAboutToBeCleared();

private:
    size_t m_size;
};

// todo: don't have singletons use singletons directly
// TODO: actually use the provider sort, here, rather than unsorted, for builtins
class ProviderListSort : public QSortFilterProxyModel {
    Q_OBJECT
    QML_SINGLETON
    QML_ELEMENT

private:
    explicit ProviderListSort() { setSourceModel(&m_model); }

public:
    static ProviderListSort *create(QQmlEngine *, QJSEngine *) { return new ProviderListSort(); }

protected:
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
    // TODO: support custom providers as well
    BuiltinProviderList m_model;
};


} // namespace gpt4all::ui


#include "llmodel_provider.inl" // IWYU pragma: export
