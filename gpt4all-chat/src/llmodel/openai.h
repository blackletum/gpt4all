#pragma once

#include "chat.h"
#include "provider.h"

#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QUrl>

class QNetworkAccessManager;


namespace gpt4all::ui {


class OpenaiModelDescription : public QObject {
    Q_OBJECT
    QML_ELEMENT

public:
    explicit OpenaiModelDescription(OpenaiProvider *provider, QString displayName, QString modelName)
        : QObject(provider)
        , m_provider(provider)
        , m_displayName(std::move(displayName))
        , m_modelName(std::move(modelName))
        {}

    // getters
    [[nodiscard]] OpenaiProvider *provider   () const { return m_provider;    }
    [[nodiscard]] const QString  &displayName() const { return m_displayName; }
    [[nodiscard]] const QString  &modelName  () const { return m_modelName;   }

    // setters
    void setDisplayName(QString value);
    void setModelName  (QString value);

Q_SIGNALS:
    void displayNameChanged(const QString &value);
    void modelNameChanged  (const QString &value);

private:
    OpenaiProvider *m_provider;
    QString         m_displayName;
    QString         m_modelName;
};

struct OpenaiConnectionDetails {
    QUrl    baseUrl;
    QString modelName;
    QString apiKey;

    OpenaiConnectionDetails(const OpenaiModelDescription *desc)
        : baseUrl(desc->provider()->baseUrl())
        , apiKey(desc->provider()->apiKey())
        , modelName(desc->modelName())
        {}
};

class OpenaiLLModel : public ChatLLModel {
public:
    explicit OpenaiLLModel(OpenaiConnectionDetails connDetails, QNetworkAccessManager *nam);

    void preload() override { /* not supported -> no-op */ }

    auto chat(QStringView prompt, const backend::GenerationParams &params, /*out*/ ChatResponseMetadata &metadata)
        -> QCoro::AsyncGenerator<QString> override;

private:
    OpenaiConnectionDetails  m_connDetails;
    QNetworkAccessManager   *m_nam;
};


} // namespace gpt4all::ui
