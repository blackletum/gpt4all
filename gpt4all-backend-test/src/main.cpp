#include "config.h"

#include <QCoro/QCoroTask> // IWYU pragma: keep
#include <fmt/base.h>
#include <gpt4all-backend/formatters.h> // IWYU pragma: keep
#include <gpt4all-backend/ollama_client.h>

#include <QCoreApplication>
#include <QTimer>
#include <QString>
#include <QUrl>

#include <coroutine>
#include <expected>
#include <variant>

using gpt4all::backend::OllamaClient;


static void run()
{
    fmt::print("Connecting to server at {}\n", OLLAMA_URL);
    OllamaClient provider(OLLAMA_URL);
    auto version = QCoro::waitFor(provider.getVersion());
    if (version) {
        fmt::print("Server version: {}\n", version->version);
    } else {
        fmt::print("Error retrieving version: {}\n", version.error().errorString);
        return QCoreApplication::exit(1);
    }
    QCoreApplication::exit(0);
}


int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QTimer::singleShot(0, &run);
    return app.exec();
}
