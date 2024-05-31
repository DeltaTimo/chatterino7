#pragma once

#include <vector>
#include <unordered_map>
#include <optional>
#include "providers/twitch/TwitchUser.hpp"
#include "providers/pronoundb/PronounDbPronouns.hpp"
#include "messages/Message.hpp"

namespace chatterino {

namespace PronounDb {
constexpr const char *ApiUrl = "https://pronoundb.org/";
constexpr const char *AlejoApiUrl = "https://pronouns.alejo.io/";
}

class PronounDbApi : public std::enable_shared_from_this<PronounDbApi> {
private:
  std::unordered_map<std::string, PronounDbPronouns> pronouns;

  std::string endpoint(std::string endpoint_);
  std::string lookup(std::string platform, std::vector<std::string> ids);

  std::string alejo_endpoint(std::string endpoint_);
  std::string alejo_user(std::string username);

public:
  void alejoGetForUser(std::string username);
  void getForUser(std::string userName, std::string userId);
  void getForUsers(std::unordered_map<std::string, std::string> userNameToIds);
  void getFromUsernames(std::vector<std::string> usernames);
  void getFromMessages(std::vector<MessagePtr> messages);
  void getFromMessage(MessagePtr message);
  std::optional<std::string> getPronounsForUsername(std::string username);
};

}