#include "providers/pronoundb/PronounDbPronouns.hpp"
#include <vector>
#include <string>
#include <algorithm>
#include <unordered_map>

namespace chatterino {

std::unordered_map<std::string, std::string> PronounDbPronouns::alejoPronouns = {
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

PronounDbPronouns::PronounDbPronouns() {
  this->display = "unspecified";
}

PronounDbPronouns::PronounDbPronouns(std::string display) {
  this->display = display;
}

PronounDbPronouns PronounDbPronouns::fromSets(std::vector<std::string> sets) {
  // This follows the rules of https://pronoundb.org/wiki/legacy-api-docs#apiv2-compatibility .
  if (std::find(sets.begin(), sets.end(), "any") != sets.end()) {
    return PronounDbPronouns("any");
  } else if (std::find(sets.begin(), sets.end(), "ask") != sets.end()) {
    return PronounDbPronouns("ask");
  }

  if (sets.size() == 0) {
    return PronounDbPronouns("unspecified");
  }

  if (sets[0] == "avoid") {
    return PronounDbPronouns("avoid");
  }

  if (sets.size() == 1) {
    if (sets[0] == "he") {
      return PronounDbPronouns("he/him");
    } else if (sets[0] == "she") {
      return PronounDbPronouns("she/her");
    } else if (sets[0] == "they") {
      return PronounDbPronouns("they/them");
    } else if (sets[0] == "it") {
      return PronounDbPronouns("it/its");
    } else {
      return PronounDbPronouns(sets[0]);
    }
  } else if (sets[0] == "he" || sets[0] == "she" || sets[0] == "they" || sets[0] == "it") {
    if (sets[0] != sets[1]) {
      return PronounDbPronouns(sets[0] + "/" + sets[1]);
    } else {
      return PronounDbPronouns(sets[0]);
    }
  }

  return PronounDbPronouns("other");
}

PronounDbPronouns PronounDbPronouns::fromAlejo(std::string s) {
  auto iter = PronounDbPronouns::alejoPronouns.find(s);
  if (iter != PronounDbPronouns::alejoPronouns.end()) {
    return PronounDbPronouns(iter->second);
  }
  return PronounDbPronouns();
}

}