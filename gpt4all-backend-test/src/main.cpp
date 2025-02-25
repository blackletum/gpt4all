
#include "config.h"

#include <QCoro/QCoroTask>
#include <fmt/format.h>
#include <gpt4all-backend/formatters.h>
#include <gpt4all-backend/main.h>

#include <QCoreApplication>
#include <QLatin1StringView>

using gpt4all::backend::LLMProvider;


int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    fmt::print("Connecting to server at {}\n", OLLAMA_URL);
    LLMProvider provider(OLLAMA_URL);
    auto version = QCoro::waitFor(provider.getVersion());
    if (version) {
        fmt::print("Server version: {}\n", *version);
    } else {
        fmt::print("Network error: {}\n", version.error().errorString);
        return 1;
    }
}
