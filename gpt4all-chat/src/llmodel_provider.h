#pragma once

#include "store_provider.h"

#include "utils.h" // IWYU pragma: keep

#include <QAbstractListModel>
#include <QObject>
#include <QPointer>
#include <QQmlEngine> // IWYU pragma: keep
#include <QSortFilterProxyModel>
#include <QString>
#include <QUrl>
#include <QUuid>
#include <QtPreprocessorSupport>

#include <cstddef>
#include <filesystem>
#include <memory>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

class QJSEngine;


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
    Q_GADGET
    Q_PROPERTY(QUuid id READ id CONSTANT)

protected:
    explicit ModelProvider(QUuid id) // load
        : m_id(std::move(id)) {}
    explicit ModelProvider(QUuid id, QString name, QUrl baseUrl) // create built-in
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
    Q_GADGET
    Q_PROPERTY(QString name    READ name    CONSTANT)
    Q_PROPERTY(QUrl    baseUrl READ baseUrl CONSTANT)
};

class ModelProviderCustom : public virtual ModelProvider {
    Q_GADGET
    Q_PROPERTY(QString name    READ name    WRITE setName    NOTIFY nameChanged   )
    Q_PROPERTY(QUrl    baseUrl READ baseUrl WRITE setBaseUrl NOTIFY baseUrlChanged)

protected:
    explicit ModelProviderCustom(std::shared_ptr<ProviderStore> store)
        : m_store(std::move(store)) {}

public:
    ~ModelProviderCustom() noexcept override;

    // setters
    void setName   (QString value) { setMemberProp<QString>(&ModelProviderCustom::m_name,    "name",    std::move(value)); }
    void setBaseUrl(QUrl    value) { setMemberProp<QUrl   >(&ModelProviderCustom::m_baseUrl, "baseUrl", std::move(value)); }

protected:
    virtual auto load() -> const ModelProviderData::Details &;
    virtual auto asData() -> ModelProviderData = 0;

    template <typename T, typename S, typename C>
    void setMemberProp(this S &self, T C::* member, std::string_view name, T value);

    std::shared_ptr<ProviderStore> m_store;
};

class ProviderRegistry : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

protected:
    explicit ProviderRegistry(std::filesystem::path path);

public:
    static ProviderRegistry *create(QQmlEngine *, QJSEngine *) { return new ProviderRegistry(getSubdir()); }
    Q_INVOKABLE   void registerBuiltinProvider(ModelProviderBuiltin *provider);
    [[nodiscard]] bool registerCustomProvider (std::unique_ptr<ModelProviderCustom> provider);

    size_t customProviderCount() const
    { return m_customProviders.size(); }
    auto customProviderAt(size_t i) const -> const ModelProviderCustom *
    { return m_customProviders.at(i).get(); }
    auto operator[](const QUuid &id) -> ModelProviderCustom *
    { return &dynamic_cast<ModelProviderCustom &>(*m_providers.at(id)); }

Q_SIGNALS:
    void customProviderAdded(size_t index);
    void aboutToBeCleared();

private:
    static auto getSubdir() -> std::filesystem::path;

private Q_SLOTS:
    void onModelPathChanged();

private:
    ProviderStore                                     m_store;
    std::unordered_map<QUuid, QPointer<QObject>>      m_providers;
    std::vector<std::unique_ptr<ModelProviderCustom>> m_customProviders;
};

class CustomProviderList : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT

protected:
    explicit CustomProviderList(QPointer<ProviderRegistry> registry);

public:
    int rowCount(const QModelIndex &parent = {}) const override
    { Q_UNUSED(parent) return int(m_size); }
    QVariant data(const QModelIndex &index, int role) const override;

private Q_SLOTS:
    void onCustomProviderAdded(size_t index);
    void onAboutToBeCleared();

private:
    QPointer<ProviderRegistry> m_registry;
    size_t                     m_size;
};

class CustomProviderListSort : public QSortFilterProxyModel {
    Q_OBJECT

protected:
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};


} // namespace gpt4all::ui


#include "llmodel_provider.inl" // IWYU pragma: export
