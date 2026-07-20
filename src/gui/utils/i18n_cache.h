#pragma once

#include <atomic>
#include <cstdint>

#include <QMutex>
#include <QObject>
#include <QString>

#include "common/registry/registry_i18n.h"

namespace vinput::gui {

class I18nCache : public QObject {
    Q_OBJECT
public:
    static I18nCache& Get();

    // Starts an async disk reload and returns the generation for that request.
    // Only the latest generation emits mapUpdated/reloadFailed.
    quint64 ReloadFromDisk();
    vinput::registry::I18nMap GetMap() const;

signals:
    void mapUpdated(quint64 generation);
    void reloadFailed(const QString& error, quint64 generation);

private:
    I18nCache() = default;
    ~I18nCache() override = default;

    mutable QMutex mutex_;
    vinput::registry::I18nMap map_;
    std::atomic<uint64_t> generation_{0};
};

} // namespace vinput::gui
