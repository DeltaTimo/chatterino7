#pragma once

#include "providers/pronouns/IPronounsApi.hpp"
#include "providers/pronouns/common/PronounsApiRequest.hpp"
#include "providers/pronouns/UserPronouns.hpp"
#include "providers/pronouns/PronounUser.hpp"

namespace chatterino {

class PronounsAlejoApi : public IPronounsApi {
private:
  static std::unordered_map<std::string, std::string> pronounsFromId;
  static UserPronouns parse(QJsonArray);
public:
  virtual void fetch(std::vector<PronounUser> users, CallbackT onDone) override;
};

} // namespace chatterino