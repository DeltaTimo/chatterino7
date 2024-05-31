#include "providers/pronouns/pronoundb/PronounsPronounDbApi.hpp"
#include "common/network/NetworkRequest.hpp"
#include "common/network/NetworkResult.hpp"
#include <iostream>
#include <sstream>
#include <unordered_set>

namespace chatterino {

/*static*/ UserPronouns PronounsPronounDbApi::setsToPronouns(std::vector<std::string> sets) {
  // This follows the rules of https://pronoundb.org/wiki/legacy-api-docs#apiv2-compatibility .
  if (std::find(sets.begin(), sets.end(), "any") != sets.end()) {
    return {"any"};
  } else if (std::find(sets.begin(), sets.end(), "ask") != sets.end()) {
    return {"ask"};
  }

  if (sets.size() == 0) {
    return {};
  }

  if (sets[0] == "avoid") {
    return {"avoid"};
  }

  if (sets.size() == 1) {
    if (sets[0] == "he") {
      return {"he/him"};
    } else if (sets[0] == "she") {
      return {"she/her"};
    } else if (sets[0] == "they") {
      return {"they/them"};
    } else if (sets[0] == "it") {
      return {"it/its"};
    } else {
      return {sets[0]};
    }
  } else if (sets[0] == "he" || sets[0] == "she" || sets[0] == "they" || sets[0] == "it") {
    if (sets[0] != sets[1]) {
      return {sets[0] + "/" + sets[1]};
    } else {
      return {sets[0]};
    }
  }

  return {"other"};
}

/*static*/ UserPronouns PronounsPronounDbApi::parseUser(QJsonObject user) {
  auto sets = user["sets"];
  if (!sets.isObject()) {
    return {};
  }

  auto set = sets.toObject()["en"];
  if (!set.isArray()) {
    return {};
  }

  auto jsonPronouns = set.toArray();
  if (jsonPronouns.count() == 0) {
    return {};
  }

  std::vector<std::string> pronounSets;
  for (auto const & pronoun : jsonPronouns) {
    if (!pronoun.isString()) {
      continue;
    }

    pronounSets.push_back(pronoun.toString().toStdString());
  }

  return setsToPronouns(pronounSets);
}

/*static*/ std::optional<std::vector<std::pair<PronounsPronounDbApi::UserId, UserPronouns>>> PronounsPronounDbApi::parse(QJsonObject object) {
  if (object.isEmpty()) {
    return {};
  }

  std::vector<std::pair<PronounsPronounDbApi::UserId, UserPronouns>> pronouns;
  for (auto const & qUserId : object.keys()) {
    auto userId = qUserId.toStdString();
    auto user = object[qUserId];
    if (!user.isObject()) {
      pronouns.push_back({userId, {}});
    } else {
      pronouns.push_back({userId, parseUser(user.toObject())});
    }
  }
  return pronouns;
}

/*static*/ std::string PronounsPronounDbApi::makeRequest(std::vector<PronounUser> & users) {
  std::stringstream s;
  s << "https://pronoundb.org/api/v2/lookup?platform=twitch&ids=";
  for (std::size_t i {0}; i < users.size(); ++i) {
    s << users[i].id;
    if (i != users.size() - 1) {
      s << ",";
    }
  }
  return s.str();
}

void PronounsPronounDbApi::fetch(std::vector<PronounUser> allUsers, IPronounsApi::CallbackT onDone) {
  auto request = std::make_shared<IPronounsApi::RequestT>(allUsers.size(), onDone);

  for (std::size_t i {0}; i < allUsers.size(); i += 50) {
    auto end = static_cast<std::size_t>(std::min(allUsers.size() - i, 50ul));
    auto users = std::vector<PronounUser>(allUsers.begin() + i, allUsers.begin() + i + end);

    NetworkRequest(makeRequest(users))
      .concurrent()
      .onSuccess([request, users](auto result) {
        if (!request) return;

        // Parse
        auto object = result.parseJson();
        // return (user.login, pronouns)
        // pronouns may be std::nullopt
        auto results = parse(object);
        if (!results) {
          // Fail all users.
          for (auto const & user : users) {
            request->finishRequest({user.username, {}});
          }
        } else {
          std::unordered_map<UserId, PronounUser> idToUser;
          for (auto const & user : users) {
            idToUser[user.id] = user;
          }
          
          std::unordered_set<UserId> receivedUsers;
          for (auto const & user : *results) {
            auto iter = idToUser.find(user.first);
            if (iter != idToUser.end() && user.second) {
              request->finishRequest({iter->second.username, user.second});
              receivedUsers.insert(iter->second.id);
            }
          }

          // Fail requests for remaining users.
          for (auto const & user : users) {
            if (receivedUsers.find(user.id) == receivedUsers.end()) {
              request->finishRequest({user.username, {}});
            }
          }
        }
      })
      .onError([request, users](auto error) {
        if (!request) return;

        for (auto const & user : users) {
          request->finishRequest({user.username, {}});
        }
      })
      .execute();
  }
}

} // namespace chatterino