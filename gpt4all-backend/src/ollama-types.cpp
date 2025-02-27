#include "ollama-types.h"

#include "json-helpers.h"

#include <type_traits>
#include <utility>

namespace json = boost::json;


namespace gpt4all::backend::ollama {

ModelInfo tag_invoke(const boost::json::value_to_tag<ModelInfo> &, const boost::json::value &value)
{
    using namespace json;
    auto &o = value.as_object();
    return {
#define T(name) std::remove_reference_t<decltype(std::declval<ModelInfo>().name)>
        .license    = value_to<T(license   )>(o.at("license"   )),
        .modelfile  = value_to<T(modelfile )>(o.at("modelfile" )),
        .parameters = value_to<T(parameters)>(o.at("parameters")),
        .template_  = value_to<T(template_ )>(o.at("template"  )), // :(
        .system     = value_to<T(system    )>(o.at("system"    )),
        .details    = value_to<T(details   )>(o.at("details"   )),
        .model_info = value_to<T(model_info)>(o.at("model_info")),
        .messages   = value_to<T(messages  )>(o.at("messages"  )),
#undef T
    };
}

} // namespace gpt4all::backend::ollama
