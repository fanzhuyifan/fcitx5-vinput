#include <algorithm>
#include <array>
#include <chrono> // IWYU pragma: keep
#include <cstdint>
#include <cstdlib>
#include <fcitx-utils/key.h>
#include <fcitx-utils/keysym.h>
#include <fcitx-utils/keysymgen.h>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "input/modifier_gesture.h"

namespace {
using G = ModifierGesture;
using Mode = G::Mode;
using Action = G::Action;
using enum G::Event;
using std::chrono_literals::operator""ms;

// Each step defaults to no event. Expected events and filtering are specified
// at the point they should happen; elapse() advances time without firing timers.
struct Step {
  enum class Kind : std::uint8_t { Press, Release, Tick, Elapse, Reset, Reload, NoTimer };
  using enum Kind;
  Kind kind;
  // Explicit default initialization allows abbreviated aggregate steps with Fcitx Key.
  // NOLINTNEXTLINE(readability-redundant-member-init)
  fcitx::Key key{};
  G::Duration elapsed{};
  G::Event event = None;
  std::size_t binding = 0;
  std::optional<bool> filter = std::nullopt;
};

Step down(const std::string& key, G::Event event = None, std::optional<bool> filter = {}) {
  return {Step::Press, fcitx::Key(key), {}, event, 0, filter};
}
Step up(const std::string& key, G::Event event = None, std::size_t binding = 0) {
  return {Step::Release, fcitx::Key(key), {}, event, binding, {}};
}
Step tick(G::Duration elapsed = 0ms, G::Event event = None, std::size_t binding = 0) {
  return {Step::Tick, fcitx::Key{}, elapsed, event, binding, {}};
}
Step elapse(G::Duration elapsed) {
  return {Step::Elapse, fcitx::Key{}, elapsed, None, 0, {}};
}

G::Binding binding(const char* key, Mode mode = Mode::Both, Action action = Action::Command,
                   G::Duration delay = 300ms) {
  return {fcitx::Key(key), action, mode, delay,
          action == Action::Palette ? std::optional(250ms) : std::nullopt};
}

struct Scenario {
  std::string name;
  std::vector<G::Binding> config;
  std::vector<Step> sequence;
  bool toggle_recording = false;
};

void run(std::size_t& scenarios_run, const Scenario& test) {
  G gesture;
  gesture.setBindings(test.config);
  std::vector<fcitx::Key> pressed;
  std::vector<fcitx::Key> consumed;
  G::Clock::time_point now{};
  std::string trace = test.name;
  for (const auto& config : test.config) {
    trace += " [" + config.key.toString() +
             ", mode=" + std::to_string(static_cast<int>(config.mode)) + "]";
  }
  const auto expect = [&](bool condition, const std::string& message) {
    if (!condition) {
      std::cerr << trace << "\nFAIL: " << message << '\n';
      std::exit(1);
    }
  };
  constexpr std::array kinds = {"down ", "up ", "tick", "elapse", "reset", "reload", "no timer"};
  constexpr std::array events = {"None", "Tap", "HoldStart", "HoldRelease", "HoldCancel"};
  for (const auto& step : test.sequence) {
    now += step.elapsed;
    trace += " / " + std::to_string(step.elapsed.count()) + "ms " +
             kinds.at(static_cast<std::size_t>(step.kind)) + step.key.toString();
    G::Result result;
    if (step.kind == Step::Press || step.kind == Step::Release) {
      auto states = step.key.states();
      for (const auto& key : pressed) {
        states |= fcitx::Key::keySymToStates(key.sym());
      }
      const auto same_key = [&](const auto& key) {
        return key.code() && step.key.code() ? key.code() == step.key.code()
                                             : key.sym() == step.key.sym();
      };
      const auto found = std::find_if(pressed.begin(), pressed.end(), same_key);
      result = gesture.keyEvent(fcitx::Key(step.key.sym(), states, step.key.code()),
                                step.kind == Step::Release, now, test.toggle_recording);
      const auto owned = std::find_if(consumed.begin(), consumed.end(), same_key);
      if (step.kind == Step::Release || found != pressed.end()) {
        expect(result.filter == (owned != consumed.end()),
               "presses, repeats, and releases must have consistent application delivery");
      }
      if (step.kind == Step::Release && owned != consumed.end()) {
        consumed.erase(owned);
      } else if (result.filter && owned == consumed.end()) {
        consumed.push_back(step.key);
      }
      if (step.kind == Step::Release && found != pressed.end()) {
        pressed.erase(found);
      } else if (step.kind == Step::Press && found == pressed.end() &&
                 !states.test(fcitx::KeyState::Repeat)) {
        pressed.push_back(step.key);
      }
    } else if (step.kind == Step::Tick) {
      result = gesture.timeout(now);
    } else if (step.kind == Step::Reset) {
      gesture.cancel();
    } else if (step.kind == Step::Reload) {
      gesture.setBindings(test.config);
    } else if (step.kind == Step::NoTimer) {
      expect(!gesture.deadline(), "timer must be disarmed");
    }
    expect(result.event == step.event,
           std::string("expected ") + events.at(static_cast<std::size_t>(step.event)) + ", got " +
               events.at(static_cast<std::size_t>(result.event)));
    expect(result.binding.has_value() == (step.event != None), "only events carry a binding");
    if (result.binding) {
      const auto& expected = test.config.at(step.binding);
      expect(result.binding->key == expected.key && result.binding->action == expected.action,
             "event must route to the expected binding and action");
    }
    const auto expected_filter = step.filter;
    if (expected_filter) {
      expect(result.filter == *expected_filter, "unexpected key filtering");
    }
  }
  expect(!gesture.deadline(), "completed sequence must leave no timer");
  ++scenarios_run;
}

std::size_t examples() {
  std::size_t scenarios_run = 0;
  const fcitx::Key locked_control(FcitxKey_Control_L, fcitx::KeyStates(fcitx::KeyState::CapsLock) |
                                                          fcitx::KeyState::NumLock);
  for (auto mode : {Mode::Tap, Mode::Hold, Mode::Both}) {
    const auto tap = mode == Mode::Hold ? None : Tap;
    const auto start = mode == Mode::Tap ? None : HoldStart;
    const auto stop = mode == Mode::Tap ? Tap : HoldRelease;
    run(scenarios_run, {"tap and hold",
                        {binding("Control_L", mode)},
                        {down("Control_L"),
                         elapse(100ms),
                         up("Control_L", tap),
                         {Step::NoTimer},
                         down("Control_L"),
                         tick(300ms, start),
                         tick(100ms),
                         up("Control_L", stop)}});
    for (bool const modifier_first : {false, true}) {
      run(scenarios_run, {"Ctrl+C cancels until all keys are up",
                          {binding("Control+Control_L", mode)},
                          {down("Control_L"),
                           elapse(50ms),
                           down("c", None, false),
                           {Step::NoTimer},
                           tick(500ms),
                           modifier_first ? up("Control_L") : up("c"),
                           modifier_first ? up("c") : down("Control_L"),
                           modifier_first ? tick() : up("Control_L"),
                           down("Control_L"),
                           elapse(50ms),
                           up("Control_L", tap)}});
    }
  }
  for (auto held : {299ms, 300ms, 301ms, 400ms}) {
    run(scenarios_run,
        {"release before delayed timer delivery",
         {binding("Control_L")},
         {down("Control_L"), elapse(held), up("Control_L", held < 300ms ? Tap : None), tick()}});
  }
  for (auto mode : {Mode::Hold, Mode::Both}) {
    run(scenarios_run, {"hold threshold",
                        {binding("Control_L", mode)},
                        {down("Control_L"), tick(299ms), tick(1ms, HoldStart),
                         up("Control_L", HoldRelease), tick()}});
  }
  for (auto gap : {199ms, 200ms, 201ms, 5000ms}) {
    run(scenarios_run, {"first release fires regardless of later release timing",
                        {binding("Control+Shift_L")},
                        {down("Control_L"),
                         down("Shift_L"),
                         elapse(299ms),
                         up("Shift_L", Tap),
                         {Step::NoTimer},
                         tick(gap),
                         up("Control_L")}});
  }
  for (auto reset : {Step::Reset, Step::Reload}) {
    run(scenarios_run, {"context/config reset",
                        {binding("Control_L")},
                        {down("Control_L"), {reset}, {Step::NoTimer}, up("Control_L")}});
  }
  for (const auto* control : {"Control_L", "Control_R"}) {
    for (const auto* shift : {"Shift_L", "Shift_R"}) {
      run(scenarios_run,
          {"explicit modifier side",
           {binding("Control+Shift_L")},
           {down(control), down(shift),
            up(control, std::string_view(shift) == "Shift_L" ? Tap : None), up(shift)}});
    }
  }
  for (const auto& test : std::vector<Scenario>{
           {"key held before modifier",
            {binding("Control_L")},
            {down("c"), down("Control_L"), up("c"), up("Control_L")}},
           {"reset recovers when an ordinary release was lost",
            {binding("Control_L")},
            {down("c"), {Step::Reset}, down("Control_L"), up("Control_L", Tap)}},
           {"single binding does not reserve the opposite side",
            {binding("Control_L", Mode::Hold)},
            {down("Control_L", None, true), down("Control_R", None, false), up("Control_L"),
             up("Control_R")}},
           {"slow three-modifier release",
            {binding("Control+Alt+Shift_L")},
            {down("Control_L"), down("Alt_L"), down("Shift_L"), elapse(50ms), up("Shift_L", Tap),
             elapse(201ms), up("Alt_L"), up("Control_L")}},
           {"typing during release",
            {binding("Control+Shift_L")},
            {down("Control_L"), down("Shift_L"), up("Shift_L", Tap), down("c"), up("c"),
             up("Control_L")}},
           {"extra modifier suppresses subset",
            {binding("Control_L")},
            {down("Control_L"), down("Alt_L"), up("Alt_L"), up("Control_L")}},
           {"custom delay and unflagged repeat",
            {binding("Control_L", Mode::Both, Action::Command, 600ms)},
            {down("Control_L"), tick(300ms), down("Control_L"), tick(300ms, HoldStart),
             down("c", HoldCancel, false), up("c"), up("Control_L")}},
           {"tap stops toggle",
            {binding("Control_L")},
            {down("Control_L"), tick(800ms), up("Control_L", Tap)},
            true},
           {"Ctrl+C does not stop toggle",
            {binding("Control_L")},
            {down("Control_L"), down("c", None, false), up("c"), up("Control_L")},
            true},
           {"palette tap boundary and interruption",
            {binding("Shift_R", Mode::Tap, Action::Palette), binding("Control_L")},
            {down("Shift_R"), elapse(250ms), up("Shift_R", Tap), down("Shift_R"), elapse(251ms),
             up("Shift_R"), down("Shift_R"), down("Control_L"), up("Control_L"), up("Shift_R"),
             down("Control_R"), up("Control_R")}},
           {"physical identity across layout change",
            {binding("Alt_L", Mode::Tap)},
            {{Step::Press, fcitx::Key(FcitxKey_Alt_L, fcitx::KeyStates(), 64)},
             {Step::Release, fcitx::Key(FcitxKey_Meta_L, fcitx::KeyStates(), 64), {}, Tap},
             down("F8", None, false)}},
           {"flagged repeat preserves deadline",
            {binding("Control_L")},
            {down("Control_L"),
             elapse(299ms),
             {Step::Press, fcitx::Key(FcitxKey_Control_L, fcitx::KeyState::Repeat)},
             tick(1ms, HoldStart),
             up("Control_L", HoldRelease)}},
           {"locks ignored",
            {binding("Control_L")},
            {{Step::Press, locked_control}, {Step::Release, locked_control, {}, Tap}}},
           {"larger chord takes over pending tap",
            {binding("Control_L"), binding("Control+Alt+Shift_L", Mode::Both, Action::Dictation)},
            {down("Control_L"), down("Alt_L"), tick(400ms), down("Shift_L"), elapse(50ms),
             up("Control_L", Tap, 1), up("Alt_L"), up("Shift_L")}},
           {"larger chord restarts hold delay",
            {binding("Control_L"), binding("Control+Alt+Shift_L", Mode::Hold, Action::Dictation)},
            {down("Control_L"), elapse(250ms), down("Alt_L"), tick(500ms), down("Shift_L"),
             tick(299ms), tick(1ms, HoldStart, 1), up("Control_L", HoldRelease, 1), up("Alt_L"),
             up("Shift_L")}},
       }) {
    run(scenarios_run, test);
  }
  for (auto action : {Action::Dictation, Action::Command, Action::Palette}) {
    for (const auto* released : {"Control_L", "Shift_L"}) {
      const auto* retained = std::string_view(released) == "Control_L" ? "Shift_L" : "Control_L";
      run(scenarios_run, {"repeated taps with another modifier held",
                          {binding("Control+Shift_L", Mode::Tap, action)},
                          {down("Control_L"),
                           down("Shift_L"),
                           elapse(50ms),
                           up(released, Tap),
                           {Step::NoTimer},
                           tick(5000ms),
                           down(retained),
                           down(released),
                           elapse(50ms),
                           up(released, Tap),
                           up(retained)}});
      run(scenarios_run, {"intervening typing prevents rearming until all keys are up",
                          {binding("Control+Shift_L", Mode::Tap, action)},
                          {down("Control_L"), down("Shift_L"), down("c"), up("c"), up(released),
                           down(released), up(released), up(retained), down("Control_L"),
                           down("Shift_L"), up(released, Tap), up(retained)}});
    }
    run(scenarios_run,
        {"shared tap routing and recovery",
         {binding("Control+Alt+Shift_L", Mode::Tap, action)},
         {down("Shift_L"), down("Control_L"), down("Alt_L"), down("c"), up("c"), elapse(50ms),
          up("Control_L"), up("Shift_L"), up("Alt_L"), down("Shift_L"), down("Control_L"),
          down("Alt_L"), elapse(50ms), up("Control_L", Tap), up("Shift_L"), up("Alt_L")}});
    if (action != Action::Palette) {
      for (auto mode : {Mode::Hold, Mode::Both}) {
        run(scenarios_run, {"repeated holds restart the delay with a modifier held",
                            {binding("Control+Shift_L", mode, action)},
                            {down("Control_L"), down("Shift_L"), tick(300ms, HoldStart),
                             up("Shift_L", HoldRelease), tick(1000ms), down("Shift_L"), tick(299ms),
                             tick(1ms, HoldStart), up("Control_L", HoldRelease), up("Shift_L")}});
        run(scenarios_run,
            {"shared hold routing",
             {binding("Control+Alt+Shift_L", mode, action)},
             {down("Alt_L"), down("Shift_L"), down("Control_L"), tick(300ms, HoldStart),
              up("Shift_L", HoldRelease), up("Alt_L"), up("Control_L")}});
      }
    }
  }
  return scenarios_run;
}

std::size_t permutations() {
  std::size_t scenarios_run = 0;
  // Keep exhaustive two-/three-/four-modifier orders and all active-hold cases.
  const std::vector<std::pair<const char*, std::vector<std::string>>> chords = {
      {"Control_L", {"Control_L"}},
      {"Control+Shift_L", {"Control_L", "Shift_L"}},
      {"Control+Alt+Shift_L", {"Control_L", "Alt_L", "Shift_L"}},
      {"Control+Alt+Super+Shift_L", {"Control_L", "Alt_L", "Shift_L", "Super_L"}}};
  for (const auto& [name, keys] : chords) {
    auto press = keys;
    std::sort(press.begin(), press.end());
    for (bool more_press = true; more_press;
         more_press = std::next_permutation(press.begin(), press.end())) {
      auto release = keys;
      std::sort(release.begin(), release.end());
      for (bool more_release = true; more_release;
           more_release = std::next_permutation(release.begin(), release.end())) {
        for (auto mode : {Mode::Tap, Mode::Hold, Mode::Both}) {
          for (bool const held : {false, true}) {
            Scenario test{"modifier orders", {binding(name, mode)}, {}};
            for (const auto& key : press) {
              test.sequence.push_back(down(key));
              if (key != press.back()) {
                test.sequence.push_back(tick(400ms));
              }
            }
            const bool started = held && mode != Mode::Tap;
            test.sequence.push_back(tick(held ? 300ms : 50ms, started ? HoldStart : None));
            for (const auto& key : release) {
              auto event = None;
              if (started && key == release.front()) {
                event = HoldRelease;
              } else if (!started && mode != Mode::Hold && key == release.front()) {
                event = Tap;
              }
              test.sequence.push_back(up(key, event));
              test.sequence.push_back(elapse(30ms));
            }
            run(scenarios_run, test);
          }
        }
      }
    }
    for (auto mode : {Mode::Hold, Mode::Both}) {
      for (auto action : {Action::Dictation, Action::Command}) {
        for (const auto* extra : {"c", "F8", "Control_R"}) {
          Scenario test{"active hold cancellation and recovery", {binding(name, mode, action)}, {}};
          for (const auto& key : keys) {
            test.sequence.push_back(down(key));
          }
          for (auto step : {tick(300ms, HoldStart), down(keys.back()),
                            down(extra, HoldCancel, false), down(extra)}) {
            test.sequence.push_back(step);
          }
          for (const auto& key : keys) {
            test.sequence.push_back(up(key));
          }
          test.sequence.push_back(tick(500ms));
          test.sequence.push_back(up(extra));
          for (const auto& key : keys) {
            test.sequence.push_back(down(key));
          }
          // Replacing Ctrl_L with Ctrl_R completes a fresh chord when the
          // binding's explicit Shift_L is still held. Its quick release taps
          // in Both mode; ordinary typing still cannot cancel the ended hold.
          const bool rearmed_tap =
              keys.size() > 1 && mode == Mode::Both && std::string_view(extra) == "Control_R";
          for (auto step : {tick(300ms, HoldStart), up(keys.front(), HoldRelease), down(extra),
                            up(extra, rearmed_tap ? Tap : None)}) {
            test.sequence.push_back(step);
          }
          for (std::size_t i = 1; i < keys.size(); ++i) {
            test.sequence.push_back(up(keys[i]));
          }
          run(scenarios_run, test);
        }
      }
    }
  }
  return scenarios_run;
}
} // namespace

int main(int argc, char** argv) {
  std::size_t scenarios_run = 0;
  const std::string_view group = argc == 2 ? argv[1] : "";
  if (group.empty() || group == "scenarios") {
    scenarios_run += examples();
  }
  if (group.empty() || group == "permutations") {
    scenarios_run += permutations();
  }
  if (scenarios_run == 0) {
    return 1;
  }
  std::cout << "PASS " << scenarios_run << " modifier scenarios\n";
}
