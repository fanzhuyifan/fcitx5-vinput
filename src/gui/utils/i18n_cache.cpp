#include "gui/utils/i18n_cache.h"

#include <QMetaObject>
#include <QtConcurrent/QtConcurrent>

namespace vinput::gui {

I18nCache& I18nCache::Get() {
    static I18nCache instance;
    return instance;
}

void I18nCache::ReloadFromDisk() {
    const uint64_t generation = ++generation_;
    QtConcurrent::run([this, generation]() {
        const std::string locale = vinput::registry::DetectPreferredLocale();
        std::string error;
        auto map =
            vinput::registry::LoadMergedCachedI18nMap(locale, &error);

        QMetaObject::invokeMethod(
            this,
            [this, generation, map = std::move(map), error = std::move(error)]() mutable {
                if (generation != generation_.load()) {
                    return;
                }
                if (!error.empty()) {
                    emit reloadFailed(QString::fromStdString(error));
                    return;
                }
                {
                    QMutexLocker locker(&mutex_);
                    map_ = std::move(map);
                }
                emit mapUpdated();
            },
            Qt::QueuedConnection);
    });
}

vinput::registry::I18nMap I18nCache::GetMap() const {
    QMutexLocker locker(&mutex_);
    return map_;
}

} // namespace vinput::gui
