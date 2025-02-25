
#include "config.h"

#include <QCoro/QCoroTask>
#include <fmt/format.h>
#include <gpt4all-backend/formatters.h>
#include <gpt4all-backend/main.h>

#include <QCoreApplication>
#include <QLatin1StringView>
#include <QTimer>

using gpt4all::backend::LLMProvider;


static void run()
{
    fmt::print("Connecting to server at {}\n", OLLAMA_URL);
    LLMProvider provider(OLLAMA_URL);
    auto version = QCoro::waitFor(provider.getVersion());
    if (version) {
        fmt::print("Server version: {}\n", *version);
    } else {
        fmt::print("Network error: {}\n", version.error().errorString);
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
