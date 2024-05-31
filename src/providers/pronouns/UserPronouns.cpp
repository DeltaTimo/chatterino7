#include "providers/pronouns/UserPronouns.hpp"
#include <string>
#include <optional>

namespace chatterino {

UserPronouns::UserPronouns() {
  representation = std::nullopt;
}
UserPronouns::UserPronouns(std::string pronouns) {
  if (pronouns.length() == 0) {
    representation = {};
  } else {
    representation = {pronouns};
  }
}

bool UserPronouns::isUnspecified() const {
  return !representation.has_value();
}

UserPronouns::operator bool() const {
  return representation.has_value();
}

} // namespace chatterino
