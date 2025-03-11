#include "store_provider.h"

#include <utility>


namespace gpt4all::ui {


auto ProviderStore::create(QString name, QUrl base_url, QString api_key)
    -> DataStoreResult<const ModelProviderData *>
{
    ModelProviderData data { QUuid::createUuid(), ProviderType::openai, name, std::move(base_url),
                             OpenaiProviderDetails { std::move(api_key) }                          };
    return createImpl(std::move(data), name);
}

auto ProviderStore::create(QString name, QUrl base_url)
    -> DataStoreResult<const ModelProviderData *>
{
    ModelProviderData data { QUuid::createUuid(), ProviderType::ollama, name, std::move(base_url), {} };
    return createImpl(std::move(data), name);
}


} // namespace gpt4all::ui
