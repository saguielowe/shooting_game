#include "gameai.h"

GameAI::GameAI(QObject *parent)
    : QObject(parent)
    , m_currentState(AIState::FOLLOW)
    , m_aiTimer(new QTimer(this))
    , m_updateInterval(50)     // 20 FPS
    , m_followDistance(100.0f)
    , m_jumpThreshold(30.0f)
    , m_difficulty(5)
    , m_reactionDelay(200)
    , m_lastUpdateTime(0)
    , m_wasJumping(false)
    , m_jumpCooldown(0)
    , m_gen(m_rd())
    , m_dis(0.0, 1.0)
{
    connect(m_aiTimer, &QTimer::timeout, this, &GameAI::processAI);
}

void GameAI::updateIntent(MoveIntent& moveIntent, AttackIntent& attackIntent)
{
    // 重置意图
    moveIntent = MoveIntent::NONE;
    attackIntent = false;

    // 检查是否有有效的玩家引用
    if (!m_aiPlayer.lock() || !m_targetPlayer.lock()) {
        return;
    }

    // 更新冷却计数
    if (m_jumpCooldown > 0) {
        m_jumpCooldown--;
    }

    // 状态机执行
    switch (m_currentState) {
    case AIState::FOLLOW:
        executeFollow(moveIntent, attackIntent);
        break;

    // === 预留的状态处理 ===
    case AIState::ATTACK:
        executeAttack(moveIntent, attackIntent);
        break;
    case AIState::SEEK_WEAPON:
        executeSeekWeapon(moveIntent, attackIntent);
        break;
    case AIState::SEEK_ARMOR:
        executeSeekArmor(moveIntent, attackIntent);
        break;
    case AIState::DODGE:
        executeDodge(moveIntent, attackIntent);
        break;
    case AIState::RETREAT:
        executeRetreat(moveIntent, attackIntent);
        break;

    default:
        break;
    }

    // 状态转换（目前只有FOLLOW状态）
    AIState nextState = determineNextState();
    if (nextState != m_currentState) {
        m_currentState = nextState;
    }
}

void GameAI::startAI()
{
    m_aiTimer->start(m_updateInterval);
}

void GameAI::stopAI()
{
    m_aiTimer->stop();
}

void GameAI::setDifficulty(int level)
{
    m_difficulty = qBound(1, level, 10);

    // 根据难度调整反应速度
    m_reactionDelay = 400 - (level * 30);  // 100-370ms

    // 预留：后续可以根据难度调整其他参数
    // m_aimAccuracy = 0.3 + (level * 0.07);
    // m_aggressiveness = 0.2 + (level * 0.08);
}

void GameAI::processAI()
{
    // 这里可以做一些周期性的状态更新
    // 目前暂时留空，主要逻辑在updateIntent中
}

// === 当前实现：跟随逻辑 ===

void GameAI::executeFollow(MoveIntent& moveIntent, AttackIntent& attackIntent)
{
    //qDebug() << "following";
    if (!shouldMoveToTarget()) {
        return; // 距离合适，不需要移动
    }

    QPointF aiPos = getPlayerPosition(m_aiPlayer.lock());
    my_platform = Map::getInstance().checkPlatform(aiPos);

    // 计算要移动到的平台，如果我或对手在空中返回-1
    next_platform = Map::getInstance().findPath(aiPos, getPlayerPosition(m_targetPlayer.lock()));

    if (next_platform == -1 && my_platform != -1){
        return; // 对手在空中，我不在空中，不需要移动
    }

    if (my_platform == -1){ // 我在空中，无论对手在哪，都去跳到平台边缘
        moveIntent = calculateMoveDirection(next_target, false); // 改变x去边缘点
    }
    else{ // 我不在空中
        if (my_platform == next_platform){ // 不需要换平台
            moveIntent = calculateMoveDirection(getPlayerPosition(m_targetPlayer.lock())); // 同一平台默认寻路
        }
        else{ // 需要换平台
            next_target = Map::getInstance().findTarget(aiPos, next_platform); // ai存下map的原来指示，前往下一个平台的边缘点
            moveIntent = calculateMoveDirection(next_target); // 前往本平台的起跳位置
        }
    }

    // 预留攻击逻辑接口
    // attackIntent = shouldAttackWhileFollowing();
}


bool GameAI::shouldMoveToTarget()
{
    QPointF aiPos = getPlayerPosition(m_aiPlayer.lock());
    QPointF targetPos = getPlayerPosition(m_targetPlayer.lock());

    float dist = distance(aiPos, targetPos);
    return dist > m_followDistance;
}

MoveIntent GameAI::calculateMoveDirection(QPointF targetPos, bool y)
{
    QPointF aiPos = getPlayerPosition(m_aiPlayer.lock());
    drawPos = targetPos;

    float dx = targetPos.x() - aiPos.x();
    if (targetPos.y() < aiPos.y() - 20 && y){ // 如果目标在上层
        QPointF revised_target = Map::getInstance().prepareJump(aiPos, next_platform);
        drawPos = revised_target;
        dx = revised_target.x() - aiPos.x();
        if (abs(dx) < 40.0f){
            return MoveIntent::JUMP;
        }
        return (dx > 0) ? MoveIntent::MOVING_RIGHT : MoveIntent::MOVING_LEFT;
    }

    if (targetPos.y() > aiPos.y() + 50 && my_platform == 1){ // 仅1号平台下落需要调整目标
        if (my_platform != -1 && next_platform != -1){
            QPointF revised_target = Map::getInstance().prepareFall(aiPos, my_platform, next_platform);
            dx = revised_target.x() - aiPos.x();
            drawPos = revised_target;
            return (dx > 0) ? MoveIntent::MOVING_RIGHT : MoveIntent::MOVING_LEFT;
        }
    }

    // 添加一个死区，避免抖动
    if (abs(dx) < 10.0f) {
        return MoveIntent::NONE;
    }

    return (dx > 0) ? MoveIntent::MOVING_RIGHT : MoveIntent::MOVING_LEFT;
}

bool GameAI::shouldJump(QPointF target)
{
    if (!isPlayerOnGround(m_aiPlayer.lock())) {
        return false; // 已经在空中
    }

    QPointF aiPos = getPlayerPosition(m_aiPlayer.lock());

    // 目标在上方且高度差足够
    if (target.y() < aiPos.y() - m_jumpThreshold) {
        return true;
    }

    // 检查前方是否有障碍需要跳过
    if (isPathBlocked()) {
        return true;
    }

    return false;
}

void GameAI::draw(QPainter& painter){
    painter.setBrush(Qt::red);
    painter.drawRect(drawPos.x(), drawPos.y(), 40, 60);
    painter.drawText(10, 10, "my_platform:" + QString::number(my_platform));
    painter.drawText(10, 20, "next_platform:" + QString::number(next_platform));
}

bool GameAI::isPathBlocked()
{
    // 这里需要根据你的碰撞检测系统实现
    // 暂时返回false，表示路径畅通
    //
    // 实际实现可能是：
    // QPointF aiPos = getPlayerPosition(m_aiPlayer);
    // QPointF targetPos = getPlayerPosition(m_targetPlayer);
    // return physicsWorld->linecast(aiPos, targetPos, PLATFORM_LAYER);

    return false;
}

// === 工具函数实现 ===

QPointF GameAI::getPlayerPosition(std::shared_ptr<Player> player)
{
    return QPointF(player->x, player->y);
}

bool GameAI::isPlayerOnGround(std::shared_ptr<Player> player)
{
    return player->onGround;
}

float GameAI::distance(const QPointF& a, const QPointF& b) const
{
    return std::sqrt(std::pow(a.x() - b.x(), 2) + std::pow(a.y() - b.y(), 2));
}

float GameAI::horizontalDistance(const QPointF& a, const QPointF& b) const
{
    return abs(a.x() - b.x());
}

// === 预留功能的占位符实现 ===

AIState GameAI::determineNextState()
{
    // 目前只返回FOLLOW状态
    // 后续可以根据游戏状态决定状态转换

    /*
    // 预留的状态转换逻辑：
    if (calculateThreat() > 0.8) {
        return AIState::RETREAT;
    }

    if (shouldDodge()) {
        return AIState::DODGE;
    }

    if (shouldSeekWeapon()) {
        return AIState::SEEK_WEAPON;
    }

    if (shouldSeekArmor()) {
        return AIState::SEEK_ARMOR;
    }

    if (hasLineOfSight(getPlayerPosition(m_aiPlayer), getPlayerPosition(m_targetPlayer))) {
        return AIState::ATTACK;
    }
    */

    return AIState::FOLLOW;
}

// 预留函数的空实现
void GameAI::executeAttack(MoveIntent& moveIntent, AttackIntent& attackIntent) {
    // 预留：攻击逻辑
    executeFollow(moveIntent, attackIntent); // 暂时fallback到跟随
    attackIntent = true; // 攻击状态下开火
}

void GameAI::executeSeekWeapon(MoveIntent& moveIntent, AttackIntent& attackIntent) {
    // 预留：寻找武器逻辑
}

void GameAI::executeSeekArmor(MoveIntent& moveIntent, AttackIntent& attackIntent) {
    // 预留：寻找护甲逻辑
}

void GameAI::executeDodge(MoveIntent& moveIntent, AttackIntent& attackIntent) {
    // 预留：闪避逻辑
}

void GameAI::executeRetreat(MoveIntent& moveIntent, AttackIntent& attackIntent) {
    // 预留：撤退逻辑
}

// 预留分析函数
double GameAI::calculateThreat() { return 0.0; }
bool GameAI::shouldSeekWeapon() { return false; }
bool GameAI::shouldSeekArmor() { return false; }
bool GameAI::shouldDodge() { return false; }
bool GameAI::shouldRetreat() { return false; }
QPointF GameAI::findBestWeapon() { return QPointF(); }
QPointF GameAI::findBestArmor() { return QPointF(); }
bool GameAI::hasLineOfSight(const QPointF& from, const QPointF& to) { return true; }
