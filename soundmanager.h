#pragma once
#include <QObject>
#include <QSoundEffect>
#include <QMap>
#include <QString>
#include <QDebug>

class SoundManager : public QObject {
    Q_OBJECT
public:
    static SoundManager& instance() {
        static SoundManager mgr;
        return mgr;
    }

    void play(const QString &name, const float vol) {
        if (sounds.contains(name)) {
            auto *effect = sounds[name];
            // 避免过快重复播放
            if (effect->isPlaying()) {
                // 如果已经在放，就不打断
                return;
            }
            effect->setVolume(vol);
            effect->play();
        }
    }

    void preload(const QString &name) {
        QString fullPath = QString("qrc:/sounds/assets/sounds/%1.wav").arg(name);
        QSoundEffect *effect = new QSoundEffect(this);
        effect->setSource(QUrl(fullPath));
        sounds[name] = effect;
    }


private:
    QMap<QString, QSoundEffect*> sounds;
    SoundManager() {}
    ~SoundManager() {}
};
