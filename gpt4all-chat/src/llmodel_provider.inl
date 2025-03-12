#include <fmt/format.h>


namespace gpt4all::ui {


template <typename T, typename S, typename C>
void GenerationParams::tryParseValue(this S &self, QMap<GenerationParam, QVariant> &values, GenerationParam key,
                                     T C::* dest)
{
    if (auto value = tryParseValue(values, key, QMetaType::fromType<T>()); value.isValid())
        self.*dest = value.template value<T>();
}

template <typename T, typename S, typename C>
void ModelProviderMutable::setMemberProp(this S &self, T C::* member, std::string_view name, T value)
{
    auto &mpc = static_cast<ModelProviderMutable &>(self);
    auto &cur = self.*member;
    if (cur != value) {
        cur = std::move(value);
        auto data = mpc.asData();
        if (auto res = mpc.m_store->setData(std::move(data)); !res)
            res.error().raise();
        QMetaObject::invokeMethod(self.asQObject(), fmt::format("{}Changed", name).c_str(), cur);
    }
}


} // namespace gpt4all::ui
