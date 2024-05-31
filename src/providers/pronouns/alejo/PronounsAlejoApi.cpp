#include "providers/pronouns/alejo/PronounsAlejoApi.hpp"
#include "common/network/NetworkRequest.hpp"
#include "common/network/NetworkResult.hpp"

namespace chatterino {

/*static*/ std::unordered_map<std::string, std::string> PronounsAlejoApi::pronounsFromId = {
  {"aeaer", "ae/aer"},
  {"any", "any"},
  {"eem", "e/em"},
  {"faefaer", "fae/faer"},
  {"hehim", "he/him"},
  {"heshe", "he/she"},
  {"hethem", "he/they"},
  {"itits", "it/its"},
  {"other", "other"},
  {"perper", "per/per"},
  {"sheher", "she/her"},
  {"shethem", "she/they"},
  {"theythem", "they/them"},
  {"vever", "ve/ver"},
  {"xexem", "xe/xem"},
  {"ziehir", "zie/hir"},
};

/*static*/ UserPronouns PronounsAlejoApi::parse(QJsonArray array) {
  if (array.isEmpty()) {
    return {};
  }

  if (array.size() == 0) {
    return {};
  }

  auto first = array.first();
  
  if (!first.isObject()) {
    return {};
  }

  auto pronoun = first.toObject()["pronoun_id"];

  if (pronoun.isUndefined()) {
    return {};
  }

  if (!pronoun.isString()) {
    return {};
  }

  auto pronounQStr = pronoun.toString();
  auto pronounStr = pronounQStr.toStdString();
  auto iter = pronounsFromId.find(pronounStr);
  if (iter != pronounsFromId.end()) {
    return { iter->second };
  } else {
    return {};
  }
}

void PronounsAlejoApi::fetch(std::vector<PronounUser> users, IPronounsApi::CallbackT onDone) {
  auto request = std::make_shared<IPronounsApi::RequestT>(users.size(), onDone);

  for (auto const & user : users) {
    NetworkRequest(std::string("https://pronouns.alejo.io/api/users/") + user.username)
      .concurrent()
      .cache()
      .onSuccess([request, user](auto result) {
        if (!request) return;

        // Parse
        auto array = result.parseJsonArray();
        // return (user.login, pronouns)
        // pronouns may be std::nullopt
        request->finishRequest({user.username, parse(array)});
      })
      .onError([request, user](auto error) {
        if (!request) return;

        // error => return (user.login, std::nullopt)
        request->finishRequest({user.username, {}});
      })
      .execute();
  }
}

} // namespace chatterino