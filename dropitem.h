#ifndef DROPITEM_H
#define DROPITEM_H
#include <QString>
#include <QPainter>
#include "player.h"
#include "entity.h"
class DropItem : public Entity {
public:
    DropItem(float startX, QString type); // 类型：ball, gun, hp...

    void update() override;
    void draw(QPainter& painter) override;
    void markForDeletion();
    bool isMarkedForDeletion() const;

    QRect hitbox() override;
    bool shouldBeRemoved() override{
        return isExpired() || isMarkedForDeletion(); // 要么超时，要么被捡起
    }
    bool isExpired() const;
    bool isNearExpire() const;
    bool isCollectedBy(Player* player);

    QString getType() const;
    QString itemType;

private:
    QPixmap pixmap;

    int lifetime = 300;         // 存活时间
    int flickerStart = 120;     // 开始闪烁
    bool onGround, toBeRemoved=false;
};


#endif // DROPITEM_H
