#pragma once

#include <QCoro/QCoroTask>

#include <QNetworkReply>
#include <QUrl>

#include <expected>

class QString;
template <typename T> class QFuture;


namespace gpt4all::backend {

struct NetErr {
    QNetworkReply::NetworkError error;
    QString                     errorString;
};

template <typename T>
using DataOrNetErr = std::expected<T, NetErr>;


class LLMProvider {
public:
    LLMProvider(QUrl baseUrl)
        : m_baseUrl(baseUrl)
        {}

    const QUrl &baseUrl() const { return m_baseUrl; }
    void getBaseUrl(QUrl value) { m_baseUrl = std::move(value); }

    /// Retrieve the Ollama version, e.g. "0.5.1"
    auto getVersion() const -> QCoro::Task<DataOrNetErr<QString>>;

private:
    QUrl m_baseUrl;
};

} // namespace gpt4all::backend
