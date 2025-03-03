#include "openai.h"

#include "mysettings.h"
#include "utils.h"

#include <QCoro/QCoroAsyncGenerator> // IWYU pragma: keep
#include <QCoro/QCoroNetworkReply> // IWYU pragma: keep
#include <fmt/format.h>
#include <gpt4all-backend/formatters.h>
#include <gpt4all-backend/generation-params.h>
#include <gpt4all-backend/rest.h>

#include <QByteArray>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QLatin1String>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QRestAccessManager>
#include <QRestReply>
#include <QStringView>
#include <QUrl>
#include <QUtf8StringView> // IWYU pragma: keep
#include <QVariant>
#include <QXmlStreamReader>
#include <Qt>

#include <expected>
#include <optional>
#include <utility>

using namespace Qt::Literals::StringLiterals;

//#define DEBUG


static auto processRespLine(const QByteArray &line) -> std::optional<QString>
{
    auto jsonData = line.trimmed();
    if (jsonData.startsWith("data:"_ba))
        jsonData.remove(0, 5);
    jsonData = jsonData.trimmed();
    if (jsonData.isEmpty())
        return std::nullopt;
    if (jsonData == "[DONE]")
        return std::nullopt;

    QJsonParseError err;
    auto document = QJsonDocument::fromJson(jsonData, &err);
    if (document.isNull())
        throw std::runtime_error(fmt::format("OpenAI chat response parsing failed: {}", err.errorString()));

    auto root = document.object();
    auto choices = root.value("choices").toArray();
    auto choice = choices.first().toObject();
    auto delta = choice.value("delta").toObject();
    return delta.value("content").toString();
}


namespace gpt4all::ui {


void OpenaiModelDescription::setDisplayName(QString value)
{
    if (m_displayName != value) {
        m_displayName = std::move(value);
        emit displayNameChanged(m_displayName);
    }
}

void OpenaiModelDescription::setModelName(QString value)
{
    if (m_modelName != value) {
        m_modelName = std::move(value);
        emit modelNameChanged(m_modelName);
    }
}

OpenaiLLModel::OpenaiLLModel(OpenaiConnectionDetails connDetails, QNetworkAccessManager *nam)
    : m_connDetails(std::move(connDetails))
    , m_nam(nam)
    {}

static auto parsePrompt(QXmlStreamReader &xml) -> std::expected<QJsonArray, QString>
{
    QJsonArray messages;

    auto xmlError = [&xml] {
        return std::unexpected(u"%1:%2: %3"_s.arg(xml.lineNumber()).arg(xml.columnNumber()).arg(xml.errorString()));
    };

    if (xml.hasError())
        return xmlError();
    if (xml.atEnd())
        return messages;

    // skip header
    bool foundElement = false;
    do {
        switch (xml.readNext()) {
            using enum QXmlStreamReader::TokenType;
        case Invalid:
            return xmlError();
        case EndDocument:
            return messages;
        default:
            foundElement = true;
        case StartDocument:
        case Comment:
        case DTD:
        case ProcessingInstruction:
            ;
        }
    } while (!foundElement);

    // document body loop
    bool foundRoot = false;
    for (;;) {
        switch (xml.tokenType()) {
            using enum QXmlStreamReader::TokenType;
        case StartElement:
            {
                auto name = xml.name();
                if (!foundRoot) {
                    if (name != "chat"_L1)
                        return std::unexpected(u"unexpected tag: %1"_s.arg(name));
                    foundRoot = true;
                } else {
                    if (name != "user"_L1 && name != "assistant"_L1 && name != "system"_L1)
                        return std::unexpected(u"unknown role: %1"_s.arg(name));
                    auto content = xml.readElementText();
                    if (xml.tokenType() != EndElement)
                        return xmlError();
                    messages << makeJsonObject({
                        { "role"_L1,    name.toString().trimmed() },
                        { "content"_L1, content                   },
                    });
                }
                break;
            }
        case Characters:
            if (!xml.isWhitespace())
                return std::unexpected(u"unexpected text: %1"_s.arg(xml.text()));
        case Comment:
        case ProcessingInstruction:
        case EndElement:
            break;
        case EndDocument:
            return messages;
        case Invalid:
            return xmlError();
        default:
            return std::unexpected(u"unexpected token: %1"_s.arg(xml.tokenString()));
        }
        xml.readNext();
    }
}

auto OpenaiLLModel::chat(QStringView prompt, const backend::GenerationParams &params,
                         /*out*/ ChatResponseMetadata &metadata) -> QCoro::AsyncGenerator<QString>
{
    auto *mySettings = MySettings::globalInstance();

    if (!params.n_predict)
        co_return; // nothing requested

    auto reqBody = makeJsonObject({
        { "model"_L1,                 m_connDetails.modelName  },
        { "max_completion_tokens"_L1, qint64(params.n_predict) },
        { "stream"_L1,                true                     },
        { "temperature"_L1,           params.temperature       },
        { "top_p"_L1,                 params.top_p             },
    });

    // conversation history
    {
        QXmlStreamReader xml(prompt);
        auto messages = parsePrompt(xml);
        if (!messages)
            throw std::invalid_argument(fmt::format("Failed to parse OpenAI prompt: {}", messages.error()));
        reqBody.insert("messages"_L1, *messages);
    }

    QNetworkRequest request(m_connDetails.baseUrl.resolved(QUrl("/v1/chat/completions")));
    request.setHeader(QNetworkRequest::UserAgentHeader, mySettings->userAgent());
    request.setRawHeader("authorization", u"Bearer %1"_s.arg(m_connDetails.apiKey).toUtf8());

    QRestAccessManager restNam(m_nam);
    std::unique_ptr<QNetworkReply> reply(restNam.post(request, QJsonDocument(reqBody)));

    auto makeError = [](const QRestReply &reply) {
        return std::runtime_error(fmt::format("OpenAI chat request failed: {}", backend::restErrorString(reply)));
    };

    QRestReply restReply(reply.get());
    if (reply->error())
        throw makeError(restReply);

    auto coroReply = qCoro(reply.get());
    for (;;) {
        auto line = co_await coroReply.readLine();
        if (!restReply.isSuccess())
            throw makeError(restReply);
        if (line.isEmpty()) {
            Q_ASSERT(reply->atEnd());
            break;
        }
        if (auto chunk = processRespLine(line))
            co_yield *chunk;
    }
}


} // namespace gpt4all::ui
