#ifndef ANIMATION_H
#define ANIMATION_H
#include <QString>
#include <QPixmap>
#include <QPainter>
#include <QMap>
class Animation
{
public:
    Animation();
    void load(const QString& name, const QString& imagePath, int frameWidth, int frameCount);
    void play(const QString& name, bool once=false);  // 切换动画
    void update(bool loop=true);                   // 更新帧，loop=false表示不需要循环播放
    void draw(QPainter& painter, float x, float y, bool flipped, bool armed);
    void loadWeapon(QString weapon);
    void paintGun(QPainter &painter, float x, float y);
    QRect getTargetRect();

private:
    struct AnimData {
        QPixmap sheet;
        int frameWidth;
        int frameCount;
    };
    QPixmap weaponImage;
    QMap<QString, AnimData> animations;
    void paintWeapon(QPainter &painter, QRect targetRect);
    QString currentAnim;
    QRect targetRect;
    qreal angle = 0;
    int currentFrame;
    int frameCounter; // 控制帧速率
    int frameDelay;
    bool playingOnce=false, oncePlayed=false;

};

#endif // ANIMATION_H
