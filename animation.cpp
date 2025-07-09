#include "Animation.h"
#include <QString>
#include <QPixmap>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
Animation::Animation()
    : currentFrame(0), frameCounter(0), frameDelay(6) // 调整为你想要的帧率
{}

void Animation::load(const QString& name, const QString& imagePath, int frameWidth, int frameCount) {
    AnimData anim;
    anim.sheet = QPixmap(imagePath);
    anim.frameWidth = frameWidth;
    anim.frameCount = frameCount;
    animations[name] = anim;
}

void Animation::play(const QString& name, bool once) { // 根据name选择绘制的动画组
    if (playingOnce && oncePlayed == false){
        return; // 如果正在做一定要播完一次的动画，那么强行阻止改变动画
    }
    if (name != currentAnim) {
        currentAnim = name;
        currentFrame = 0;
        frameCounter = 0;
        playingOnce = once; // 接受新参数
        oncePlayed = false;
    }
}

void Animation::update(bool loop) { // 根据play的动画组选择绘制哪一帧，循环或停止到最后一帧
    frameCounter++;
    if (frameCounter >= frameDelay) {
        frameCounter = 0;
        if (loop && playingOnce == false){
            currentFrame = (currentFrame + 1) % animations[currentAnim].frameCount;
        }
        else if (!loop){
            if (animations[currentAnim].frameCount > currentFrame + 1){
                currentFrame ++;
            }
        }
        else if (playingOnce){
            if (oncePlayed) return;
            if (animations[currentAnim].frameCount > currentFrame + 1){
                currentFrame ++;
            }
            else{
                oncePlayed = true;
                playingOnce = false;
            }
        }
    }
}

void Animation::draw(QPainter& painter, float x, float y, bool flipped) { // 负责绘制，绘制哪一帧由update决定
    if (!animations.contains(currentAnim)) return;

    const AnimData& anim = animations[currentAnim];
    int frameWidth = anim.frameWidth;
// idle/jump/run: 40, 43, 30, 37 (x, y, w, h)，frame是30*37的
    QPixmap frame = anim.sheet.copy(currentFrame * frameWidth + 40, 43, 30, 37);

    // 设置缩放比例
    const float scale = 1.5f;
    float scaledWidth = static_cast<int>(frame.width() * scale);
    float scaledHeight = static_cast<int>(frame.height() * scale);

    targetRect = QRect(
        x - (scaledWidth / 2.0),  // 左移一半宽度，此时x和y不是碰撞箱左上角坐标，而是竖直中分线
        y, scaledWidth, scaledHeight);

    // 关闭像素平滑处理，保留清晰像素风格
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    painter.setPen(Qt::blue);
    painter.setBrush(Qt::NoBrush);
    if (flipped) {
        frame = frame.transformed(QTransform().scale(-1, 1));
        // 注意翻转后也要按缩放后宽度修正 x 坐标
        painter.drawPixmap(targetRect, frame);
        painter.drawRect(targetRect);
    } else {
        painter.drawPixmap(targetRect, frame);
        painter.drawRect(targetRect);
    }
}

QRect Animation::getTargetRect(){
    return targetRect;
}
