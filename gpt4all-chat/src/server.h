#ifndef SERVER_H
#define SERVER_H

#include "chatllm.h"
#include "database.h"

#include <QHttpServerResponse>
#include <QJsonObject>
#include <QList>
#include <QObject> // IWYU pragma: keep
#include <QString>

#include <memory>
#include <optional>
#include <utility>

class Chat;
class ChatRequest;
class CompletionRequest;
namespace gpt4all::ui { class MwHttpServer; }


class Server : public ChatLLM
{
    Q_OBJECT

public:
    explicit Server(Chat *chat);
    ~Server() override;

public Q_SLOTS:
    void start();

Q_SIGNALS:
    void requestResetResponseState();

private:
    auto handleCompletionRequest(const CompletionRequest &request) -> std::pair<QHttpServerResponse, std::optional<QJsonObject>>;
    auto handleChatRequest(const ChatRequest &request) -> std::pair<QHttpServerResponse, std::optional<QJsonObject>>;

private Q_SLOTS:
    void handleDatabaseResultsChanged(const QList<ResultInfo> &results) { m_databaseResults = results; }
    void handleCollectionListChanged(const QList<QString> &collectionList) { m_collections = collectionList; }

private:
    Chat *m_chat;
    std::unique_ptr<gpt4all::ui::MwHttpServer> m_server;
    QList<ResultInfo> m_databaseResults;
    QList<QString> m_collections;
};

#endif // SERVER_H
