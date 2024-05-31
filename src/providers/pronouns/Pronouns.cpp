#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include "providers/pronouns/IPronounsApi.hpp"
#include "providers/pronouns/Pronouns.hpp"
#include "common/network/NetworkRequest.hpp"
#include "common/network/NetworkResult.hpp"
#include "providers/pronouns/alejo/PronounsAlejoApi.hpp"
#include "providers/pronouns/pronoundb/PronounsPronounDbApi.hpp"
#include "providers/twitch/api/Helix.hpp"

namespace chatterino {

Pronouns::Pronouns() :
  providerPriority{PronounProvider::PRONOUNDB, PronounProvider::ALEJO} {
}

std::vector<std::string> Pronouns::filterOutKnownUsers(std::vector<std::string> &usernames) {
  std::vector<std::string> new_usernames;
  new_usernames.reserve(usernames.size());

  for (auto const & username : usernames) {
    auto iter = saved.find(username);
    if (iter == saved.end()) {
      // No pronouns found yet.
      new_usernames.push_back(username);
    }
  }

  return new_usernames;
}

std::shared_ptr<IPronounsApi> Pronouns::getProvider(PronounProvider provider) {
  auto iter = providers.find(provider);

  if (iter != providers.end()) {
    return iter->second;
  }

  switch (provider) {
    case PronounProvider::ALEJO: {
      auto ptr = std::make_shared<PronounsAlejoApi>();
      providers.insert_or_assign(PronounProvider::ALEJO, ptr);
      return ptr;
    }
    case PronounProvider::PRONOUNDB: {
      auto ptr = std::make_shared<PronounsPronounDbApi>();
      providers.insert_or_assign(PronounProvider::PRONOUNDB, ptr);
      return ptr;
    }
  }

  return {};
}

void Pronouns::fetchForUsers(std::vector<PronounUser> allUsers, std::unordered_set<PronounProvider> doneProviders) {
  for (auto const providerId : providerPriority) {
    // Provider already done.
    if (doneProviders.find(providerId) != doneProviders.end()) {
      continue;
    }

    auto provider = getProvider(providerId);
    if (!provider) {
      continue;
    }

    std::vector<PronounUser> users;
    for (auto const & user : allUsers) {
      auto iter = saved.find(user.username);
      if (iter == saved.end() || !iter->second) {
        // Only add missing users.
        users.push_back(user);
      }
    }

    provider->fetch(users, [this, users, providerId = std::move(providerId), doneProviders = std::move(doneProviders)](IPronounsApi::ResultsT results) mutable {
      {
        std::lock_guard<std::mutex> lock(this->mutex);

        for (auto const & result : results) {
          if (result.second) {
            this->saved[result.first] = result.second;
          }
        }
      }

      // Try the rest of the providers (recursively).
      doneProviders.insert(providerId);
      fetchForUsers(users, doneProviders);
    });
    return;

  }
}

void Pronouns::fetchForUsernames(std::vector<std::string> usernames) {
  // Don't get pronouns for users who we already have pronouns for.
  usernames = filterOutKnownUsers(usernames);

  // Get user ids.
  QStringList qUsernames;
  for (auto const & username : usernames) {
    qUsernames.append(QString::fromStdString(username));
  }

  if (qUsernames.empty()) {
    return;
  }

  getHelix()->fetchUsers(QStringList(), qUsernames,
      [this](std::vector<HelixUser> helixUsers) {
        // On success.
        // Fetch pronouns from providers.
        std::vector<PronounUser> users;
        for (auto const & helixUser : helixUsers) {
          PronounUser user;
          user.id = helixUser.id.toStdString();
          user.username = helixUser.login.toStdString();
          users.push_back(user);
        }
        this->fetchForUsers(users);
      },
      []() {
        // On error.
      });
}

std::optional<UserPronouns> Pronouns::getForUsername(std::string username) {
  auto iter = saved.find(username);
  if (iter == saved.end()) {
    return {};
  }

  return iter->second;
}

} // namespace chatterino
