#pragma once

#include "llmodel_chat.h"
#include "llmodel_description.h"
#include "llmodel_provider.h"

#include <QLatin1StringView> // IWYU pragma: keep
#include <QObject> // IWYU pragma: keep
#include <QString>
#include <QUrl>
#include <QVariant>
#include <QtTypes> // IWYU pragma: keep

#include <memory>
#include <utility>

class QNetworkAccessManager;
template <typename Key, typename T> class QMap;
template <typename T> class QSet;


namespace gpt4all::ui {


class OpenaiChatModel;

struct OpenaiGenerationParamsData {
    uint  n_predict;
    float temperature;
    float top_p;
};

class OpenaiGenerationParams : public GenerationParams, public OpenaiGenerationParamsData {
public:
    explicit OpenaiGenerationParams(QMap<GenerationParam, QVariant> values) { parse(std::move(values)); }
    auto toMap() const -> QMap<QLatin1StringView, QVariant> override;
    bool isNoop() const override { return !n_predict; }

protected:
    void parseInner(QMap<GenerationParam, QVariant> &values) override;
};

class OpenaiProvider : public QObject, public virtual ModelProvider {
    Q_OBJECT

protected:
    explicit OpenaiProvider() = default; // custom
    explicit OpenaiProvider(QString apiKey) // built-in
        : m_apiKey(std::move(apiKey))
        {}
public:
    ~OpenaiProvider() noexcept override = 0;

          QObject *asQObject()       override { return this; }
    const QObject *asQObject() const override { return this; }

    [[nodiscard]] const QString &apiKey() const { return m_apiKey; }

    auto supportedGenerationParams() const -> QSet<GenerationParam> override;
    auto makeGenerationParams(const QMap<GenerationParam, QVariant> &values) const -> OpenaiGenerationParams * override;

protected:
    QString m_apiKey;
};

class OpenaiProviderBuiltin : public OpenaiProvider, public ModelProviderBuiltin {
    Q_GADGET
    Q_PROPERTY(QString apiKey READ apiKey CONSTANT)

public:
    /// Create a new built-in OpenAI provider (transient).
    explicit OpenaiProviderBuiltin(QUuid id, QString name, QUrl baseUrl, QString apiKey);
};

class OpenaiProviderCustom final : public OpenaiProvider, public ModelProviderCustom {
    Q_OBJECT

    Q_PROPERTY(QString apiKey READ apiKey WRITE setApiKey NOTIFY apiKeyChanged)

public:
    /// Load an existing OpenaiProvider from disk.
    explicit OpenaiProviderCustom(std::shared_ptr<ProviderStore> store, QUuid id);

    /// Create a new OpenaiProvider on disk.
    explicit OpenaiProviderCustom(std::shared_ptr<ProviderStore> store, QString name, QUrl baseUrl, QString apiKey);

    void setApiKey(QString value) { setMemberProp<QString>(&OpenaiProviderCustom::m_apiKey, "apiKey", std::move(value)); }

Q_SIGNALS:
    void nameChanged   (const QString &value);
    void baseUrlChanged(const QUrl    &value);
    void apiKeyChanged (const QString &value);

protected:
    auto asData() -> ModelProviderData override
    { return { m_id, ProviderType::openai, m_name, m_baseUrl, OpenaiProviderDetails { m_apiKey } }; }
};

class OpenaiModelDescription : public ModelDescription {
    Q_GADGET
    Q_PROPERTY(QString modelName READ modelName CONSTANT)

public:
    explicit OpenaiModelDescription(std::shared_ptr<const OpenaiProvider> provider, QString modelName);

    // getters
    [[nodiscard]] auto           provider () const -> const OpenaiProvider * override { return m_provider.get(); }
    [[nodiscard]] QVariant       key      () const                           override { return m_modelName;      }
    [[nodiscard]] const QString &modelName() const                                    { return m_modelName;      }

    [[nodiscard]] auto newInstance(QNetworkAccessManager *nam) const -> std::unique_ptr<OpenaiChatModel>;

protected:
    [[nodiscard]] auto newInstanceImpl(QNetworkAccessManager *nam) const -> ChatLLMInstance * override;

private:
    std::shared_ptr<const OpenaiProvider> m_provider;
    QString                               m_modelName;
};

class OpenaiChatModel : public ChatLLMInstance {
public:
    explicit OpenaiChatModel(std::shared_ptr<const OpenaiModelDescription> description, QNetworkAccessManager *nam);

    auto description() const -> const OpenaiModelDescription * override
    { return m_description.get(); }

    auto preload() -> QCoro::Task<void> override;

    auto generate(QStringView prompt, const GenerationParams *params, /*out*/ ChatResponseMetadata &metadata)
        -> QCoro::AsyncGenerator<QString> override;

private:
    std::shared_ptr<const OpenaiModelDescription>  m_description;
    QNetworkAccessManager                         *m_nam;
};


} // namespace gpt4all::ui
