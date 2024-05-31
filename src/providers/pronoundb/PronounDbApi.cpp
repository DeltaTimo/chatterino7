#include "providers/pronoundb/PronounDbPronouns.hpp"
#include "providers/pronoundb/PronounDbApi.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "common/Common.hpp"
#include "common/network/NetworkRequest.hpp"
#include "common/network/NetworkResult.hpp"
#include <string>
#include <vector>
#include <sstream>
#include <unordered_map>

namespace chatterino {

std::string PronounDbApi::endpoint(std::string endpoint_) {
  return PronounDb::ApiUrl + endpoint_;
}

std::string PronounDbApi::alejo_endpoint(std::string endpoint_) {
  return PronounDb::AlejoApiUrl + endpoint_;
}

std::string PronounDbApi::lookup(std::string platform, std::vector<std::string> ids) {
  std::stringstream s;
  s << "api/v2/lookup?platform=" << platform << "&ids=";
  for (std::size_t i {0} ; i < ids.size(); ++i) {
    s << ids[i];
    if (i != ids.size()) {
      s << ",";
    }
  }
  return s.str();
}

std::string PronounDbApi::alejo_user(std::string username) {
  return std::string("api/users/") + username;
}

void PronounDbApi::getForUser(std::string userName, std::string userId) {
  std::unordered_map<std::string, std::string> userNameToId;
  userNameToId[userName] = userId;
  getForUsers(userNameToId);
}

void PronounDbApi::alejoGetForUser(std::string username) {
  NetworkRequest(PronounDbApi::alejo_endpoint(PronounDbApi::alejo_user(username)))
    .concurrent()
    // .cache()
    .onSuccess([this, username](auto result) {
      auto json = result.parseJsonArray();
      if (json.size() == 0) {
        return;
      }

      auto first = json.first();
      
      if (!first.isObject()) {
        return;
      }

      auto pronoun = first.toObject()["pronoun_id"];

      if (pronoun.isUndefined()) {
        return;
      }

      if (!pronoun.isString()) {
        return;
      }

      if (pronoun.isString()) {
        auto pronounStr = pronoun.toString();
        auto alejoPronoun = PronounDbPronouns::fromAlejo(pronounStr.toStdString());
        pronouns[username] = alejoPronoun;
      }
    })
    .execute();
}

// This function does the heavy lifting.
void PronounDbApi::getForUsers(std::unordered_map<std::string, std::string> userNameToId) {
  // PronounDb only supports up to 50 ids in a single request.
  if (userNameToId.size() > 50) {
    std::vector<std::unordered_map<std::string, std::string>> maps;
    std::size_t i {0};
    for (auto const & kv : userNameToId) {
      if (i % 50 == 0) {
        maps.push_back(std::unordered_map<std::string, std::string>());
      }
      maps.back()[kv.first] = kv.second;
      ++i;
    }
    for (auto const & map : maps) {
      getForUsers(map);
    }
    // Don't continue. The above calls have already done the work.
    return;
  }

  std::unordered_map<std::string, std::string> userIdToName;
  std::vector<std::string> userIds;
  for (auto const & kv : userNameToId) {
    if (pronouns.find(kv.first) == pronouns.end()) {
      userIds.push_back(kv.second);
      userIdToName[kv.second] = kv.first;
      pronouns[kv.first] = PronounDbPronouns();
      std::cout << "checking pronouns for " << kv.first << "(" << kv.second << ")" << std::endl;
    }
  }

  // Up to 50 ids in a request, send request.
  NetworkRequest(PronounDbApi::endpoint(PronounDbApi::lookup("twitch", userIds)))
    .concurrent()
    // .cache()
    .onSuccess([this, userIdToName](auto result) {
      auto json = result.parseJson();

      if (json.isEmpty()) {
        for (auto const & kv : userIdToName) {
          alejoGetForUser(kv.second);
        }
        return;
      }

      // Json object looks like: { "USER_ID1": { "sets": { "set_id": ["my", "pronoun", "sets"] } }, "USER_ID2": ... }
      for (const QString & user : json.keys()) {
        std::string name = userIdToName.at(user.toStdString());

        if (!json[user].isObject()) {
          // Fallback to Alejo.
          alejoGetForUser(name);
          continue;
        }

        // user = USER_ID
        auto sets = json[user].toObject()["sets"];
        if (!sets.isObject()) {
          // Fallback to Alejo.
          alejoGetForUser(name);
          continue;
        }

        // Only "en" set is currently supported.
        auto set = sets.toObject()["en"];
        if (set.isUndefined()) {
          // Fallback to Alejo.
          alejoGetForUser(name);
          continue;
        } else if (!set.isArray()) {
          // Fallback to Alejo.
          alejoGetForUser(name);
          continue;
        }

        QJsonArray jsonPronouns = set.toArray();
        if (jsonPronouns.count() == 0) {
          // Fallback to Alejo.
          alejoGetForUser(name);
          continue;
        }

        std::vector<std::string> pronounSets;
        for (auto const pronoun : jsonPronouns) {
          if (!pronoun.isString()) {
            continue;
          }

          pronounSets.push_back(pronoun.toString().toStdString());
        }

        // Assign pronouns to user!
        pronouns[name] = PronounDbPronouns::fromSets(pronounSets);
        std::cout << "Adding pronouns for " << name << ": " << pronouns[name].display << std::endl;
      }
    })
    .execute();
}

void PronounDbApi::getFromMessage(MessagePtr message) {
  getFromMessages({message});
}

void PronounDbApi::getFromMessages(std::vector<MessagePtr> messages) {
  std::vector<std::string> usernames;
  for (auto const & messagePtr : messages) {
    usernames.push_back(messagePtr->loginName.toStdString());
  }

  getFromUsernames(usernames);
}

void PronounDbApi::getFromUsernames(std::vector<std::string> usernames_) {
  QStringList usernames;
  for (auto const & username : usernames_) {
    auto iter = pronouns.find(username);
    if (iter != pronouns.end()) {
      // Already got the pronouns!
      continue;
    }
    usernames.append(QString::fromStdString(username));
    std::cout << "fetching ids for " << username << std::endl;
  }

  if (usernames.empty()) {
    return;
  }

  getHelix()->fetchUsers(QStringList(), usernames, [this](std::vector<HelixUser> users) {
    std::vector<std::string> userIds;
    std::unordered_map<std::string, std::string> userNameToId;
    for (const HelixUser & user : users) {
      auto name = user.login.toStdString();
      userNameToId[name] = user.id.toStdString();
    }

    getForUsers(userNameToId);
  }, [](){});
}

std::optional<std::string> PronounDbApi::getPronounsForUsername(std::string username) {
  auto iter = pronouns.find(username);
  if (iter != pronouns.end()) {
    return {iter->second.display};
  }
  
  // Fetch for next message...
  // getFromUsernames({username});

  return {};
}

}