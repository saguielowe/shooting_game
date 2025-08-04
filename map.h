#ifndef MAP_H
#define MAP_H
#include <QVector>
#include <vector>
#include <QRect>
#include <QPixmap>
#include "CollisionResult.h"
class Map
{
public:
    // 获取单例实例的全局访问点
    static Map& getInstance();

    // 根据场景名称加载地图数据
    QPixmap loadScene(const QString& sceneName);

    // 获取碰撞箱数据供其他类使用
    const QVector<QRect>& getCollisionBoxes() const;

    // 假设提供一个方法来检查碰撞
    CollisionResult checkCollision(const QRect& playerRect, const float playerVx, const float playerVy);

    int findPath(QPointF start, QPointF target);

    int checkPlatform(QPointF target);
    int getNextPlatform(int start, int target);
    float distance(const QPointF& a, const QPointF& b) const;
    QPointF findTarget(QPointF start, int nextPlatform);
    QPointF prepareJump(QPointF start, int nextPlatform);
    QPointF prepareFall(QPointF start, int currentPlatform, int nextPlatform);


private:
    QVector <QRect> collisionBox;
    std::vector<std::vector<int>> accessPlatforms; // 储存每个平台id能到达的平台id，id参考collisionbox
    QString currentSceneName;
    QPixmap map;
    Map();
};

#endif // MAP_H
