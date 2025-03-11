#pragma once

#include "store_base.h"

#include <boost/describe/class.hpp>
#include <boost/describe/enum.hpp>

#include <QString>
#include <QUrl>
#include <QUuid>

#include <variant>


namespace gpt4all::ui {


BOOST_DEFINE_ENUM_CLASS(ProviderType, openai, ollama)

struct OpenaiProviderDetails {
    QString api_key;
};
BOOST_DESCRIBE_STRUCT(OpenaiProviderDetails, (), (api_key))

struct ModelProviderData {
    using Details = std::variant<std::monostate, OpenaiProviderDetails>;
    QUuid        id;
    ProviderType type;
    QString      name;
    QUrl         base_url;
    Details      details;
};
BOOST_DESCRIBE_STRUCT(ModelProviderData, (), (id, type, name, base_url, details))

class ProviderStore : public DataStore<ModelProviderData> {
private:
    using Super = DataStore<ModelProviderData>;

public:
    using Super::Super;

    /// OpenAI
    auto create(QString name, QUrl base_url, QString api_key) -> DataStoreResult<const ModelProviderData *>;
    /// Ollama
    auto create(QString name, QUrl base_url) -> DataStoreResult<const ModelProviderData *>;
};


} // namespace gpt4all::ui
