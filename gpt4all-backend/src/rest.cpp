#include "rest.h"

#include <QNetworkReply>
#include <QRestReply>
#include <QString>

using namespace Qt::Literals::StringLiterals;


namespace gpt4all::backend {


QString restErrorString(const QRestReply &reply)
{
    auto *nr = reply.networkReply();
    if (reply.hasError())
        return nr->errorString();

    if (!reply.isHttpStatusSuccess()) {
        auto code   = reply.httpStatus();
        auto reason = nr->attribute(QNetworkRequest::HttpReasonPhraseAttribute);
        return u"HTTP %1%2%3 for URL \"%4\""_s.arg(
            QString::number(code),
            reason.isValid() ? u" "_s : QString(),
            reason.toString(),
            nr->request().url().toString()
        );
    }

    Q_UNREACHABLE();
}


} // namespace gpt4all::backend
