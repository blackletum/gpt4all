#pragma once

#include "llmodel_chat.h"
#include "llmodel_description.h"
#include "llmodel_provider.h"

#include <QByteArray>
#include <QLatin1StringView> // IWYU pragma: keep
#include <QObject>
#include <QString>
#include <QUrl>
#include <QVariant>
#include <QtTypes> // IWYU pragma: keep

class QNetworkAccessManager;
template <typename Key, typename T> class QMap;
template <typename T> class QSet;


namespace gpt4all::ui {


class OllamaChatModel;

struct OllamaGenerationParamsData {
    uint n_predict;
    // TODO(jared): include ollama-specific generation params
};

class OllamaGenerationParams : public GenerationParams, public OllamaGenerationParamsData {
public:
    explicit OllamaGenerationParams(QMap<GenerationParam, QVariant> values) { parse(std::move(values)); }
    auto toMap() const -> QMap<QLatin1StringView, QVariant> override;
    bool isNoop() const override { return !n_predict; }

protected:
    void parseInner(QMap<GenerationParam, QVariant> &values) override;
};

class OllamaProvider : public QObject, public virtual ModelProvider {
    Q_OBJECT

public:
    ~OllamaProvider() noexcept override = 0;

          QObject *asQObject()       override { return this; }
    const QObject *asQObject() const override { return this; }

    auto supportedGenerationParams() const -> QSet<GenerationParam> override;
    auto makeGenerationParams(const QMap<GenerationParam, QVariant> &values) const -> OllamaGenerationParams * override;
};

class OllamaProviderBuiltin : public ModelProviderBuiltin, public OllamaProvider {
    Q_GADGET

public:
    /// Create a new built-in Ollama provider (transient).
    explicit OllamaProviderBuiltin(QUuid id, QString name, QUrl baseUrl)
        : ModelProvider(std::move(id), std::move(name), std::move(baseUrl)) {}
};

class OllamaProviderCustom final : public OllamaProvider, public ModelProviderCustom {
    Q_OBJECT

public:
    /// Load an existing OllamaProvider from disk.
    explicit OllamaProviderCustom(std::shared_ptr<ProviderStore> store, QUuid id);

    /// Create a new OllamaProvider on disk.
    explicit OllamaProviderCustom(std::shared_ptr<ProviderStore> store, QString name, QUrl baseUrl);

Q_SIGNALS:
    void nameChanged   (const QString &value);
    void baseUrlChanged(const QUrl    &value);

protected:
    auto asData() -> ModelProviderData override
    { return { m_id, ProviderType::ollama, m_name, m_baseUrl, {} }; }
};

class OllamaModelDescription : public ModelDescription {
    Q_GADGET
    Q_PROPERTY(QByteArray modelHash READ modelHash CONSTANT)

public:
    explicit OllamaModelDescription(std::shared_ptr<const OllamaProvider> provider, QByteArray modelHash);

    // getters
    [[nodiscard]] auto              provider () const -> const OllamaProvider * override { return m_provider.get(); }
    [[nodiscard]] QVariant          key      () const                           override { return m_modelHash;      }
    [[nodiscard]] const QByteArray &modelHash() const                                    { return m_modelHash;      }

    [[nodiscard]] auto newInstance(QNetworkAccessManager *nam) const -> std::unique_ptr<OllamaChatModel>;

protected:
    [[nodiscard]] auto newInstanceImpl(QNetworkAccessManager *nam) const -> ChatLLMInstance * override;

private:
    std::shared_ptr<const OllamaProvider> m_provider;
    QByteArray                            m_modelHash;
};

class OllamaChatModel : public ChatLLMInstance {
public:
    explicit OllamaChatModel(std::shared_ptr<const OllamaModelDescription> description, QNetworkAccessManager *nam);

    auto description() const -> const OllamaModelDescription * override
    { return m_description.get(); }

    auto preload() -> QCoro::Task<void> override;

    auto generate(QStringView prompt, const GenerationParams &params, /*out*/ ChatResponseMetadata &metadata)
        -> QCoro::AsyncGenerator<QString> override;

private:
    std::shared_ptr<const OllamaModelDescription> m_description;
    // TODO: implement generate using Ollama backend
    QNetworkAccessManager                         *m_nam;
};


} // namespace gpt4all::ui
