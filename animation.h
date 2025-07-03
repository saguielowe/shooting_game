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
    void play(const QString& name);  // 切换动画
    void update();                   // 更新帧
    void draw(QPainter& painter, float x, float y, bool flipped = false);
    QRect getTargetRect();

private:
    struct AnimData {
        QPixmap sheet;
        int frameWidth;
        int frameCount;
    };
    QMap<QString, AnimData> animations;
    QString currentAnim;
    QRect targetRect;
    int currentFrame;
    int frameCounter; // 控制帧速率
    int frameDelay;

};

#endif // ANIMATION_H
