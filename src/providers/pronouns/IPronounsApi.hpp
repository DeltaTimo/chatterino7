#pragma once

#include <optional>
#include <string>
#include <vector>
#include <unordered_map>
#include "providers/pronouns/UserPronouns.hpp"
#include "providers/pronouns/common/PronounsApiRequest.hpp"
#include "providers/pronouns/PronounUser.hpp"

namespace chatterino {

class IPronounsApi {
public:
  using ResultT = std::pair<std::string, UserPronouns>;
  using ResultsT = std::vector<ResultT>;
  using RequestT = PronounsApiRequest<ResultT>;
  using CallbackT = std::function<void(ResultsT)>;

  // Retrieve from APIs.
  virtual void fetch(std::vector<PronounUser> users, CallbackT onDone) {};
};

} // namespace chatterino
