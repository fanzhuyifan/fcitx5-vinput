#include "cli/control/daemon_actions.h"

#include <cstdio>

#include "common/i18n.h"
#include "common/utils/string_utils.h"

#include "cli/runtime/dbus_client.h"
#include "cli/runtime/systemd_client.h"
int RunDaemonControlStatus(Formatter& fmt, const CliContext& ctx) {
  vinput::cli::DbusClient dbus;

  std::string dbus_error;
  bool running = dbus.IsDaemonRunning(&dbus_error);

  if (!running) {
    if (ctx.json_output) {
      fmt.PrintJson({{"running", false}});
    } else {
      fmt.PrintError(_("Daemon is not running."));
    }
    return 1;
  }

  std::string status;
  std::string get_error;
  if (!dbus.GetDaemonStatus(&status, &get_error)) {
    if (ctx.json_output) {
      fmt.PrintJson({{"running", true}, {"status", nullptr}, {"error", get_error}});
    } else {
      fmt.PrintError(get_error);
    }
    return 1;
  }

  if (ctx.json_output) {
    fmt.PrintJson({{"running", true}, {"status", status}});
  } else {
    char buf[256];
    std::snprintf(buf, sizeof(buf), _("Daemon status: %s"), status.c_str());
    fmt.PrintInfo(buf);
  }

  return 0;
}

int RunDaemonControlStart(Formatter& fmt, const CliContext& ctx) {
  (void)ctx;
  const auto result = vinput::cli::SystemctlStartWithDiagnostics();
  if (result.ok()) {
    fmt.PrintSuccess(_("Daemon started."));
    return 0;
  }
  fmt.PrintError(result.failure_message);
  vinput::cli::NotifyDaemonNotification(result.notification);
  return 1;
}

int RunDaemonControlStop(Formatter& fmt, const CliContext& ctx) {
  (void)ctx;
  int r = vinput::cli::SystemctlStop();
  if (r == 0) {
    fmt.PrintSuccess(_("Daemon stopped."));
    return 0;
  }
  fmt.PrintError(vinput::str::FmtStr(_("systemctl stop failed (exit code: %d)"), r));
  return 1;
}

int RunDaemonControlRestart(Formatter& fmt, const CliContext& ctx) {
  (void)ctx;
  const auto result = vinput::cli::SystemctlRestartWithDiagnostics();
  if (result.ok()) {
    fmt.PrintSuccess(_("Daemon restarted."));
    return 0;
  }
  fmt.PrintError(result.failure_message);
  vinput::cli::NotifyDaemonNotification(result.notification);
  return 1;
}

int RunDaemonControlLogs(bool follow, int lines, Formatter& fmt, const CliContext& ctx) {
  (void)fmt;
  (void)ctx;
  return vinput::cli::JournalctlLogs(follow, lines);
}
