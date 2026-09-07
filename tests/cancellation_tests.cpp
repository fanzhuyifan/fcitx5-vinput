#include <cstdint>
#include <cstdlib>
#include <fcitx-utils/dbus/bus.h>
#include <fcitx-utils/dbus/message.h>
#include <fcitx-utils/event.h>
#include <fcitx-utils/key.h>
#include <fcitx-utils/keysym.h>
#include <fcitx/event.h>
#include <fcitx/inputcontext.h>
#include <fcitx/inputcontextmanager.h>
#include <filesystem>
#include <functional>
#include <iostream>
#include <memory>
#include <span>
#include <stdlib.h> // NOLINT(modernize-deprecated-headers): POSIX mkdtemp and setenv.
#include <string>
#include <time.h> // NOLINT(modernize-deprecated-headers): POSIX CLOCK_MONOTONIC.
#include <vector>

#include "common/config/vinput_config.h"
#include "common/dbus/dbus_interface.h"

#include "daemon/asr/runtime/recognition_contract.h"
#include "daemon/audio/audio_capture.h"
#include "daemon/runtime/daemon_runtime_controller.h"
#include "daemon/runtime/dbus_service.h"

#include "core/vinput.h"
#include "input/modifier_gesture.h"

namespace {
using Runtime = vinput::daemon::runtime::DaemonRuntimeController;
using Phase = vinput::dbus::Status;
using namespace vinput::dbus;

void expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    std::exit(1);
  }
}

struct RecognitionSession : vinput::daemon::asr::RecognitionSession {
  int cancelled = 0;
  int finished = 0;
  int pushed = 0;
  bool PushAudio(std::span<const int16_t>, std::string*) override {
    ++pushed;
    return true;
  }
  bool Finish(std::string*) override {
    ++finished;
    return true;
  }
  void Cancel() override { ++cancelled; }
  std::vector<vinput::daemon::asr::RecognitionEvent> PollEvents() override { return {}; }
};

class InputContext : public fcitx::InputContext {
public:
  explicit InputContext(fcitx::Instance& instance)
      : fcitx::InputContext(instance.inputContextManager()) {
    created();
  }
  InputContext(const InputContext&) = delete;
  InputContext& operator=(const InputContext&) = delete;
  InputContext(InputContext&&) = delete;
  InputContext& operator=(InputContext&&) = delete;
  ~InputContext() override { destroy(); }
  [[nodiscard]] const char* frontend() const override { return "cancellation-test"; }
  std::string committed;

protected:
  void commitStringImpl(const std::string& text) override { committed += text; }
  void deleteSurroundingTextImpl(int, unsigned int) override {}
  void forwardKeyImpl(const fcitx::ForwardKeyEvent&) override {}
  void updatePreeditImpl() override {}
};

// Isolate all configuration and context-history writes from the user's session.
struct Environment {
  std::filesystem::path directory;
  Environment() {
    char pattern[] = "/tmp/vinput-cancellation-XXXXXX";
    auto* path = mkdtemp(pattern);
    expect(path != nullptr, "create temporary environment");
    directory = path;
    for (const auto* name :
         {"XDG_CONFIG_HOME", "XDG_DATA_HOME", "XDG_CACHE_HOME", "XDG_RUNTIME_DIR"}) {
      const auto value = directory / name;
      std::filesystem::create_directory(value);
      setenv(name, value.c_str(), 1);
    }
  }
  Environment(const Environment&) = delete;
  Environment& operator=(const Environment&) = delete;
  Environment(Environment&&) = delete;
  Environment& operator=(Environment&&) = delete;
  ~Environment() { std::filesystem::remove_all(directory); }
};
} // namespace

// Fixture access only seeds otherwise hardware-dependent recording state. Key
// handling, D-Bus calls/replies, cancellation, and signal dispatch are production code.
struct VinputCancellationTest {
  fcitx::Instance instance;
  fcitx::dbus::Bus bus{fcitx::dbus::BusType::Session};
  DbusService server;
  AudioCapture capture;
  Runtime runtime{&capture, &server, nullptr, nullptr};
  VinputEngine engine{&instance};
  InputContext ic{instance};
  std::shared_ptr<RecognitionSession> recognition = std::make_shared<RecognitionSession>();
  int cancellations = 0;
  int stops = 0;
  std::unique_ptr<fcitx::EventSourceIO> server_io;

  VinputCancellationTest(int argc, char** argv) : instance(argc, argv) {
    std::string error;
    expect(server.Start(&error), error.c_str());
    expect(bus.isOpen(), "connect to private D-Bus");
    bus.attachEventLoop(&instance.eventLoop());
    engine.bus_ = &bus;
    engine.setupDBusWatcher();
    server.SetCancelOperationHandler([this](bool raw) {
      ++cancellations;
      return runtime.CancelOperation(raw);
    });
    server.SetStatusHandler([this] { return runtime.GetStatus(); });
    server.SetStopHandler([this](const std::string& scene) {
      ++stops;
      return runtime.StopRecording(scene);
    });
    server_io =
        instance.eventLoop().addIOEvent(server.GetFd(), fcitx::IOEventFlag::In,
                                        [this](fcitx::EventSourceIO*, int, fcitx::IOEventFlags) {
                                          while (server.ProcessOnce()) {
                                          }
                                          return true;
                                        });
  }

  void armRecording(bool command = false) {
    runtime.phase_ = Phase::Recording;
    runtime.active_session_ = recognition;
    runtime.accepting_chunks_ = true;
    runtime.current_recording_pcm_ = {1, 2, 3};
    runtime.pending_chunk_pcm_ = {4, 5};
    runtime.current_sample_count_ = 5;
    runtime.latest_final_text_ = "uncommitted transcript";
    runtime.current_is_command_ = command;
    runtime.current_selected_text_ = command ? "original selection" : "";
  }

  void armFrontend(const char* shortcut, bool held = true, bool released = false) {
    engine.enterPendingStartState(&ic, fcitx::Key(shortcut), false);
    if (!engine.session_) {
      std::cerr << "FAIL: frontend session was not created\n";
      std::exit(1);
    }
    engine.session_->stop_on_release = held;
    engine.session_->trigger_released = released;
    engine.session_->phase = VinputEngine::Session::Phase::Recording;
  }

  VinputEngine::Session& requireSession() {
    if (!engine.session_) {
      std::cerr << "FAIL: expected an active frontend session\n";
      std::exit(1);
    }
    return *engine.session_;
  }

  void interrupt() {
    fcitx::KeyEvent event(&ic, fcitx::Key(FcitxKey_c, fcitx::KeyState::Ctrl));
    engine.handleKeyEvent(event);
    expect(!event.accepted() && !event.filtered(), "interrupting Ctrl+C reaches the application");
  }

  void runUntil(std::function<bool()> done) {
    bool completed = false;
    auto check = instance.eventLoop().addPostEvent([&](fcitx::EventSource*) {
      if (done()) {
        completed = true;
        instance.eventLoop().exit();
      }
      return true;
    });
    // POSIX exposes this constant through <time.h>, not the private <bits/time.h>.
    // NOLINTNEXTLINE(misc-include-cleaner)
    constexpr auto clock_id = CLOCK_MONOTONIC;
    auto timeout = instance.eventLoop().addTimeEvent(clock_id, fcitx::now(clock_id) + 5'000'000, 0,
                                                     [&](fcitx::EventSourceTime*, uint64_t) {
                                                       instance.eventLoop().exit();
                                                       return false;
                                                     });
    instance.eventLoop().exec();
    expect(completed, "asynchronous operation completed before timeout");
  }

  void expectDiscarded() {
    expect(runtime.GetStatus() == kStatusIdle, "daemon returns to idle");
    expect(!runtime.active_session_ && !runtime.current_order_,
           "no session or inference order remains");
    expect(!runtime.accepting_chunks_ && runtime.current_recording_pcm_.empty() &&
               runtime.pending_chunk_pcm_.empty() && runtime.latest_final_text_.empty() &&
               runtime.current_selected_text_.empty() && runtime.current_sample_count_ == 0,
           "recording and command buffers are discarded");
    expect(recognition->cancelled == 1 && recognition->finished == 0 && recognition->pushed == 0,
           "ASR session is cancelled without submitting audio or finishing inference");
    expect(ic.committed.empty() && !engine.session_ && !engine.recording_cancel_requested_,
           "frontend clears without committing stale text");
    expect(cancellations == 1 && stops == 0, "one discard request and no normal Stop request");
  }

  void recordingDiscard(bool delayed, bool reject, bool command = false) {
    armFrontend("Control+Alt+Shift_L");
    int signals = 0;
    auto observer = bus.addFilter([&](fcitx::dbus::Message& msg) {
      if (msg.type() == fcitx::dbus::MessageType::Signal && msg.interface() == kInterface) {
        ++signals;
        expect(engine.recording_cancel_requested_,
               "queued signals precede cancellation acknowledgement");
      }
      return false;
    });
    const auto start = [&] {
      if (reject) {
        return DbusService::MethodResult::Failure("busy");
      }
      armRecording(command);
      // Updates queued before cancellation must be delivered before its reply.
      server.EmitStatusChanged(kStatusRecording);
      server.EmitRecognitionPartial("stale partial");
      server.EmitRecognitionResult(R"({"commit_text":"stale result"})");
      return DbusService::MethodResult::Success();
    };
    server.SetStartHandler(start);
    server.SetStartCommandHandler([&](const std::string&) { return start(); });
    if (delayed) {
      requireSession().phase = VinputEngine::Session::Phase::PendingStart;
      expect(command ? engine.callStartCommandRecording("selection") : engine.callStartRecording(),
             "start request was queued");
    } else {
      start();
    }
    interrupt();
    expect(engine.recording_cancel_requested_, "interruption records discard intent immediately");
    expect(!delayed || !engine.pending_cancel_call_slot_, "cancel waits for the Start reply");
    // Losing the original context/session must not lose a pending discard.
    if (delayed) {
      engine.finishFrontendSession();
    }
    runUntil([&] {
      return !engine.pending_start_call_slot_ && !engine.pending_cancel_call_slot_ &&
             !engine.recording_cancel_requested_;
    });
    if (reject) {
      expect(cancellations == 0 && recognition->cancelled == 0,
             "a rejected Start must not cancel another client's recording");
      expect(!engine.session_ && ic.committed.empty(), "rejected start clears frontend");
    } else {
      expect(signals == 4, "all queued recording updates and final idle reached the addon");
      expectDiscarded();
    }
  }

  void shortcutScope() {
    const struct {
      const char* key;
      bool held;
      bool released;
    } cases[] = {{"F8", true, false},
                 {"Control+space", true, false},
                 {"Control_L", false, true},
                 {"Control+Shift_L", true, true}};
    for (const auto& test : cases) {
      armFrontend(test.key, test.held, test.released);
      interrupt();
      expect(!engine.recording_cancel_requested_ && !engine.pending_cancel_call_slot_,
             "ordinary holds, tap-toggle, and typing after hold release keep their behavior");
      engine.finishFrontendSession();
    }
    armRecording();
    expect(!runtime.CancelOperation(true).ok && runtime.GetStatus() == kStatusRecording,
           "Use raw text remains unavailable during recording");
  }

  static void holdRepeat(int argc, char** argv) {
    for (const auto* shortcut : {"F8", "Control+space"}) {
      VinputCancellationTest test(argc, argv);
      auto& engine = test.engine;
      engine.trigger_keys_ = {fcitx::Key("F8")};
      engine.command_keys_ = {fcitx::Key("Control+space")};
      engine.trigger_mode_ = TriggerMode::Hold;
      engine.hold_activation_delay_ = std::chrono::milliseconds(1500);
      const auto send = [&](bool release = false, bool repeat = false) {
        const fcitx::Key key(shortcut);
        fcitx::KeyEvent event(
            &test.ic,
            fcitx::Key(key.sym(), key.states() | (repeat ? fcitx::KeyState::Repeat
                                                         : fcitx::KeyState::NoState)),
            release);
        engine.handleKeyEvent(event);
        expect(event.accepted() && event.filtered(), "voice shortcut events stay consumed");
      };
      send();
      expect(engine.pending_start_event_ && engine.pending_start_event_->isEnabled(),
             "initial press arms the hold timer");
      const auto deadline = engine.pending_start_event_->time();
      // Model repeat after debounce without sleeping or moving the hold deadline.
      engine.last_trigger_time_ -= std::chrono::milliseconds(100);
      const auto before_repeat = engine.last_trigger_time_;
      send(false, true);
      expect(engine.pending_start_event_->isEnabled() &&
                 engine.pending_start_event_->time() == deadline &&
                 engine.last_trigger_time_ == before_repeat,
             (std::string(shortcut) + ": repeat preserves timer and debounce timestamp").c_str());
      send(true);
      expect(!engine.pending_start_event_->isEnabled() && (engine.pending_start_ic_ == nullptr) &&
                 !engine.session_,
             "early release cancels activation");
      send();
      expect(engine.pending_start_event_->isEnabled() && engine.pending_start_ic_ == &test.ic,
             "a new real press rearms activation");
      if (std::string(shortcut) != "F8") {
        continue;
      }
      test.server.SetStartHandler([&] {
        test.armRecording();
        test.server.EmitStatusChanged(kStatusRecording);
        test.server.FlushEmitQueue();
        return DbusService::MethodResult::Success();
      });
      engine.pending_start_event_->setTime(fcitx::now(CLOCK_MONOTONIC));
      test.runUntil([&] {
        if (engine.pending_start_call_slot_ || !engine.session_ ||
            engine.session_->phase != VinputEngine::Session::Phase::Recording) {
          return false;
        }
        engine.last_trigger_time_ -= std::chrono::milliseconds(100);
        const auto before_repeat = engine.last_trigger_time_;
        send(false, true);
        expect(engine.session_->phase == VinputEngine::Session::Phase::Recording &&
                   !engine.session_->trigger_released && !engine.pending_stop_call_slot_ &&
                   (!engine.pending_stop_event_ || !engine.pending_stop_event_->isEnabled()) &&
                   engine.last_trigger_time_ == before_repeat,
               "repeat after activation leaves recording and debounce unchanged");
        return true;
      });
    }
  }

  static void modifierRouting(int argc, char** argv) {
    using G = ModifierGesture;
    enum class Routing : std::uint8_t { Pass, Consume };
    using enum Routing;
    struct Step {
      enum class Kind : std::uint8_t { Press, Release, Hold, Reset, Recording, Finished };
      using enum Kind;
      Kind kind;
      const char* key = "";
      Routing routing = Consume;
    };
    const auto down = [](const char* key, Routing route = Consume) {
      return Step{Step::Press, key, route};
    };
    const auto up = [](const char* key, Routing route = Consume) {
      return Step{Step::Release, key, route};
    };
    const Step hold{Step::Hold};
    const Step reset{Step::Reset};
    const Step recording{Step::Recording};
    const Step finished{Step::Finished};
    const struct {
      const char* name;
      std::vector<Step> sequence;
      bool recorded;
      G::Mode mode = G::Mode::Both;
      bool cancelled = false;
      const char* shortcut = "Alt+Alt_L";
      const char* extra_shortcut = nullptr;
    } cases[] = {
        {"Alt tap toggle", {down("Alt_L"), up("Alt_L"), down("Alt_L"), up("Alt_L")}, true},
        {"Alt hold", {down("Alt_L"), hold, up("Alt_L")}, true, G::Mode::Hold},
        {"Alt both hold", {down("Alt_L"), hold, up("Alt_L")}, true},
        {"hold consumes repeats", {down("Alt_L"), hold, down("Alt+Alt_L"), up("Alt_L")}, true},
        {"chord presses and releases reach the application",
         {down("Control_L", Pass), down("Control+Alt_L", Pass), hold,
          up("Control+Alt+Control_L", Pass), up("Alt+Alt_L", Pass)},
         true,
         G::Mode::Hold,
         false,
         "Control+Alt_L"},
        {"chord with reverse press and release order",
         {down("Alt_L", Pass), down("Alt+Control_L", Pass), hold, up("Control+Alt+Alt_L", Pass),
          up("Control+Control_L", Pass)},
         true,
         G::Mode::Hold,
         false,
         "Control+Alt_L"},
        {"remaining chord release passes after recording finishes",
         {down("Control_L", Pass), down("Control+Alt_L", Pass), hold, recording,
          up("Control+Alt+Control_L", Pass), finished, up("Alt+Alt_L", Pass)},
         true,
         G::Mode::Hold,
         false,
         "Control+Alt_L"},
        {"unobserved modifier release passes through",
         {down("Control+Alt_L", Pass), hold, up("Control+Alt+Control_L", Pass),
          up("Alt+Alt_L", Pass)},
         true,
         G::Mode::Hold,
         false,
         "Control+Alt_L"},
        {"prefix alone does not activate",
         {down("Control_L", Pass), up("Control_L", Pass)},
         false,
         G::Mode::Both,
         false,
         "Control+Alt_L"},
        {"single binding retains ownership when a chord completes",
         {down("Control_L"), down("Control+Alt_L", Pass), hold, recording,
          up("Control+Alt+Alt_L", Pass), finished, up("Control+Control_L")},
         true,
         G::Mode::Hold,
         false,
         "Control+Alt_L",
         "Control_L"},
        {"Alt+C preserves modifiers",
         {down("Alt_L"), down("Alt+c", Pass), up("Alt+c", Pass), up("Alt+Alt_L")},
         false},
        {"Ctrl+C preserves modifiers",
         {down("Control_L"), down("Control+c", Pass), up("Control+c", Pass), up("Control_L")},
         false,
         G::Mode::Both,
         false,
         "Control_L"},
        {"early hold stays consumed", {down("Alt_L"), up("Alt_L")}, false, G::Mode::Hold},
        {"reset preserves pending release", {down("Alt_L"), reset, up("Alt_L")}, false},
        {"reset preserves active release",
         {down("Alt_L"), hold, recording, reset, up("Alt_L")},
         true},
        {"active interruption consumes only modifiers",
         {down("Alt_L"), hold, recording, down("Alt+c", Pass), up("Alt+c", Pass), up("Alt_L")},
         true,
         G::Mode::Both,
         true,
         "Alt+Alt_L"},
    };
    for (const auto& row : cases) {
      VinputCancellationTest test(argc, argv);
      auto& engine = test.engine;
      auto& server = test.server;
      server.SetStartHandler([&] {
        test.armRecording();
        server.EmitStatusChanged(kStatusRecording);
        server.FlushEmitQueue();
        return DbusService::MethodResult::Success();
      });
      server.SetStopHandler([&](const std::string&) {
        expect(engine.session_->trigger_released && engine.session_->stop_on_release,
               "release or second tap requests recording completion");
        test.runtime.phase_ = Phase::Idle;
        server.EmitRecognitionResult(R"({"commit_text":"dictated text"})");
        server.EmitStatusChanged(kStatusIdle);
        server.FlushEmitQueue();
        return DbusService::MethodResult::Success();
      });
      std::vector<G::Binding> bindings = {{fcitx::Key(row.shortcut), G::Action::Dictation, row.mode,
                                           std::chrono::milliseconds(300)}};
      if (row.extra_shortcut != nullptr) {
        bindings.push_back({fcitx::Key(row.extra_shortcut), G::Action::Dictation, row.mode,
                            std::chrono::milliseconds(300)});
      }
      engine.modifier_gesture_.setBindings(bindings);
      size_t next = 0;
      const auto advance = [&] {
        while (next < row.sequence.size()) {
          const auto& step = row.sequence[next];
          if (step.kind == Step::Recording &&
              (engine.pending_start_call_slot_ || !engine.session_ ||
               engine.session_->phase != VinputEngine::Session::Phase::Recording)) {
            return false;
          }
          if (step.kind == Step::Finished && (engine.pending_stop_call_slot_ || engine.session_ ||
                                              test.ic.committed != "dictated text")) {
            return false;
          }
          ++next;
          if (step.kind == Step::Reset) {
            engine.resetPendingGestures();
          } else if (step.kind == Step::Hold) {
            const auto deadline = engine.modifier_gesture_.deadline();
            expect(deadline.has_value(), "complete chord arms the hold timer");
            engine.dispatchModifierGesture(engine.modifier_gesture_.timeout(*deadline), &test.ic);
            engine.updateModifierTimer();
          } else if (step.kind == Step::Press || step.kind == Step::Release) {
            const fcitx::Key key(step.key);
            fcitx::KeyEvent event(&test.ic, key, step.kind == Step::Release);
            engine.handleKeyEvent(event);
            expect(event.accepted() == (step.routing == Consume) &&
                       event.filtered() == (step.routing == Consume),
                   (std::string(row.name) + ": " + step.key + " routing").c_str());
            expect(event.rawKey() == key, "key identity and modifier flags stay unchanged");
          }
        }
        return true;
      };
      advance();
      expect(engine.session_.has_value() == row.recorded,
             (std::string(row.name) + ": recording starts only for completed gestures").c_str());
      if (row.recorded) {
        test.runUntil([&] {
          const bool sequence_complete = advance();
          // Deliver the scheduled stop before synchronous status polling, which
          // cannot run against a fake server sharing this event loop.
          if (engine.pending_stop_event_ && engine.pending_stop_event_->isEnabled()) {
            engine.pending_stop_event_->setTime(fcitx::now(CLOCK_MONOTONIC));
          }
          return sequence_complete && !engine.pending_start_call_slot_ &&
                 !engine.pending_stop_call_slot_ && !engine.pending_cancel_call_slot_ &&
                 !engine.session_;
        });
      }
      expect(test.ic.committed == (row.recorded && !row.cancelled ? "dictated text" : ""),
             (std::string(row.name) + ": committed text").c_str());
      expect(test.cancellations == (row.cancelled ? 1 : 0), "only interrupted holds are discarded");
    }
  }

  void failedCancellation() {
    armFrontend("Control_L");
    armRecording();
    server.SetCancelOperationHandler([&](bool) {
      ++cancellations;
      server.EmitRecognitionResult(R"({"commit_text":"stale result"})");
      return DbusService::MethodResult::Failure("cancel failed");
    });
    interrupt();
    runUntil([&] { return !engine.pending_cancel_call_slot_; });
    expect(engine.recording_cancel_requested_ && ic.committed.empty(),
           "a failed cancellation retains discard intent and suppresses results");
    engine.finishStopRecording();
    engine.activateVoiceTrigger(&ic, fcitx::Key("F8"), false);
    expect(!engine.pending_stop_call_slot_ && !engine.pending_start_call_slot_,
           "failed cancellation cannot turn into a normal stop or new recording");
    runtime.CancelOperation(false);
    engine.applyDaemonStatusLocally(kStatusIdle);
    expect(!engine.recording_cancel_requested_ && !engine.session_,
           "observing idle recovers the frontend after a failed acknowledgement");
  }

  void postprocessing() {
    using State = Runtime::PostprocessingState;
    const struct {
      bool command;
      bool raw;
      bool accepted;
      State action;
    } cases[] = {{false, false, true, State::Discard},
                 {false, true, true, State::CommitRaw},
                 {true, false, true, State::Discard},
                 {true, true, false, State::Command}};
    for (const auto& test : cases) {
      runtime.postprocessing_cancel_requested_ = false;
      runtime.EnterPostprocessing(test.command);
      const auto result = runtime.CancelOperation(test.raw);
      expect(result.ok == test.accepted &&
                 runtime.postprocessing_cancel_requested_ == test.accepted,
             "unified cancellation preserves postprocessing acceptance");
      expect(runtime.TakePostprocessingState() == test.action,
             "worker receives the requested cancellation action");
      expect(runtime.GetStatus() == kStatusPostprocessing,
             "postprocessing completion remains owned by the worker");
    }
    armFrontend("F8");
    requireSession().phase = VinputEngine::Session::Phase::Postprocessing;
    engine.last_known_daemon_status_ = kStatusPostprocessing;
    runtime.EnterPostprocessing(false);
    engine.callCancelOperation(true);
    runUntil([&] { return !engine.pending_cancel_call_slot_; });
    expect(cancellations == 1 && runtime.postprocessing_cancel_requested_,
           "addon postprocessing uses the shared D-Bus method");
    expect(engine.session_.has_value(), "raw-text cancellation waits for the worker's result");
  }
};

int main(int argc, char** argv) {
  expect(argc == 2, "one test scenario is required");
  const std::string scenario = argv[1];
  Environment const environment;
  char program[] = "vinput-cancellation-tests";
  char disable[] = "--disable=all";
  char* instance_args[] = {program, disable, nullptr};
  if (scenario == "modifier-routing" || scenario == "hold-repeat") {
    if (scenario == "modifier-routing") {
      VinputCancellationTest::modifierRouting(2, instance_args);
    } else {
      VinputCancellationTest::holdRepeat(2, instance_args);
    }
    std::cout << "PASS " << scenario << '\n';
    return 0;
  }
  VinputCancellationTest test(2, instance_args);
  if (scenario == "recording-discard") {
    test.recordingDiscard(false, false);
  } else if (scenario == "delayed-start" || scenario == "delayed-command-start" ||
             scenario == "rejected-start") {
    test.recordingDiscard(true, scenario == "rejected-start", scenario == "delayed-command-start");
  } else if (scenario == "shortcut-scope") {
    test.shortcutScope();
  } else if (scenario == "failed-cancellation") {
    test.failedCancellation();
  } else if (scenario == "postprocessing") {
    test.postprocessing();
  } else {
    expect(false, "unknown scenario");
  }
  std::cout << "PASS " << scenario << '\n';
}
