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

void Animation::draw(QPainter& painter, float x, float y, bool flipped, bool armed) { // 负责绘制，绘制哪一帧由update决定
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
        if (armed && currentAnim != "knife") paintWeapon(painter, targetRect);
        //painter.drawRect(targetRect);
    } else {
        painter.drawPixmap(targetRect, frame);
        if (armed && currentAnim != "knife") paintWeapon(painter, targetRect);
        //painter.drawRect(targetRect);
    }
}

QRect Animation::getTargetRect(){
    return targetRect;
}

void Animation::loadWeapon(QString weapon){
    QString path = ":/items/assets/items/" + weapon + ".png";
    if (weapon == "rifle" || weapon == "sniper"){
        angle = 270;
    }
    else if (weapon == "knife"){
        angle = 90;
    }
    QPixmap pixmap(path);
    if (pixmap.isNull()){
        QPixmap empty(40,20);
        empty.fill(Qt::transparent);
        pixmap = empty;
    }
    weaponImage = pixmap.scaled(40, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

void Animation::paintWeapon(QPainter &painter, QRect targetRect){
    painter.save();
    QPoint pos(targetRect.x() - 9, targetRect.y() + 17);
    painter.translate(pos.x() + weaponImage.width() / 2,
                       pos.y() + weaponImage.height() / 2);
    painter.rotate(angle);
    painter.translate(-weaponImage.width() / 2,
                       -weaponImage.height() / 2);

    // 绘制图像（此时已经绕中心旋转好）
    painter.drawPixmap(0, 0, weaponImage);

    // 恢复 painter 状态，避免影响其他内容
    painter.restore();
}

void Animation::paintGun(QPainter &painter, float x, float y){
    painter.drawPixmap(x, y, weaponImage);
}
/* TODO：
 * 护甲由于移动时得携带，除非软件生成，否则要增加大量动画资源，故采取血条上显示护甲栏的方式；
 * 武器携带时在后面手的挂点，旋转180度放置；蹲下时只绘制枪械，在前面手托举；
 * 进攻时，拳击与小刀绘制动画；实心球暂且不绘制动画，枪械动画即为一帧托举式，随后生成对应实体。
 * bug：不同武器挂点和旋转角不同，根据下蹲攻击与否绘制武器攻击位置。
 */
