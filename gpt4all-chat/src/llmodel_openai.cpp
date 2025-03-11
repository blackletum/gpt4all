#include "llmodel_openai.h"

#include "mysettings.h"
#include "utils.h"

#include <QCoro/QCoroAsyncGenerator> // IWYU pragma: keep
#include <QCoro/QCoroNetworkReply> // IWYU pragma: keep
#include <fmt/format.h>
#include <gpt4all-backend/formatters.h> // IWYU pragma: keep
#include <gpt4all-backend/rest.h>

#include <QAnyStringView>
#include <QByteArray>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QList>
#include <QMap>
#include <QMetaEnum>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRestAccessManager>
#include <QRestReply>
#include <QSet>
#include <QStringView>
#include <QUtf8StringView> // IWYU pragma: keep
#include <QVariant>
#include <QXmlStreamReader>
#include <QtAssert>

#include <coroutine>
#include <expected>
#include <memory>
#include <optional>
#include <stdexcept>
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


void OpenaiGenerationParams::parseInner(QMap<GenerationParam, QVariant> &values)
{
    tryParseValue(values, GenerationParam::NPredict,    &OpenaiGenerationParams::n_predict  );
    tryParseValue(values, GenerationParam::Temperature, &OpenaiGenerationParams::temperature);
    tryParseValue(values, GenerationParam::TopP,        &OpenaiGenerationParams::top_p      );
}

auto OpenaiGenerationParams::toMap() const -> QMap<QLatin1StringView, QVariant>
{
    return {
        {  "max_completion_tokens"_L1,  n_predict   },
        {  "temperature"_L1,            temperature },
        {  "top_p"_L1,                  top_p       },
    };
}

auto OpenaiProvider::supportedGenerationParams() const -> QSet<GenerationParam>
{
    using enum GenerationParam;
    return { NPredict, Temperature, TopP };
}

auto OpenaiProvider::makeGenerationParams(const QMap<GenerationParam, QVariant> &values) const
    -> OpenaiGenerationParams *
{ return new OpenaiGenerationParams(values); }

OpenaiProviderBuiltin::OpenaiProviderBuiltin(QUuid id, QString name, QUrl baseUrl, QString apiKey)
    : ModelProvider(std::move(id), std::move(name), std::move(baseUrl))
    , OpenaiProvider(std::move(apiKey))
    {}

/// load
OpenaiProviderCustom::OpenaiProviderCustom(std::shared_ptr<ProviderStore> store, QUuid id)
    : ModelProvider(std::move(id))
    , ModelProviderCustom(std::move(store))
{
    auto &details = load();
    m_apiKey = std::get<OpenaiProviderDetails>(details).api_key;
}

/// create
OpenaiProviderCustom::OpenaiProviderCustom(std::shared_ptr<ProviderStore> store, QString name, QUrl baseUrl,
                                           QString apiKey)
    : ModelProvider(std::move(name), std::move(baseUrl))
    , ModelProviderCustom(std::move(store))
    , OpenaiProvider(std::move(apiKey))
{
    auto data = m_store->create(m_name, m_baseUrl, m_apiKey);
    if (!data)
        data.error().raise();
    m_id = (*data)->id;
}

OpenaiModelDescription::OpenaiModelDescription(std::shared_ptr<const OpenaiProvider> provider, QString modelName)
    : m_provider(std::move(provider))
    , m_modelName(std::move(modelName))
    {}

auto OpenaiModelDescription::newInstance(QNetworkAccessManager *nam) const -> std::unique_ptr<OpenaiChatModel>
{ return std::unique_ptr<OpenaiChatModel>(&dynamic_cast<OpenaiChatModel &>(*newInstanceImpl(nam))); }

auto OpenaiModelDescription::newInstanceImpl(QNetworkAccessManager *nam) const -> ChatLLMInstance *
{ return new OpenaiChatModel({ shared_from_this(), this }, nam); }

OpenaiChatModel::OpenaiChatModel(std::shared_ptr<const OpenaiModelDescription> description, QNetworkAccessManager *nam)
    : m_description(std::move(description))
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

auto preload() -> QCoro::Task<>
{ co_return; /* not supported -> no-op */ }

auto OpenaiChatModel::generate(QStringView prompt, const GenerationParams &params,
                               /*out*/ ChatResponseMetadata &metadata) -> QCoro::AsyncGenerator<QString>
{
    auto *mySettings = MySettings::globalInstance();

    if (params.isNoop())
        co_return; // nothing requested

    auto reqBody = makeJsonObject({
        { "model"_L1,  m_description->modelName() },
        { "stream"_L1, true                       },
    });
    extend(reqBody, params.toMap());

    // conversation history
    {
        QXmlStreamReader xml(prompt);
        auto messages = parsePrompt(xml);
        if (!messages)
            throw std::invalid_argument(fmt::format("Failed to parse OpenAI prompt: {}", messages.error()));
        reqBody.insert("messages"_L1, *messages);
    }

    auto &provider = *m_description->provider();
    QNetworkRequest request(provider.baseUrl().resolved(QUrl("/v1/chat/completions")));
    request.setHeader(QNetworkRequest::UserAgentHeader, mySettings->userAgent());
    request.setRawHeader("authorization", u"Bearer %1"_s.arg(provider.apiKey()).toUtf8());

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
