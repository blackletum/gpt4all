#pragma once

#include "ollama-types.h"

#include <QCoro/QCoroTask> // IWYU pragma: keep
#include <boost/json.hpp> // IWYU pragma: keep
#include <boost/system.hpp> // IWYU pragma: keep

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QString>
#include <QUrl>

#include <cassert>
#include <expected>
#include <utility>
#include <variant>

class QNetworkRequest;
class QRestReply;


namespace gpt4all::backend {


struct ResponseError {
public:
    struct BadStatus { int code; };

private:
    using ErrorCode = std::variant<
        QNetworkReply::NetworkError,
        boost::system::error_code,
        BadStatus
    >;

public:
    ErrorCode error;
    QString   errorString;

    ResponseError(const QRestReply *reply);

    ResponseError(const boost::system::system_error &e)
        : error(e.code())
        , errorString(QString::fromUtf8(e.what()))
    {
        assert(e.code());
    }
};

template <typename T>
using DataOrRespErr = std::expected<T, ResponseError>;

class OllamaClient {
public:
    OllamaClient(QUrl baseUrl, QString m_userAgent)
        : m_baseUrl(baseUrl)
        , m_userAgent(std::move(m_userAgent))
        {}

    const QUrl &baseUrl() const { return m_baseUrl; }
    void getBaseUrl(QUrl value) { m_baseUrl = std::move(value); }

    /// Returns the Ollama server version as a string.
    auto version() -> QCoro::Task<DataOrRespErr<ollama::VersionResponse>>
    { return get<ollama::VersionResponse>(QStringLiteral("version")); }

    /// Lists models that are available locally.
    auto list() -> QCoro::Task<DataOrRespErr<ollama::ListResponse>>
    { return get<ollama::ListResponse>(QStringLiteral("tags")); }

    /// Obtains model information, including details, modelfile, license etc.
    auto show(const ollama::ShowRequest &req) -> QCoro::Task<DataOrRespErr<ollama::ShowResponse>>
    { return post<ollama::ShowResponse>(QStringLiteral("show"), req); }

private:
    QNetworkRequest makeRequest(const QString &path) const;

    auto processResponse(QNetworkReply &reply) -> QCoro::Task<DataOrRespErr<boost::json::value>>;

    template <typename Resp>
    auto get(const QString &path) -> QCoro::Task<DataOrRespErr<Resp>>;
    template <typename Resp, typename Req>
    auto post(const QString &path, Req const &body) -> QCoro::Task<DataOrRespErr<Resp>>;

    auto getJson(const QString &path) -> QCoro::Task<DataOrRespErr<boost::json::value>>;
    auto postJson(const QString &path, const boost::json::value &body)
        -> QCoro::Task<DataOrRespErr<boost::json::value>>;

private:
    QUrl                       m_baseUrl;
    QString                    m_userAgent;
    QNetworkAccessManager      m_nam;
    boost::json::stream_parser m_parser;
};

extern template auto OllamaClient::get(const QString &) -> QCoro::Task<DataOrRespErr<ollama::VersionResponse>>;
extern template auto OllamaClient::get(const QString &) -> QCoro::Task<DataOrRespErr<ollama::ListResponse>>;

extern template auto OllamaClient::post(const QString &, const ollama::ShowRequest &)
    -> QCoro::Task<DataOrRespErr<ollama::ShowResponse>>;


} // namespace gpt4all::backend
