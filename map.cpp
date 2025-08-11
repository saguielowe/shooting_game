#include "map.h"
#include <queue>
#include <QDebug>
#include <QPixmap>
Map::Map() {}

Map& Map::getInstance() {
    static Map instance; // 静态局部变量，C++11保证线程安全地只创建一次
    return instance;
}

QPixmap Map::loadScene(const QString& sceneName) {
    // qDebug() << "Loading scene: " << sceneName;
    currentSceneName = sceneName;
    map = QPixmap(":/Maps/assets/" + sceneName + "_map.png");
    collisionBox.clear(); // 清除旧的碰撞箱

    // 根据场景名称加载不同的碰撞箱数据
    if (sceneName == "ice") {
        collisionBox << QRect(372, 668, 253, 178) << QRect(238, 248, 534, 44)
                     << QRect(592, 475, 490, 48) << QRect(0, 846, 372, 56)
                     << QRect(0, 474, 425, 48) << QRect(0, 0, 20, 956)
                     << QRect(1004, 0, 20, 956) << QRect(635, 846, 389, 56);
        accessPlatforms.resize(8); // 5个平台编号为 0~4
        accessPlatforms[0].push_back(2);
        accessPlatforms[0].push_back(3);
        accessPlatforms[0].push_back(4);

        accessPlatforms[1].push_back(2);
        accessPlatforms[1].push_back(4);

        accessPlatforms[2].push_back(0);
        accessPlatforms[2].push_back(1);

        accessPlatforms[3].push_back(0);

        accessPlatforms[4].push_back(0);
        accessPlatforms[4].push_back(1);

        accessPlatforms[7].push_back(0);
    }
    else if (sceneName == "default"){
        collisionBox << QRect(368, 705, 259, 192) << QRect(221, 267, 563, 44)
                     << QRect(591, 499, 411, 54) << QRect(17, 897, 351, 61)
                     << QRect(17, 498, 401, 56) << QRect(0, 0, 20, 956)
                     << QRect(1004, 0, 20, 956) << QRect(627, 897, 380, 61);
        accessPlatforms.resize(8); // 5个平台编号为 0~4
        accessPlatforms[0].push_back(2);
        accessPlatforms[0].push_back(3);
        accessPlatforms[0].push_back(4);
        accessPlatforms[0].push_back(7);

        accessPlatforms[1].push_back(2);
        accessPlatforms[1].push_back(4);

        accessPlatforms[2].push_back(0);
        accessPlatforms[2].push_back(1);

        accessPlatforms[3].push_back(0);

        accessPlatforms[4].push_back(0);
        accessPlatforms[4].push_back(1);

        accessPlatforms[7].push_back(0);
    }
    else if (sceneName == "grass"){
        collisionBox << QRect(365, 704, 264, 247) << QRect(221, 267, 560, 61)
                     << QRect(590, 491, 409, 57) << QRect(20, 874, 345, 77)
                     << QRect(17, 491, 401, 57) << QRect(0, 0, 20, 956)
                     << QRect(1004, 0, 20, 956) << QRect(632, 874, 350, 77);
        accessPlatforms.resize(8); // 5个平台编号为 0~4
        accessPlatforms[0].push_back(2);
        accessPlatforms[0].push_back(3);
        accessPlatforms[0].push_back(4);

        accessPlatforms[1].push_back(2);
        accessPlatforms[1].push_back(4);

        accessPlatforms[2].push_back(0);
        accessPlatforms[2].push_back(1);

        accessPlatforms[3].push_back(0);

        accessPlatforms[4].push_back(0);
        accessPlatforms[4].push_back(1);

        accessPlatforms[7].push_back(0);
    }
    return map;
}

const QVector<QRect>& Map::getCollisionBoxes() const {
    return collisionBox;
}

CollisionResult Map::checkCollision(const QRect& playerHitbox, const float playerVx, const float playerVy) {
    const float tolerance = 1;
    CollisionResult result;
    for (const QRect& box : collisionBox) {
        // 首先进行粗略的 AABB 碰撞检测
        if (playerHitbox.intersects(box)) {

            // 如果发生相交，进一步判断碰撞方向
            QRectF overlap = playerHitbox.intersected(box);
            // 1. 判断是否是垂直方向的主导碰撞 (地面或天花板)
            if (playerVy > 0) { // 玩家向下移动 (可能碰到地面)
                // 如果重叠的垂直高度远小于水平宽度，则更可能是垂直碰撞
                if (overlap.height() < overlap.width() + tolerance &&
                        (playerHitbox.bottom() - box.top() >= -tolerance && playerHitbox.bottom() <= box.bottom()))
                {
                    result.direction = "ground";
                    result.correctedPosition = box.top();
                }
            } else if (playerVy < 0) { // 玩家向上移动 (可能碰到天花板)
                    // 如果重叠的垂直高度远小于水平宽度，则更可能是垂直碰撞
                    if (overlap.height() < overlap.width() + tolerance &&
                        (playerHitbox.top() - box.bottom() <= tolerance && playerHitbox.top() >= box.top()))
                    {
                        result.direction = "ceiling";
                        result.correctedPosition = box.bottom();
                    }
                }

                // 2. 判断是否是水平方向的主导碰撞 (左右墙壁)
                if (playerVx > 0) { // 玩家向右移动 (可能碰到右墙)
                    // 如果重叠的水平宽度远小于垂直高度，则更可能是水平碰撞
                    if (overlap.width() < overlap.height() + tolerance &&
                        (playerHitbox.right() - box.left() >= -tolerance && playerHitbox.right() <= box.right()))
                    {
                        result.direction = "right";
                        result.correctedPosition = box.left();
                    }
                }
                if (playerVx < 0) { // 玩家向左移动 (可能碰到左墙)
                    // 如果重叠的水平宽度远小于垂直高度，则更可能是水平碰撞
                    if (overlap.width() < overlap.height() + tolerance &&
                        (playerHitbox.left() - box.right() <= tolerance && playerHitbox.left() >= box.left()))
                    {
                        result.direction = "left";
                        result.correctedPosition = box.right();
                        //qDebug() << "Collision: Left wall with obstacle at " << box;
                    }
                }

                // 如果到达这里，说明发生了重叠，但可能无法明确判断是哪个主要方向的碰撞，
                // 或者同时发生了多方向碰撞。你可能需要更复杂的逻辑来处理这种情况。
                // 这里为了简洁，可以返回第一个检测到的主要方向，或者返回一个通用的"overlap"。
                // 例如，你可以选择在检测到第一个碰撞后就返回，这取决于你的游戏逻辑。
                // 为避免多重碰撞的歧义，更高级的系统会计算 MTV (Minimum Translation Vector)。
            }
        }
    return result;
}

int Map::findPath(QPointF start, QPointF target, bool type){ // type=true表示玩家，偏移60；否则偏移40
    int currentPlatform = checkPlatform(start, type);
    int targetPlatform = checkPlatform(target, type);
    if (currentPlatform == targetPlatform && currentPlatform != -1){ // 如果在同一平台，那么直接过去
        return targetPlatform;
    }
    else if (targetPlatform != -1 && currentPlatform != -1){ // 在空中没法判断
        int nextPlatform = getNextPlatform(currentPlatform, targetPlatform);
        return nextPlatform;
    }
    return -1;
}

QPointF Map::findTarget(QPointF start, int nextPlatform){ // 去找新平台的目标点
    QPointF leftTarget = QPointF(collisionBox[nextPlatform].x() + 80, collisionBox[nextPlatform].y() - 60);
    QPointF rightTarget = QPointF(collisionBox[nextPlatform].x() + collisionBox[nextPlatform].width() - 80, collisionBox[nextPlatform].y() - 60);
    QPointF finalTarget;
    if (distance(leftTarget, start) < distance(rightTarget, start)){
        finalTarget = leftTarget;
    }
    else{
        finalTarget = rightTarget;
    }
    return finalTarget;
}

QPointF Map::prepareJump(QPointF start, int nextPlatform){
    QPointF leftTarget = QPointF(collisionBox[nextPlatform].x() + 80, collisionBox[nextPlatform].y() - 60);
    QPointF rightTarget = QPointF(collisionBox[nextPlatform].x() + collisionBox[nextPlatform].width() - 80, collisionBox[nextPlatform].y() - 60);
    QPointF finalTarget; // 选择这一步需要跳到的位置，但可能需要调整起跳点
    if (distance(leftTarget, start) < distance(rightTarget, start)){
        finalTarget = QPointF(collisionBox[nextPlatform].x() - 140, start.y());
    }
    else{
        finalTarget = QPointF(rightTarget.x() + 220, start.y());
    }
    return finalTarget;
}

QPointF Map::prepareFall(QPointF start, int currentPlatform, int nextPlatform){
    QPointF leftTarget = QPointF(collisionBox[currentPlatform].x() - 80, collisionBox[nextPlatform].y() - 60);
    QPointF rightTarget = QPointF(collisionBox[currentPlatform].x() + collisionBox[currentPlatform].width() + 80, collisionBox[nextPlatform].y() - 60);
    //QPointF finalTarget; // 选择这一步需要跳到的位置，但可能需要调整起跳点
    if (leftTarget.x() > 30){
        return leftTarget;
    }
    return rightTarget;
}

float Map::distance(const QPointF& a, const QPointF& b) const
{
    return std::sqrt(std::pow(a.x() - b.x(), 2) + std::pow(a.y() - b.y(), 2));
}

int Map::checkPlatform(QPointF target, bool type){
    if (type){
        for(int i=0;i<8;i++){
            if(collisionBox[i].y() + 20 > target.y() + 60 && collisionBox[i].y() < target.y() + 60 && target.x() > collisionBox[i].x() - 30 && target.x() < collisionBox[i].x() + collisionBox[i].width() + 30){
                return i; // 注意player有60的高度，坐标取正中央上边
            }
        }
        return -1;
    }
    for(int i=0;i<8;i++){
        if(collisionBox[i].y() + 30 > target.y() + 40 && collisionBox[i].y() - 30 < target.y() + 40 && target.x() > collisionBox[i].x() - 30 && target.x() < collisionBox[i].x() + collisionBox[i].width() + 30){
            return i; // 注意drop有40的高度，坐标取正中央上边
        }
    }
    return -1;
}

int Map::getNextPlatform(int start, int target) {
    const auto& graph = accessPlatforms;
    int n = graph.size();
    std::vector<bool> visited(n, false);
    std::vector<int> parent(n, -1); // 记录路径
    std::queue<int> q;

    q.push(start);
    visited[start] = true;

    while (!q.empty()) {
        int curr = q.front();
        q.pop();

        if (curr == target) break;

        for (int neighbor : graph[curr]) {
            if (!visited[neighbor]) {
                visited[neighbor] = true;
                parent[neighbor] = curr;
                q.push(neighbor);
            }
        }
    }

    if (!visited[target]) {
        return -1; // 无法到达
    }

    // 回溯，找到从目标往前的路径
    int curr = target;
    int prev = parent[target];

    while (prev != -1 && parent[prev] != -1) {
        curr = prev;
        prev = parent[prev];
    }

    return curr; // 起点后第一步
}
