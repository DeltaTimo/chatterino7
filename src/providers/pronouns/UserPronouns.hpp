#pragma once

#include <string>
#include <optional>

namespace chatterino {

class UserPronouns {
private:
  // TODO: Better representation?
  // if pronouns are specified by the user, std::string,
  // otherwise, std::nullopt.
  std::optional<std::string> representation;
public:
  UserPronouns();
  UserPronouns(std::string);

  inline std::string format() const {
    if (representation) {
      return *representation;
    } else {
      return "";
    }
  }

  bool isUnspecified() const;
  operator bool() const;
};

} // namespace chatterino
