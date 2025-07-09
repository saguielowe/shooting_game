#ifndef DROPITEM_H
#define DROPITEM_H
#include <QString>
#include <QPainter>
#include "player.h"
class DropItem {
public:
    DropItem(float startX, QString type); // 类型：ball, gun, hp...

    void update(float dt);
    void draw(QPainter& painter);
    void markForDeletion();
    bool isMarkedForDeletion() const;

    QRect hitbox();
    bool isExpired() const;
    bool isNearExpire() const;
    bool isCollectedBy(Player* player);

    QString getType() const;
    QString itemType;

private:
    QPointF position;
    QPointF velocity;
    QPixmap pixmap;

    int lifetime = 300;         // 存活时间
    int flickerStart = 120;     // 开始闪烁
    bool onGround, toBeRemoved=false;
};


#endif // DROPITEM_H
