#include "llmodel_provider.h"

using namespace Qt::StringLiterals;


namespace gpt4all::ui {


// TODO: use these in the constructor of ProviderRegistry
// TODO: we have to be careful to reserve these names for ProviderStore purposes, so the user can't write JSON files that alias them.
// this *is a problem*, because we want to be able to safely introduce these.
// so we need a different namespace, i.e. a *different directory*.
const std::array<
    ProviderRegistry::BuiltinProviderData, ProviderRegistry::N_BUILTIN
> ProviderRegistry::s_builtinProviders {
    BuiltinProviderData {
        .id       = QUuid("20f963dc-1f99-441e-ad80-f30a0a06bcac"),
        .name     = u"Groq"_s,
        .base_url = u"https://api.groq.com/openai/v1/"_s,
    },
    BuiltinProviderData {
        .id       = QUuid("6f874c3a-f1ad-47f7-9129-755c5477146c"),
        .name     = u"OpenAI"_s,
        .base_url = u"https://api.openai.com/v1/"_s,
    },
    BuiltinProviderData {
        .id       = QUuid("7ae617b3-c0b2-4d2c-9ff2-bc3f049494cc"),
        .name     = u"Mistral"_s,
        .base_url = u"https://api.mistral.ai/v1/"_s,
    },
};


} // namespace gpt4all::ui
