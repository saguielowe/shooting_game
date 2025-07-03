#ifndef COLLISIONRESULT_H
#define COLLISIONRESULT_H

#include <QPointF>
#include <QRect>
#include <QString>

struct CollisionResult {
    QString direction = "none";
    float correctedPosition; // 玩家应移动到的位置（如修正后的 y）
    QRect obstacleBox;         // 撞上的障碍物（可选，用于判断平台属性等）
};

#endif // COLLISIONRESULT_H
