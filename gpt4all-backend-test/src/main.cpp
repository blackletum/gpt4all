#include "config.h"

#include <QCoro/QCoroTask> // IWYU pragma: keep
#include <fmt/base.h>
#include <gpt4all-backend/formatters.h> // IWYU pragma: keep
#include <gpt4all-backend/ollama-client.h>

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

    auto versionResp = QCoro::waitFor(provider.getVersion());
    if (versionResp) {
        fmt::print("Server version: {}\n", versionResp->version);
    } else {
        fmt::print("Error retrieving version: {}\n", versionResp.error().errorString);
        return QCoreApplication::exit(1);
    }

    auto modelsResponse = QCoro::waitFor(provider.listModels());
    if (modelsResponse) {
        fmt::print("Available models:\n");
        for (const auto & model : modelsResponse->models)
            fmt::print("{}\n", model.model);
    } else {
        fmt::print("Error retrieving available models: {}\n", modelsResponse.error().errorString);
        return QCoreApplication::exit(1);
    }

    auto showResponse = QCoro::waitFor(provider.showModelInfo({ .model = "DeepSeek-R1-Distill-Llama-70B-Q4_K_S" }));
    if (showResponse) {
        fmt::print("Model family: {}\n", showResponse->details.family);
    } else {
        fmt::print("Error retrieving model info: {}\n", showResponse.error().errorString);
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
