#pragma once

#include "providers/pronouns/IPronounsApi.hpp"
#include "providers/pronouns/common/PronounsApiRequest.hpp"
#include "providers/pronouns/UserPronouns.hpp"
#include "providers/pronouns/PronounUser.hpp"
#include <QJsonArray>

namespace chatterino {

class PronounsPronounDbApi : public IPronounsApi {
public:
  using UserId = std::string;
  virtual void fetch(std::vector<PronounUser> users, CallbackT onDone) override;
private:
  static inline UserPronouns setsToPronouns(std::vector<std::string> sets);
  static std::optional<std::vector<std::pair<UserId, UserPronouns>>> parse(QJsonObject);
  static UserPronouns parseUser(QJsonObject);
  static std::string makeRequest(std::vector<PronounUser> & users);
};

} // namespace chatterino