#ifndef GAMEAI_H
#define GAMEAI_H

#include <QObject>
#include <QPointF>
#include <QVector>
#include <QTimer>
#include <QDebug>
#include <cmath>
#include <QQueue>
#include <random>
#include "player.h"
#include "map.h"
#include "CollisionResult.h"


using AttackIntent = bool;  // 简单的bool型

// 前向声明，假设你有Player类
class Player;

// AI状态枚举（完整架构，目前只实现FOLLOW）
enum class AIState {
    IDLE,
    FOLLOW,           // 当前实现：跟随玩家
    ATTACK,           // 预留：攻击模式
    SEEK_WEAPON,      // 预留：寻找武器
    SEEK_ARMOR,       // 预留：寻找护甲
    DODGE,            // 预留：闪避子弹
    RETREAT,          // 预留：战术撤退
    SEEK_SUPPLY       // 预留：寻找补给
};
// 为QDebug添加MoveIntent支持
inline QDebug operator<<(QDebug debug, const MoveIntent& intent)
{
    QDebugStateSaver saver(debug);
    switch (intent) {
    case MoveIntent::NONE:
        debug.nospace() << "MoveIntent::NONE";
        break;
    case MoveIntent::MOVING_LEFT:
        debug.nospace() << "MoveIntent::MOVING_LEFT";
        break;
    case MoveIntent::MOVING_RIGHT:
        debug.nospace() << "MoveIntent::MOVING_RIGHT";
        break;
    case MoveIntent::CROUCH:
        debug.nospace() << "MoveIntent::CROUCH";
        break;
    case MoveIntent::JUMP:
        debug.nospace() << "MoveIntent::JUMP";
        break;
    }
    return debug;
}
class GameAI : public QObject
{
    Q_OBJECT

public:
    explicit GameAI(QObject *parent = nullptr);

    // 主要接口：设置AI控制的玩家和目标玩家
    void setAIPlayer(const std::shared_ptr<Player>& aiPlayer) { m_aiPlayer = aiPlayer; }
    void setTargetPlayer(const std::shared_ptr<Player>& targetPlayer) { m_targetPlayer = targetPlayer; }

    // 核心接口：更新AI意图
    void updateIntent(MoveIntent& moveIntent, AttackIntent& attackIntent);

    // AI控制
    void startAI();
    void stopAI();
    void setUpdateInterval(int ms) { m_updateInterval = ms; }

    // 参数设置
    void setFollowDistance(float distance) { m_followDistance = distance; }
    void setDifficulty(int level); // 1-10，目前只影响反应速度

    // 状态查询（调试用）
    AIState getCurrentState() const { return m_currentState; }
    void draw(QPainter& painter);

private slots:
    void processAI();

private:
    // === 当前实现的功能 ===

    // 跟随逻辑（当前唯一实现的状态）
    void executeFollow(MoveIntent& moveIntent, AttackIntent& attackIntent);
    bool shouldMoveToTarget();
    MoveIntent calculateMoveDirection(QPointF target, bool y=true);
    bool shouldJump(QPointF target);

    // 寻路核心
    bool isPathBlocked();
    bool needsJumpToReach(const QPointF& target);
    QPointF getPlayerPosition(std::shared_ptr<Player> player);
    bool isPlayerOnGround(std::shared_ptr<Player> player);

    // === 预留的功能接口 ===

    // 状态决策（目前只返回FOLLOW）
    AIState determineNextState();

    // 预留的执行函数
    void executeAttack(MoveIntent& moveIntent, AttackIntent& attackIntent);
    void executeSeekWeapon(MoveIntent& moveIntent, AttackIntent& attackIntent);
    void executeSeekArmor(MoveIntent& moveIntent, AttackIntent& attackIntent);
    void executeDodge(MoveIntent& moveIntent, AttackIntent& attackIntent);
    void executeRetreat(MoveIntent& moveIntent, AttackIntent& attackIntent);
    void executeSeekSupply(MoveIntent& moveIntent, AttackIntent& attackIntent);
    void executeJump(MoveIntent& moveIntent, QPointF target, QPointF start);

    // 预留的分析函数
    double calculateThreat();
    bool shouldSeekWeapon();
    bool shouldSeekArmor();
    bool shouldDodge();
    bool shouldRetreat();
    bool shouldSeekSupply();
    QPointF findBestWeapon();
    QPointF findBestArmor();
    QPointF findBestSupply();
    bool hasLineOfSight(const QPointF& from, const QPointF& to);

    // 工具函数
    float distance(const QPointF& a, const QPointF& b) const;
    float horizontalDistance(const QPointF& a, const QPointF& b) const;
private:
    int getWeaponPriority(const QString& weaponType) const;

public:
    struct DropInfo {
        QPointF position;
        QString itemType;
    };

private:
    QVector<DropInfo> m_availableDrops; // 缓存掉落物信息
    QString m_currentWeapon; // 当前武器类型
    void executeMoveTo(QPointF target, MoveIntent& moveIntent, float stopDistance = 10.0f);
    QString getoutput(AIState state);
public:
    void updateDropsInfo(const QVector<DropInfo>& drops) { m_availableDrops = drops; }
    void setCurrentWeapon(const QString& weapon) { m_currentWeapon = weapon; }
    // 在gameai.h中添加：
private:
    bool isRangedWeapon(const QString& weaponType) const;
    void executeRangedAttack(MoveIntent& moveIntent, AttackIntent& attackIntent);
    float getRangedAttackDistance(const QString& weaponType) const;
private:
    // 预定动作序列
    QQueue<MoveIntent> m_plannedMoves;     // 预定的动作队列
    bool m_executingPlan;                  // 是否在执行预定计划

public:
    void planMoveSequence(const QVector<MoveIntent>& moves); // 设定动作序列
    bool isExecutingPlan() const { return m_executingPlan; }

private:
    // 玩家引用
    std::weak_ptr<Player> m_aiPlayer;        // AI控制的玩家
    std::weak_ptr<Player> m_targetPlayer;    // 目标玩家（通常是人类玩家）

    // AI状态
    AIState m_currentState;

    // 定时器
    QTimer* m_aiTimer;
    int m_updateInterval;      // AI更新间隔（毫秒）

    // 当前功能参数（跟随）
    float m_followDistance;    // 跟随距离
    float m_jumpThreshold;     // 跳跃高度阈值

    // AI难度参数
    int m_difficulty;
    int m_reactionDelay;       // 反应延迟（毫秒）

    // 状态跟踪
    QPointF m_lastTargetPos;
    QPointF next_target;
    QPointF drawPos;
    int next_platform;
    int m_lastUpdateTime;
    int my_platform;
    bool m_wasJumping;
    int m_jumpCooldown;
    int m_lastDodgeTime;      // 闪避冷却时间
    QPointF m_lastPosition;           // 上一帧的位置
    int m_stuckCounter;               // 卡住计数器
    static const int STUCK_THRESHOLD = 40;        // 3秒无移动算卡住(60帧)
    static const int TARGET_REFRESH_THRESHOLD = 100; // 5秒强制刷新目标

    bool isStuck();                   // 检测是否卡住
    void handleStuckSituation(MoveIntent& moveIntent); // 处理卡住情况

    // 预留：随机数生成器（用于后续功能）
    std::random_device m_rd;
    std::mt19937 m_gen;
    std::uniform_real_distribution<> m_dis;
    // 在private部分添加：
    MoveIntent m_lastAttackMove;     // 上次攻击时的移动方向
    int m_attackMoveTimer;           // 攻击走位计时器
    float getAttackRange(std::shared_ptr<Player> player); // 获取攻击范围
};

#endif // GAMEAI_H
