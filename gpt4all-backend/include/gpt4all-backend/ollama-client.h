#pragma once

#include "ollama-types.h"

#include <QCoro/QCoroTask> // IWYU pragma: keep

#include <QJsonParseError>
#include <QNetworkReply>
#include <QString>
#include <QUrl>

#include <cassert>
#include <expected>
#include <utility>
#include <variant>

class QNetworkRequest;
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
    OllamaClient(QUrl baseUrl, QString m_userAgent = QStringLiteral("GPT4All"))
        : m_baseUrl(baseUrl)
        , m_userAgent(std::move(m_userAgent))
        {}

    const QUrl &baseUrl() const { return m_baseUrl; }
    void getBaseUrl(QUrl value) { m_baseUrl = std::move(value); }

    /// Returns the version of the Ollama server.
    auto getVersion() -> QCoro::Task<DataOrRespErr<ollama::VersionResponse>>
    { return get<ollama::VersionResponse>(QStringLiteral("version")); }

    /// List models that are available locally.
    auto listModels() -> QCoro::Task<DataOrRespErr<ollama::ModelsResponse>>
    { return get<ollama::ModelsResponse>(QStringLiteral("tags")); }

    /// Show details about a model including modelfile, template, parameters, license, and system prompt.
    auto showModelInfo(const ollama::ModelInfoRequest &req) -> QCoro::Task<DataOrRespErr<ollama::ModelInfo>>
    { return post<ollama::ModelInfo>(QStringLiteral("show"), req); }

private:
    QNetworkRequest makeRequest(const QString &path) const;

    template <typename Resp>
    auto get(const QString &path) -> QCoro::Task<DataOrRespErr<Resp>>;
    template <typename Resp, typename Req>
    auto post(const QString &path, Req const &req) -> QCoro::Task<DataOrRespErr<Resp>>;

    auto getJson(const QString &path) -> QCoro::Task<DataOrRespErr<boost::json::value>>;
    auto postJson(const QString &path, const boost::json::value &req)
        -> QCoro::Task<DataOrRespErr<boost::json::value>>;

private:
    QUrl                  m_baseUrl;
    QString               m_userAgent;
    QNetworkAccessManager m_nam;
};

extern template auto OllamaClient::get(const QString &) -> QCoro::Task<DataOrRespErr<ollama::VersionResponse>>;
extern template auto OllamaClient::get(const QString &) -> QCoro::Task<DataOrRespErr<ollama::ModelsResponse>>;

extern template auto OllamaClient::post(const QString &, const ollama::ModelInfoRequest &)
    -> QCoro::Task<DataOrRespErr<ollama::ModelInfo>>;

} // namespace gpt4all::backend
