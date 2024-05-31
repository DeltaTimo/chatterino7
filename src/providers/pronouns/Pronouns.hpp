#pragma once

#include "providers/pronouns/UserPronouns.hpp"
#include <optional>
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include "providers/pronouns/PronounUser.hpp"
#include "providers/pronouns/IPronounsApi.hpp"

namespace chatterino {

enum class PronounProvider {
  PRONOUNDB,
  ALEJO
};

class Pronouns {
private:
  // mutex for editing the map saved.
  std::mutex mutex;
  // Login name -> Pronouns
  std::unordered_map<std::string, UserPronouns> saved;
  std::unordered_map<PronounProvider, std::shared_ptr<IPronounsApi>> providers;
  std::vector<PronounProvider> providerPriority;

  std::shared_ptr<IPronounsApi> getProvider(PronounProvider);

  std::vector<std::string> filterOutKnownUsers(std::vector<std::string> &usernames);

  void fetchForUsers(std::vector<PronounUser> users, std::unordered_set<PronounProvider> doneProviders = {});
public:
  Pronouns();

  // Retrieve from APIs.
  void fetchForUsernames(std::vector<std::string> usernames);

  // Getting from local state.
  std::optional<UserPronouns> getForUsername(std::string username);
};

} // namespace chatterino
