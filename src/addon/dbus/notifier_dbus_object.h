#pragma once

#include <fcitx-utils/dbus/objectvtable.h>
#include <functional>
#include <string>

#include "common/dbus/dbus_interface.h"
#include "common/dbus/error_info.h"

class VinputNotifierDBusObject : public fcitx::dbus::ObjectVTable<VinputNotifierDBusObject> {
public:
  explicit VinputNotifierDBusObject(
      std::function<void(const vinput::dbus::ErrorInfo&)> notification_callback)
      : notification_callback_(std::move(notification_callback)) {}

  void Notify(const std::string& code, const std::string& subject, const std::string& detail,
              const std::string& raw_message) {
    if (notification_callback_) {
      notification_callback_(vinput::dbus::MakeErrorInfo(code, subject, detail, raw_message));
    }
  }

private:
  FCITX_OBJECT_VTABLE_METHOD(Notify, vinput::dbus::kMethodNotify, vinput::dbus::kErrorInfoSignature,
                             "");

  std::function<void(const vinput::dbus::ErrorInfo&)> notification_callback_;
};
