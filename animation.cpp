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
    anim.sheet = QPixmap(imagePath);/*
    if (anim.sheet.isNull()) {
        qDebug() << "Failed to load image:" << imagePath;
    } else {
        qDebug() << "Loaded" << imagePath << "size:" << anim.sheet.size();
    }*/
    anim.frameWidth = frameWidth;
    anim.frameCount = frameCount;
    animations[name] = anim;
}

void Animation::play(const QString& name) {
    if (name != currentAnim) {
        currentAnim = name;
        currentFrame = 0;
        frameCounter = 0;
    }
}

void Animation::update() {
    // qDebug()<<"updating player's animation……";
    frameCounter++;
    if (frameCounter >= frameDelay) {
        frameCounter = 0;
        currentFrame = (currentFrame + 1) % animations[currentAnim].frameCount;
    }
}

void Animation::draw(QPainter& painter, float x, float y, bool flipped) {
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
