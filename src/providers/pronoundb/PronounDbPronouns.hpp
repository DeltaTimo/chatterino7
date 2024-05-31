#pragma once
#include <string>
#include <vector>
#include <unordered_map>

namespace chatterino {

struct PronounDbPronouns {
  std::string display;
  static PronounDbPronouns fromSets(std::vector<std::string>);
  static PronounDbPronouns fromAlejo(std::string);
  PronounDbPronouns();
  PronounDbPronouns(std::string display);
private:
  static std::unordered_map<std::string, std::string> alejoPronouns;
};

}