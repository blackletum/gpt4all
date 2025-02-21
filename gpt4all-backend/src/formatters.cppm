module;

#include <QByteArray>
#include <QString>
#include <QStringView>
#include <QUtf8StringView>
#include <QVariant>

#include <string_view>

export module gpt4all.backend.formatters;
import fmt;


// fmtlib formatters for QString and QVariant

#define MAKE_FORMATTER(type, conversion)                                         \
    export template <>                                                           \
    struct fmt::formatter<type, char> : fmt::formatter<std::string_view, char> { \
        template <typename FmtContext>                                           \
        FmtContext::iterator format(const type &value, FmtContext &ctx) const    \
        {                                                                        \
            auto valueUtf8 = (conversion);                                       \
            std::string_view view(valueUtf8.cbegin(), valueUtf8.cend());         \
            return formatter<std::string_view, char>::format(view, ctx);         \
        }                                                                        \
    }

MAKE_FORMATTER(QUtf8StringView, value                    );
MAKE_FORMATTER(QStringView,     value.toUtf8()           );
MAKE_FORMATTER(QString,         value.toUtf8()           );
MAKE_FORMATTER(QVariant,        value.toString().toUtf8());
