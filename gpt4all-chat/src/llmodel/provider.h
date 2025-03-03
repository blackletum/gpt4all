#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QUrl>


namespace gpt4all::ui {


class ModelProvider : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString name READ name CONSTANT)

public:
    [[nodiscard]] virtual QString name() = 0;
};

class OpenaiProvider : public ModelProvider {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QUrl    baseUrl READ baseUrl WRITE setBaseUrl NOTIFY baseUrlChanged)
    Q_PROPERTY(QString apiKey  READ apiKey  WRITE setApiKey  NOTIFY apiKeyChanged)

public:
    [[nodiscard]] QString name() override  { return m_name;    }
    [[nodiscard]] const QUrl    &baseUrl() { return m_baseUrl; }
    [[nodiscard]] const QString &apiKey () { return m_apiKey;  }

    void setBaseUrl(QUrl    value);
    void setApiKey (QString value);

Q_SIGNALS:
    void baseUrlChanged(const QUrl    &value);
    void apiKeyChanged (const QString &value);

private:
    QString m_name;
    QUrl    m_baseUrl;
    QString m_apiKey;
};


} // namespace gpt4all::ui
