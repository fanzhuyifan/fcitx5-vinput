#include <cstdlib>
#include <iostream>
#include <string_view>

#include "daemon/postprocess/prompt_template.h"

namespace {

void Require(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "prompt template test failed: " << message << '\n';
    std::exit(1);
  }
}

} // namespace

int main() {
  using vinput::prompt_template::HasCommandInputInterpolation;
  using vinput::prompt_template::HasInterpolation;

  Require(HasCommandInputInterpolation("before {{selected}} after"),
          "selected text enables explicit command-input layout");
  Require(HasCommandInputInterpolation("before {{ asr }} after"),
          "a whitespace-padded ASR variable enables explicit command-input layout");
  Require(HasInterpolation("{{context}}"), "context still enables general interpolation");
  Require(!HasCommandInputInterpolation("{{context}}"),
          "a context-only prompt retains automatic command-input blocks");
  Require(HasInterpolation("{{unknown}}"), "unknown variables remain general interpolation");
  Require(!HasCommandInputInterpolation("{{unknown}}"),
          "an unknown variable retains automatic command-input blocks");

  return 0;
}
