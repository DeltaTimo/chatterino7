#include "providers/recentmessages/Impl.hpp"

#include "common/Env.hpp"
#include "common/QLogging.hpp"
#include "providers/twitch/IrcMessageHandler.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchMessageBuilder.hpp"
#include "singletons/Settings.hpp"
#include "util/FormatTime.hpp"

#include <QJsonArray>
#include <QUrlQuery>

namespace {

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
const auto &LOG = chatterinoRecentMessages;

}  // namespace

namespace chatterino::recentmessages::detail {

// convertClearchatToNotice takes a Communi::IrcMessage that is a CLEARCHAT
// command and converts it to a readable NOTICE message. This has
// historically been done in the Recent Messages API, but this functionality
// has been moved to Chatterino instead.
Communi::IrcMessage *convertClearchatToNotice(Communi::IrcMessage *message)
{
    auto channelName = message->parameter(0);
    QString noticeMessage{};
    if (message->tags().contains("target-user-id"))
    {
        auto target = message->parameter(1);

        if (message->tags().contains("ban-duration"))
        {
            // User was timed out
            noticeMessage =
                QString("%1 has been timed out for %2.")
                    .arg(target)
                    .arg(formatTime(message->tag("ban-duration").toString()));
        }
        else
        {
            // User was permanently banned
            noticeMessage =
                QString("%1 has been permanently banned.").arg(target);
        }
    }
    else
    {
        // Chat was cleared
        noticeMessage = "Chat has been cleared by a moderator.";
    }

    // rebuild the raw IRC message so we can convert it back to an ircmessage again!
    // this could probably be done in a smarter way

    auto s = QString(":tmi.twitch.tv NOTICE %1 :%2")
                 .arg(channelName)
                 .arg(noticeMessage);

    auto *newMessage = Communi::IrcMessage::fromData(s.toUtf8(), nullptr);
    newMessage->setTags(message->tags());

    return newMessage;
}

// Parse the IRC messages returned in JSON form into Communi messages
std::vector<Communi::IrcMessage *> parseRecentMessages(
    const QJsonObject &jsonRoot)
{
    const auto jsonMessages = jsonRoot.value("messages").toArray();
    std::vector<Communi::IrcMessage *> messages;

    if (jsonMessages.empty())
    {
        return messages;
    }

    for (const auto &jsonMessage : jsonMessages)
    {
        auto content = jsonMessage.toString();

        // For explanation of why this exists, see src/providers/twitch/TwitchChannel.hpp,
        // where these constants are defined
        content.replace(COMBINED_FIXER, ZERO_WIDTH_JOINER);

        auto *message =
            Communi::IrcMessage::fromData(content.toUtf8(), nullptr);

        if (message->command() == "CLEARCHAT")
        {
            message = convertClearchatToNotice(message);
        }

        messages.emplace_back(message);
    }

    return messages;
}

// Build Communi messages retrieved from the recent messages API into
// proper chatterino messages.
std::vector<MessagePtr> buildRecentMessages(
    std::vector<Communi::IrcMessage *> &messages, Channel *channel)
{
    auto &handler = IrcMessageHandler::instance();
    std::vector<MessagePtr> allBuiltMessages;

    for (auto *message : messages)
    {
        if (message->tags().contains("rm-received-ts"))
        {
            const auto msgDate =
                QDateTime::fromMSecsSinceEpoch(
                    message->tags().value("rm-received-ts").toLongLong())
                    .date();

            // Check if we need to insert a message stating that a new day began
            if (msgDate != channel->lastDate_)
            {
                channel->lastDate_ = msgDate;
                auto msg = makeSystemMessage(
                    QLocale().toString(msgDate, QLocale::LongFormat),
                    QTime(0, 0));
                msg->flags.set(MessageFlag::RecentMessage);
                allBuiltMessages.emplace_back(msg);
            }
        }

        auto builtMessages =
            handler.parseMessageWithReply(channel, message, allBuiltMessages);

        for (const auto &builtMessage : builtMessages)
        {
            builtMessage->flags.set(MessageFlag::RecentMessage);
            allBuiltMessages.emplace_back(builtMessage);
        }

        message->deleteLater();
    }

    return allBuiltMessages;
}

// Returns the URL to be used for querying the Recent Messages API for the
// given channel.
QUrl constructRecentMessagesUrl(const QString &name)
{
    QUrl url(Env::get().recentMessagesApiUrl.arg(name));
    QUrlQuery urlQuery(url);
    if (!urlQuery.hasQueryItem("limit"))
    {
        urlQuery.addQueryItem(
            "limit", QString::number(getSettings()->twitchMessageHistoryLimit));
    }
    url.setQuery(urlQuery);
    return url;
}

}  // namespace chatterino::recentmessages::detail