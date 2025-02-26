#pragma once

#include "ollama_responses.h"

#include <QCoro/QCoroTask> // IWYU pragma: keep

#include <QJsonParseError>
#include <QNetworkReply>
#include <QString>
#include <QUrl>

#include <cassert>
#include <expected>
#include <utility>
#include <variant>

namespace boost::json { class value; }


namespace gpt4all::backend {

struct ResponseError {
private:
    using ErrorCode = std::variant<
        QNetworkReply::NetworkError,
        std::exception_ptr
    >;

public:
    ErrorCode error;
    QString   errorString;

    ResponseError(const QNetworkReply *reply)
        : error(reply->error())
        , errorString(reply->errorString())
    {
        assert(reply->error());
    }

    ResponseError(const std::exception &e, std::exception_ptr err)
        : error(std::move(err))
        , errorString(e.what())
    {
        assert(std::get<std::exception_ptr>(error));
    }
};

template <typename T>
using DataOrRespErr = std::expected<T, ResponseError>;

class OllamaClient {
public:
    OllamaClient(QUrl baseUrl)
        : m_baseUrl(baseUrl)
        {}

    const QUrl &baseUrl() const { return m_baseUrl; }
    void getBaseUrl(QUrl value) { m_baseUrl = std::move(value); }

    /// Returns the version of the Ollama server.
    auto getVersion() -> QCoro::Task<DataOrRespErr<ollama::VersionResponse>>
    { return getSimple<ollama::VersionResponse>(QStringLiteral("version")); }

    /// List models that are available locally.
    auto listModels() -> QCoro::Task<DataOrRespErr<ollama::ModelsResponse>>
    { return getSimple<ollama::ModelsResponse>(QStringLiteral("tags")); }

private:
    template <typename T>
    auto getSimple(const QString &endpoint) -> QCoro::Task<DataOrRespErr<T>>;

    auto getSimpleGeneric(const QString &endpoint) -> QCoro::Task<DataOrRespErr<boost::json::value>>;

private:
    QUrl                  m_baseUrl;
    QNetworkAccessManager m_nam;
};

extern template auto OllamaClient::getSimple(const QString &) -> QCoro::Task<DataOrRespErr<ollama::VersionResponse>>;
extern template auto OllamaClient::getSimple(const QString &) -> QCoro::Task<DataOrRespErr<ollama::ModelsResponse>>;

} // namespace gpt4all::backend
