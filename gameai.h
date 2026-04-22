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
#include <array>
#include <QRandomGenerator>
#include "player.h"
#include "map.h"
#include "CollisionResult.h"


using AttackIntent = bool;  // 简单的bool型

// 前向声明，假设你有Player类
class Player;

// AI状态枚举（统一五目标）
enum class AIState {
    IDLE,
    WAIT,             // 等待：等待法术/等待成长窗口
    SURVIVE,          // 保命：撤离、躲避、补给
    GROW,             // 成长：武器/护甲/词条/补给
    CHASE,            // 追击：主动压近到可攻击窗口
    ATTACK            // 攻击：执行当前武器打击策略
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
     // AI 性格类型
     enum class AIPersonality {
          RUSH,       // 冲锋：激进攻击，优先高伤害修饰符
          KITE,       // 风筝：保守远程，避免近战
          SCAVENGER,  // 拾荒：均衡型，优先生存和补给
          TRICKSTER   // 诡术：技巧型，法术和 CDR 优先
     };

    // 统一权重配置：测试时只需要改这一处即可。
    struct AITuning {
        // risk
        float riskEnemyRifleSniper = 0.38f;
        float riskEnemyBall = 0.22f;
        float riskEnemyKnife = 0.12f;
        float riskEnemyArmor = 0.08f;
        float riskEnemyDamageBonus = 0.12f;
        float riskEnemyIronBody = 0.15f;
        float riskEnemyStealth = 0.10f;
        float riskSelfFrozen = 0.45f;
        float riskSelfLowHpScale = 0.45f;

        // state scores
        float waitBase = 0.20f;
        float waitSpellReadyBonus = 0.35f;
        float waitNoGrowthBonus = 0.15f;
        float surviveLowHpScale = 0.50f;
        float surviveRiskScale = 0.60f;
        float surviveEnemyRangedBonus = 0.20f;
        float growBase = 0.50f;
        float growGrowthScoreScale = 0.50f;
        float chaseBase = 0.25f;
        float chaseNoAttackBonus = 0.35f;
        float chaseDistNorm = 420.0f;
        float chaseDistCap = 0.40f;
        float attackCanAttack = 0.75f;
        float attackCannotAttack = 0.10f;
        float attackRangedBonus = 0.10f;
        float attackRiskPenalty = 0.30f;

        // state score modifiers
        float frozenSurviveBonus = 0.45f;
        float enemySpellWaitBonus = 0.05f;
        float activeSpellWaitPenalty = 0.08f;
        float activeSpellAttackBonus = 0.10f;
        float enemyIronBodyAttackPenalty = 0.20f;
        float stealthFirstHitChaseBonus = 0.25f;
        float stealthFirstHitAttackBonus = 0.10f;
        float ironBodyCloseRange = 180.0f;
        float ironBodyCloseSurviveBonus = 0.10f;
        float ironBodyCloseAttackBonus = 0.12f;
        float ironBodyHardenedChaseBonus = 0.10f;

        // hard gates
        float panicRisk = 0.90f;
        float panicHpRatio = 0.15f;
        int stateCommitFrames = 14;

        // personality multipliers [RUSH,KITE,SCAVENGER,TRICKSTER]
        std::array<float, 4> waitMul    = {0.75f, 1.20f, 0.95f, 1.15f};
        std::array<float, 4> surviveMul = {0.80f, 1.30f, 1.00f, 0.95f};
        std::array<float, 4> growMul    = {0.90f, 0.95f, 1.35f, 1.10f};
        std::array<float, 4> chaseMul   = {1.25f, 0.90f, 0.95f, 1.05f};
        std::array<float, 4> attackMul  = {1.35f, 0.95f, 0.95f, 1.20f};

        // stealth spell
        float stealthBase = 0.10f;
        float stealthRiskScale = 0.40f;
        float stealthLowHpScale = 0.20f;
        float stealthFirstHitWindowBonus = 0.30f;
        float stealthFirstHitStateBonus = 0.20f;
        float stealthDefensiveStateBonus = 0.15f;
        float stealthTricksterBonus = 0.10f;
        float stealthVsEnemyIronBodyBonus = 0.12f;
        float stealthMinScore = 0.05f;
        float stealthMaxScore = 0.90f;
        float stealthWindowMinDist = 80.0f;
        float stealthWindowMaxDist = 260.0f;
        float stealthWindowMaxDy = 95.0f;
        float stealthVsIronBodyRange = 180.0f;

        // iron body spell
        float ironBodyBase = 0.08f;
        float ironBodyCloseDist = 190.0f;
        float ironBodyCloseBonus = 0.25f;
        float ironBodyVsMeleeBonus = 0.20f;
        float ironBodyRiskScale = 0.35f;
        float ironBodyLowHpScale = 0.10f;
        float ironBodyThornsBonus = 0.15f;
        float ironBodyHardenedBonus = 0.10f;
        float ironBodyRushBonus = 0.08f;
        float ironBodyMinScore = 0.05f;
        float ironBodyMaxScore = 0.90f;
        float ironBodyThornsRange = 170.0f;
    };

    enum class TuningPreset {
        DEFAULT,
        AGGRESSIVE,
        DEFENSIVE,
        GROWTH_FOCUSED
    };

    void setTuning(const AITuning& tuning) { m_tuning = tuning; }
    const AITuning& getTuning() const { return m_tuning; }
    void resetTuning();
    void applyTuningPreset(TuningPreset preset);

    // 主要接口：设置AI控制的玩家和目标玩家
    void setAIPlayer(const std::shared_ptr<Player>& aiPlayer) { m_aiPlayer = aiPlayer; }
    void setTargetPlayer(const std::shared_ptr<Player>& targetPlayer) { m_targetPlayer = targetPlayer; }

    // 性格系统接口
    void setPersonality(AIPersonality personality) { m_personality = personality; }
        AIPersonality getPersonality() const { return m_personality; }
        static AIPersonality randomPersonality() {
            int p = QRandomGenerator::global()->bounded(4);
            return static_cast<AIPersonality>(p);
        }
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

    // 统一目标执行层
    void executeWait(MoveIntent& moveIntent, AttackIntent& attackIntent);
    void executeSurvive(MoveIntent& moveIntent, AttackIntent& attackIntent);
    void executeGrow(MoveIntent& moveIntent, AttackIntent& attackIntent);
    void executeChase(MoveIntent& moveIntent, AttackIntent& attackIntent);
    void executeAttack(MoveIntent& moveIntent, AttackIntent& attackIntent);

    // 兼容已有追踪路径逻辑
    void executeFollow(MoveIntent& moveIntent, AttackIntent& attackIntent);
    void executeDodge(MoveIntent& moveIntent, AttackIntent& attackIntent);
    void executeRetreat(MoveIntent& moveIntent, AttackIntent& attackIntent);
    bool shouldMoveToTarget();
    MoveIntent calculateMoveDirection(QPointF target, bool y=true);
    bool shouldJump(QPointF target);

    // 寻路核心
    bool isPathBlocked();
    bool needsJumpToReach(const QPointF& target);
    QPointF getPlayerPosition(std::shared_ptr<Player> player) const;
    bool isPlayerOnGround(std::shared_ptr<Player> player) const;

    // 统一状态决策（单次评分）
    AIState determineNextState();
    void executeJump(MoveIntent& moveIntent, QPointF target, QPointF start);

    // 统一成长目标
    enum class GrowthType {
        NONE,
        WEAPON,
        ARMOR,
        SUPPLY,
        MODIFIER
    };

    struct GrowthTarget {
        GrowthType type = GrowthType::NONE;
        QPointF position;
        float score = -1.0f;
    };

    double calculateThreat();
    bool shouldDodge();
    GrowthTarget pickBestGrowthTarget() const;
    float scoreGrowthDrop(const QString& itemType, const QPointF& dropPos, const QPointF& aiPos, float healthRatio) const;
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
    QVector<Player::WeaponType> m_weaponSlots; // 武器槽位列表缓存
    int m_activeSlotIndex = 0; // 目标激活槽位（AI 想要切到的槽位）
    int m_currentSlotIndex = 0; // 当前槽位（上一帧玩家的槽位）
    int m_weaponSwitchCooldown = 0; // 切槽冷却计时器
    void executeMoveTo(QPointF target, MoveIntent& moveIntent, float stopDistance = 10.0f);
    QString getoutput(AIState state);
public:
    void updateDropsInfo(const QVector<DropInfo>& drops) { m_availableDrops = drops; }
    void setCurrentWeapon(const QString& weapon) { m_currentWeapon = weapon; }
    void updateWeaponSlotInfo(const QVector<Player::WeaponType>& weaponSlots, int activeIdx) {
        m_weaponSlots = weaponSlots;
        m_currentSlotIndex = activeIdx;  // 保存玩家当前的槽位，不覆盖 m_activeSlotIndex
    }
    
    // 武器切槽意图消费函数
    int consumeWeaponSwitchIntent() {
        if (m_activeSlotIndex != m_currentSlotIndex) {
            int target = m_activeSlotIndex;
            m_currentSlotIndex = m_activeSlotIndex;  // 标记为已处理
            return target;
        }
        return -1; // 不需要切槽
    }
    // 法术锁：开局选定后，本局内不再切换。
    void setCurrentSpell(GameSession::Spell spell) {
        if (!m_spellLocked && spell != GameSession::Spell::NONE) {
            m_aiSpell = spell;
            m_spellLocked = true;
        }
    }
    bool consumeSpellCastIntent();
    // 在gameai.h中添加：
private:
    bool isRangedWeapon(const QString& weaponType) const;
    void executeRangedAttack(MoveIntent& moveIntent, AttackIntent& attackIntent);
    float getRangedAttackDistance(const QString& weaponType) const;
    AIPersonality m_personality = AIPersonality::RUSH;  // 性格成员需要在这里声明供 shouldCastFreeze 使用
    void handleStealthTarget(MoveIntent& moveIntent, AttackIntent& attackIntent);
    void updateSpellIntent();
    float calculateRisk(const std::shared_ptr<Player>& aiPlayer,
                        const std::shared_ptr<Player>& targetPlayer) const;
    bool shouldCastFreeze();
    bool shouldCastStealth();
    bool shouldCastIronBody();
    bool shouldSwitchWeapon();
    int evaluateWeaponUtility(Player::WeaponType w) const;
private:
    // 预定动作序列
    QQueue<MoveIntent> m_plannedMoves;     // 预定的动作队列
    bool m_executingPlan;                  // 是否在执行预定计划
    QPointF m_lastKnownTargetPos;
    bool    m_targetVisible = true;
        bool m_castSpellIntent = false;
        bool m_spellLocked = false;
        GameSession::Spell m_aiSpell = GameSession::Spell::NONE;
        AITuning m_tuning;

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
    int m_stateCommitFrames;  // 当前状态最短承诺帧
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
