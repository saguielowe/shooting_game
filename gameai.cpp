#include "gameai.h"

namespace {
int personalityIndex(GameAI::AIPersonality personality)
{
    switch (personality) {
    case GameAI::AIPersonality::RUSH: return 0;
    case GameAI::AIPersonality::KITE: return 1;
    case GameAI::AIPersonality::SCAVENGER: return 2;
    case GameAI::AIPersonality::TRICKSTER: return 3;
    }
    return 0;
}
}

GameAI::GameAI(QObject *parent)
    : QObject(parent)
    , m_currentState(AIState::CHASE)
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
    , m_stuckCounter(0)
    , m_lastDodgeTime(0)        // 新增初始化
    , m_stateCommitFrames(0)
    , m_dis(0.0, 1.0)
{
    connect(m_aiTimer, &QTimer::timeout, this, &GameAI::processAI);
}
bool GameAI::isStuck()
{
    auto aiPlayer = m_aiPlayer.lock();
    if (!aiPlayer) return false;

    QPointF currentPos = getPlayerPosition(aiPlayer);

    // 计算位置变化
    float positionChange = distance(currentPos, m_lastPosition);

    if (positionChange < 5.0f) { // 位移很小
        m_stuckCounter++;
    } else {
        m_stuckCounter = 0; // 重置计数器
    }

    m_lastPosition = currentPos;
    return m_stuckCounter >= STUCK_THRESHOLD;
}
void GameAI::handleStuckSituation(MoveIntent& moveIntent)
{
    // 卡住时不做随机乱动，改为刷新目标与状态，避免“无头苍蝇”。
    qDebug() << "AI stuck detected! Retargeting strategy.";

    m_plannedMoves.clear();
    m_executingPlan = false;
    m_stateCommitFrames = 0;
    m_stuckCounter = 0;

    const GrowthTarget growth = pickBestGrowthTarget();
    switch (m_currentState) {
    case AIState::GROW:
        m_currentState = AIState::CHASE;
        break;
    case AIState::CHASE:
    case AIState::ATTACK:
        m_currentState = (growth.type != GrowthType::NONE) ? AIState::GROW : AIState::WAIT;
        break;
    case AIState::WAIT:
        m_currentState = AIState::CHASE;
        break;
    case AIState::SURVIVE:
        m_currentState = (growth.type != GrowthType::NONE) ? AIState::GROW : AIState::CHASE;
        break;
    default:
        m_currentState = AIState::CHASE;
        break;
    }

    // 立刻朝新目标执行一帧，保证“换目标”当下生效。
    switch (m_currentState) {
    case AIState::GROW:
        {
            AttackIntent attackIntent = false;
            executeGrow(moveIntent, attackIntent);
        }
        break;
    case AIState::SURVIVE:
        {
            AttackIntent attackIntent = false;
            executeSurvive(moveIntent, attackIntent);
        }
        break;
    case AIState::WAIT:
        {
            AttackIntent attackIntent = false;
            executeWait(moveIntent, attackIntent);
        }
        break;
    case AIState::CHASE:
    case AIState::ATTACK:
    default:
        {
            AttackIntent attackIntent = false;
            executeChase(moveIntent, attackIntent);
        }
        break;
    }
}
void GameAI::planMoveSequence(const QVector<MoveIntent>& moves)
{
    m_plannedMoves.clear();
    for (const MoveIntent& move : moves) {
        m_plannedMoves.enqueue(move);
    }
    m_executingPlan = !m_plannedMoves.isEmpty();
}
void GameAI::updateIntent(MoveIntent& moveIntent, AttackIntent& attackIntent)
{
    // 重置意图
    moveIntent = MoveIntent::NONE;
    attackIntent = false;
    m_castSpellIntent = false;

    // 检查是否有有效的玩家引用
    if (!m_aiPlayer.lock() || !m_targetPlayer.lock()) {
        return;
    }

    auto target = m_targetPlayer.lock();
    if (target->spellState.stealthActive) {
        m_targetVisible = false;
        // 最后已知位置保持不变，AI去那里找
    } else {
        m_targetVisible = true;
        m_lastKnownTargetPos = getPlayerPosition(target);
    }

    // 隐身时：直接走特殊逻辑，跳过正常状态机
    if (!m_targetVisible) {
        handleStealthTarget(moveIntent, attackIntent);
        return;
    }

    // === 卡住检测和处理（最高优先级） ===
    if (isStuck()) {
        handleStuckSituation(moveIntent);
        return;
    }

    // === 优先执行预定动作序列 ===
    if (m_executingPlan && !m_plannedMoves.isEmpty()) {
        moveIntent = m_plannedMoves.dequeue();
        if (m_plannedMoves.isEmpty()) {
            m_executingPlan = false; // 计划执行完毕
        }
        return; // 直接返回，不执行其他逻辑
    }

    // 统一决策：同一帧内先选目标状态，再产出施法与切槽意图。
    AIState nextState = determineNextState();
    if (nextState != m_currentState) {
        m_currentState = nextState;
    }

    // 施法与切槽同样属于本帧意图的一部分。
    updateSpellIntent();

    const bool shouldSwitch = shouldSwitchWeapon();
    if (shouldSwitch) {
        // 默认切槽优先；但高风险下的防御法术允许抢占切槽。
        const auto aiPlayer = m_aiPlayer.lock();
        const auto targetPlayer = m_targetPlayer.lock();
        const float risk = (aiPlayer && targetPlayer) ? calculateRisk(aiPlayer, targetPlayer) : 0.0f;
        const bool defensiveSpell =
            (m_aiSpell == GameSession::Spell::STEALTH || m_aiSpell == GameSession::Spell::IRON_BODY);

        if (!(m_castSpellIntent && defensiveSpell && risk > 0.70f)) {
            // 切槽帧不施法，避免同帧双意图冲突。
            m_castSpellIntent = false;
            moveIntent = MoveIntent::NONE;
            attackIntent = false;
            return;
        }
    }

    // 状态机执行
    switch (m_currentState) {
    case AIState::WAIT:
        executeWait(moveIntent, attackIntent);
        break;
    case AIState::SURVIVE:
        executeSurvive(moveIntent, attackIntent);
        break;
    case AIState::GROW:
        executeGrow(moveIntent, attackIntent);
        break;
    case AIState::CHASE:
        executeChase(moveIntent, attackIntent);
        break;
    case AIState::ATTACK:
        executeAttack(moveIntent, attackIntent);
        break;
    default:
        break;
    }
}

bool GameAI::consumeSpellCastIntent()
{
    if (!m_castSpellIntent) return false;
    m_castSpellIntent = false;
    return true;
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

void GameAI::resetTuning()
{
    m_tuning = AITuning{};
}

void GameAI::applyTuningPreset(TuningPreset preset)
{
    resetTuning();

    // 预设只改关键方向参数，便于快速 A/B 测试。
    switch (preset) {
    case TuningPreset::DEFAULT:
        break;
    case TuningPreset::AGGRESSIVE:
        m_tuning.attackCanAttack += 0.12f;
        m_tuning.attackRiskPenalty *= 0.70f;
        m_tuning.chaseNoAttackBonus += 0.10f;
        m_tuning.panicRisk += 0.05f;
        m_tuning.stealthDefensiveStateBonus *= 0.7f;
        break;
    case TuningPreset::DEFENSIVE:
        m_tuning.surviveRiskScale += 0.20f;
        m_tuning.surviveEnemyRangedBonus += 0.10f;
        m_tuning.attackRiskPenalty += 0.15f;
        m_tuning.panicHpRatio += 0.05f;
        m_tuning.stealthDefensiveStateBonus += 0.10f;
        break;
    case TuningPreset::GROWTH_FOCUSED:
        m_tuning.growBase += 0.20f;
        m_tuning.growGrowthScoreScale += 0.20f;
        m_tuning.waitNoGrowthBonus += 0.10f;
        m_tuning.chaseNoAttackBonus *= 0.8f;
        m_tuning.attackCanAttack *= 0.9f;
        break;
    }
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

}


bool GameAI::shouldMoveToTarget()
{
    QPointF aiPos = getPlayerPosition(m_aiPlayer.lock());
    QPointF targetPos = getPlayerPosition(m_targetPlayer.lock());

    float dist = distance(aiPos, targetPos);
    return dist > m_followDistance;
}

void GameAI::executeMoveTo(QPointF target, MoveIntent& moveIntent, float stopDistance)
{
    QPointF aiPos = getPlayerPosition(m_aiPlayer.lock());
    my_platform = Map::getInstance().checkPlatform(aiPos);

    // 检查是否已经足够接近目标
    float dist = distance(aiPos, target);
    if (dist <= stopDistance) {
        moveIntent = MoveIntent::NONE;
        return;
    }

    // 计算要移动到的平台
    next_platform = Map::getInstance().findPath(aiPos, target, false); // 移动到掉落物位置

    if (next_platform == -1 && my_platform != -1){
        return; // 目标在空中，我不在空中，不需要移动
    }

    if (my_platform == -1){ // 我在空中
        moveIntent = calculateMoveDirection(next_target, false);
    }
    else{ // 我不在空中
        if (my_platform == next_platform){ // 不需要换平台
            moveIntent = calculateMoveDirection(target);
        }
        else{ // 需要换平台
            next_target = Map::getInstance().findTarget(aiPos, next_platform);
            moveIntent = calculateMoveDirection(next_target);
        }
    }
}
MoveIntent GameAI::calculateMoveDirection(QPointF targetPos, bool y)
{
    QPointF aiPos = getPlayerPosition(m_aiPlayer.lock());
    drawPos = targetPos;

    float dx = targetPos.x() - aiPos.x();
    if (targetPos.y() < aiPos.y() - 20 && y){ // 如果目标在上层
        if (next_platform == -1) return MoveIntent::NONE;
        QPointF revised_target = Map::getInstance().prepareJump(aiPos, next_platform);
        drawPos = revised_target;
        dx = revised_target.x() - aiPos.x();
        if (abs(dx) < 40.0f){
            //确定一下跳跃方向再起跳
            QVector<MoveIntent> jumpSequence;
            if ((targetPos.x() < aiPos.x()) && (m_aiPlayer.lock()->direction == 1)){ // 目标左，方向右
                auto firstIntent = MoveIntent::MOVING_LEFT;
                jumpSequence.append(firstIntent);
                jumpSequence.append(MoveIntent::JUMP);
                return firstIntent; // 二楼目标在左，往左起跳
            }
            else if ((targetPos.x() > aiPos.x()) && (m_aiPlayer.lock()->direction == 0)){
                auto firstIntent = MoveIntent::MOVING_RIGHT;
                jumpSequence.append(firstIntent);
                jumpSequence.append(MoveIntent::JUMP);
                return firstIntent;
            }
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

void GameAI::draw(QPainter& painter){
    // painter.setBrush(Qt::red);
    // painter.drawRect(drawPos.x(), drawPos.y(), 40, 60);
    // painter.drawText(10, 10, "my_platform:" + QString::number(my_platform));
    // painter.drawText(10, 20, "next_platform:" + QString::number(next_platform));
    // painter.drawText(10, 30, "AI state:" + getoutput(m_currentState));
}

// === 工具函数实现 ===
QString GameAI::getoutput(AIState state){
    if (state == AIState::WAIT){
        return "WAIT";
    }
    else if (state == AIState::SURVIVE){
        return "SURVIVE";
    }
    else if (state == AIState::GROW){
        return "GROW";
    }
    else if (state == AIState::CHASE){
        return "CHASE";
    }
    else if (state == AIState::ATTACK){
        return "ATTACK";
    }
    else{
        return "NONE";
    }
}

QPointF GameAI::getPlayerPosition(std::shared_ptr<Player> player) const
{
    return QPointF(player->x, player->y);
}

bool GameAI::isPlayerOnGround(std::shared_ptr<Player> player) const
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

float GameAI::calculateRisk(const std::shared_ptr<Player>& aiPlayer,
                            const std::shared_ptr<Player>& targetPlayer) const
{
    if (!aiPlayer || !targetPlayer) return 0.0f;

    const float healthRatio = (float)aiPlayer->hp / aiPlayer->getMaxHp();
    float risk = 0.0f;

    if (targetPlayer->weapon == Player::WeaponType::rifle ||
        targetPlayer->weapon == Player::WeaponType::sniper) {
        risk += m_tuning.riskEnemyRifleSniper;
    } else if (targetPlayer->weapon == Player::WeaponType::ball) {
        risk += m_tuning.riskEnemyBall;
    } else if (targetPlayer->weapon == Player::WeaponType::knife) {
        risk += m_tuning.riskEnemyKnife;
    }

    if (targetPlayer->armor != Player::ArmorType::noArmor) risk += m_tuning.riskEnemyArmor;
    if (targetPlayer->modifiers.damageBonusMultiplier > 1.2f) risk += m_tuning.riskEnemyDamageBonus;
    if (targetPlayer->spellState.ironBodyActive) risk += m_tuning.riskEnemyIronBody;
    if (targetPlayer->spellState.stealthActive) risk += m_tuning.riskEnemyStealth;
    if (aiPlayer->spellState.isFrozen) risk += m_tuning.riskSelfFrozen;

    risk += (1.0f - healthRatio) * m_tuning.riskSelfLowHpScale;
    return qBound(0.0f, risk, 1.0f);
}

// === 预留功能的占位符实现 ===

AIState GameAI::determineNextState()
{
    auto aiPlayer = m_aiPlayer.lock();
    auto targetPlayer = m_targetPlayer.lock();
    if (!aiPlayer || !targetPlayer) return AIState::CHASE;

    const QPointF aiPos = getPlayerPosition(aiPlayer);
    const QPointF targetPos = getPlayerPosition(targetPlayer);
    const float healthRatio = (float)aiPlayer->hp / aiPlayer->getMaxHp();
    const float dist = distance(aiPos, targetPos);
    const float dy = std::abs(targetPos.y() - aiPos.y());
    const bool enemyRanged = (targetPlayer->weapon == Player::WeaponType::rifle ||
                              targetPlayer->weapon == Player::WeaponType::sniper ||
                              targetPlayer->weapon == Player::WeaponType::ball);
    const bool aiSpellOnCd = aiPlayer->spellState.isOnCooldown();
    const bool aiSpellActive = aiPlayer->spellState.stealthActive || aiPlayer->spellState.ironBodyActive ||
                               aiPlayer->spellState.barrierActive || aiPlayer->spellState.cloneActive;
    const bool enemySpellActive = targetPlayer->spellState.stealthActive || targetPlayer->spellState.ironBodyActive ||
                                  targetPlayer->spellState.barrierActive || targetPlayer->spellState.cloneActive ||
                                  targetPlayer->spellState.isFrozen;

    const float risk = calculateRisk(aiPlayer, targetPlayer);
    const bool canSpellSoon = !aiSpellOnCd;
    const bool hasSight = hasLineOfSight(aiPos, targetPos);
    const bool canAttack = isRangedWeapon(m_currentWeapon)
                               ? hasSight
                               : (horizontalDistance(aiPos, targetPos) <= 140.0f && dy <= 90.0f);

    const GrowthTarget growth = pickBestGrowthTarget();
    const bool canGrow = (growth.type != GrowthType::NONE);
    const bool hasStealthFirstHit = aiPlayer->modifiers.stealthFirstHit && !aiPlayer->spellState.stealthFirstHitUsed;
    const bool enemyIronBodyUp = targetPlayer->spellState.ironBodyActive;

    float sWait = m_tuning.waitBase + (canSpellSoon ? m_tuning.waitSpellReadyBonus : 0.0f) +
                  (canGrow ? 0.0f : m_tuning.waitNoGrowthBonus);
    float sSurvive = m_tuning.surviveLowHpScale * (1.0f - healthRatio) + m_tuning.surviveRiskScale * risk +
                     (enemyRanged ? m_tuning.surviveEnemyRangedBonus : 0.0f);
    float sGrow = canGrow ? (m_tuning.growBase + m_tuning.growGrowthScoreScale * growth.score) : -1.0f;
    float sChase = m_tuning.chaseBase + (canAttack ? 0.0f : m_tuning.chaseNoAttackBonus) +
                   qBound(0.0f, dist / m_tuning.chaseDistNorm, m_tuning.chaseDistCap);
    float sAttack = (canAttack ? m_tuning.attackCanAttack : m_tuning.attackCannotAttack) +
                    (isRangedWeapon(m_currentWeapon) ? m_tuning.attackRangedBonus : 0.0f) -
                    m_tuning.attackRiskPenalty * risk;

    // 法术状态与词条对上层目标的影响：统一在状态分数层做加减。
    if (aiPlayer->spellState.isFrozen) sSurvive += m_tuning.frozenSurviveBonus;
    if (enemySpellActive) sWait += m_tuning.enemySpellWaitBonus;
    if (enemyIronBodyUp) sAttack -= m_tuning.enemyIronBodyAttackPenalty;
    if (aiSpellActive) {
        sWait -= m_tuning.activeSpellWaitPenalty;
        sAttack += m_tuning.activeSpellAttackBonus;
    }
    if (m_aiSpell == GameSession::Spell::STEALTH && hasStealthFirstHit) {
        if (dist > 90.0f && dist < 260.0f && dy < 90.0f) {
            sChase += m_tuning.stealthFirstHitChaseBonus;
            sAttack += m_tuning.stealthFirstHitAttackBonus;
        }
    }
    if (m_aiSpell == GameSession::Spell::IRON_BODY) {
        if (dist < m_tuning.ironBodyCloseRange) {
            sSurvive += m_tuning.ironBodyCloseSurviveBonus;
            sAttack += m_tuning.ironBodyCloseAttackBonus;
        }
        if (aiPlayer->modifiers.ironBodyHardened) sChase += m_tuning.ironBodyHardenedChaseBonus;
    }

    const int pIdx = personalityIndex(m_personality);
    sWait *= m_tuning.waitMul[pIdx];
    sSurvive *= m_tuning.surviveMul[pIdx];
    sGrow *= m_tuning.growMul[pIdx];
    sChase *= m_tuning.chaseMul[pIdx];
    sAttack *= m_tuning.attackMul[pIdx];

    AIState bestState = AIState::WAIT;
    float bestScore = sWait;
    if (sSurvive > bestScore) { bestScore = sSurvive; bestState = AIState::SURVIVE; }
    if (sGrow > bestScore) { bestScore = sGrow; bestState = AIState::GROW; }
    if (sChase > bestScore) { bestScore = sChase; bestState = AIState::CHASE; }
    if (sAttack > bestScore) { bestScore = sAttack; bestState = AIState::ATTACK; }

    // 危机打断：保命优先。
    if (risk > m_tuning.panicRisk || healthRatio < m_tuning.panicHpRatio) {
        bestState = AIState::SURVIVE;
    }

    // 最短承诺窗口：避免因玩家连续跳跃导致每帧变状态。
    if (m_currentState != bestState && m_stateCommitFrames > 0 && m_currentState != AIState::SURVIVE) {
        --m_stateCommitFrames;
        return m_currentState;
    }

    if (m_currentState != bestState) {
        m_stateCommitFrames = m_tuning.stateCommitFrames;
    } else if (m_stateCommitFrames > 0) {
        --m_stateCommitFrames;
    }

    return bestState;

}

void GameAI::executeWait(MoveIntent& moveIntent, AttackIntent& attackIntent)
{
    auto aiPlayer = m_aiPlayer.lock();
    auto targetPlayer = m_targetPlayer.lock();
    if (!aiPlayer || !targetPlayer) return;

    QPointF aiPos = getPlayerPosition(aiPlayer);
    QPointF targetPos = getPlayerPosition(targetPlayer);
    float dx = targetPos.x() - aiPos.x();

    // 等待不是站桩：保持小幅横向机动，顺便等技能窗口。
    if (std::abs(dx) > 140.0f) {
        moveIntent = (dx > 0) ? MoveIntent::MOVING_RIGHT : MoveIntent::MOVING_LEFT;
    } else if (std::abs(dx) < 70.0f) {
        moveIntent = (dx > 0) ? MoveIntent::MOVING_LEFT : MoveIntent::MOVING_RIGHT;
    } else {
        moveIntent = MoveIntent::NONE;
    }
    attackIntent = false;
}

void GameAI::executeSurvive(MoveIntent& moveIntent, AttackIntent& attackIntent)
{
    if (shouldDodge()) {
        executeDodge(moveIntent, attackIntent);
        return;
    }
    executeRetreat(moveIntent, attackIntent);
}

void GameAI::executeGrow(MoveIntent& moveIntent, AttackIntent& attackIntent)
{
    auto aiPlayer = m_aiPlayer.lock();
    if (!aiPlayer) return;

    GrowthTarget target = pickBestGrowthTarget();
    if (target.type == GrowthType::NONE) {
        executeChase(moveIntent, attackIntent);
        return;
    }

    QPointF aiPos = getPlayerPosition(aiPlayer);
    float distX = abs(aiPos.x() - target.position.x());
    float distAll = distance(aiPos, target.position);

    if (distX < 20.0f && distAll < 100.0f) {
        moveIntent = MoveIntent::CROUCH;
        attackIntent = false;
        return;
    }

    executeMoveTo(target.position, moveIntent, 20.0f);
    attackIntent = false;
}

void GameAI::executeChase(MoveIntent& moveIntent, AttackIntent& attackIntent)
{
    executeFollow(moveIntent, attackIntent);
    attackIntent = false;
}

bool GameAI::shouldDodge()
{
    auto aiPlayer = m_aiPlayer.lock();
    auto targetPlayer = m_targetPlayer.lock();
    if (!aiPlayer || !targetPlayer) return false;

    // 闪避冷却时间
    if (m_lastDodgeTime > 0) {
        m_lastDodgeTime--;
        return false;
    }

    QPointF aiPos = getPlayerPosition(aiPlayer);
    QPointF targetPos = getPlayerPosition(targetPlayer);

    // 获取对手武器类型（你需要实现这个接口）
    Player::WeaponType enemyWeapon = targetPlayer->weapon; // 需要你在widget中提供

    float dist = distance(aiPos, targetPos);

    // 对手有枪械且在同一高度
    if ((enemyWeapon == Player::WeaponType::rifle || enemyWeapon == Player::WeaponType::sniper) &&
        abs(targetPos.y() - aiPos.y()) <= 80.0f) {
        return true;
    }

    // 对手扔实心球，距离较近时闪避
    if (enemyWeapon == Player::WeaponType::ball && dist < 400.0f) {
        return true;
    }

    return false;
}

void GameAI::executeDodge(MoveIntent& moveIntent, AttackIntent& attackIntent)
{
    auto aiPlayer = m_aiPlayer.lock();
    auto targetPlayer = m_targetPlayer.lock();
    if (!aiPlayer || !targetPlayer) return;

    QPointF aiPos = getPlayerPosition(aiPlayer);
    QPointF targetPos = getPlayerPosition(targetPlayer);

    Player::WeaponType enemyWeapon = targetPlayer->weapon;
    float dist = distance(aiPos, targetPos);

    if (enemyWeapon == Player::WeaponType::rifle || enemyWeapon == Player::WeaponType::sniper) {
        // 对付枪械：疯狂跳跃干扰瞄准
        if (isPlayerOnGround(aiPlayer)) {
            moveIntent = MoveIntent::JUMP;
        }

        // 但不能放弃进攻机会！
        // 如果距离合适且有好武器，边跳边靠近
        // if (dist > 120.0f && isRangedWeapon(m_currentWeapon)) {
        //     // 一边跳一边靠近
        //     float dx = targetPos.x() - aiPos.x();
        //     if (abs(dx) > 20.0f) {
        //         MoveIntent approachMove = (dx > 0) ? MoveIntent::MOVING_RIGHT : MoveIntent::MOVING_LEFT;

        //         // 50%概率跳跃，50%概率靠近
        //         if (m_dis(m_gen) > 0.5) {
        //             moveIntent = MoveIntent::JUMP;
        //         } else {
        //             moveIntent = approachMove;
        //         }
        //     }
        // }

        // // 在合适距离内可以反击
        // if (dist <= 200.0f && isRangedWeapon(m_currentWeapon)) {
        //     attackIntent = true; // 边闪避边反击
        // }

        m_lastDodgeTime = 15; // 短冷却，保持跳跃频率
    }
    else if (enemyWeapon == Player::WeaponType::ball) {
        // 对付实心球：执行撤退逻辑
        executeRetreat(moveIntent, attackIntent);
        m_lastDodgeTime = 40; // 较长冷却
    }
    else {
        // 其他情况回到正常状态
        executeFollow(moveIntent, attackIntent);
    }
}

void GameAI::executeRetreat(MoveIntent& moveIntent, AttackIntent& attackIntent)
{
    auto aiPlayer = m_aiPlayer.lock();
    auto targetPlayer = m_targetPlayer.lock();
    if (!aiPlayer || !targetPlayer) return;

    QPointF aiPos = getPlayerPosition(aiPlayer);
    QPointF targetPos = getPlayerPosition(targetPlayer);
    float currentDist = distance(aiPos, targetPos);

    // 如果已经足够远了，就停下
    if (currentDist > 180.0f) {
        moveIntent = MoveIntent::NONE;
        attackIntent = false;
        return;
    }

    // 距离太近，慌乱地随机移动：左30%，右30%，跳40%
    double randomValue = m_dis(m_gen);

    if (randomValue < 0.3) {
        moveIntent = MoveIntent::MOVING_LEFT;
    }
    else if (randomValue < 0.6) {
        moveIntent = MoveIntent::MOVING_RIGHT;
    }
    else {
        moveIntent = MoveIntent::JUMP;
    }

    attackIntent = false;
}

// 预留分析函数
double GameAI::calculateThreat() { return 0.0; }
GameAI::GrowthTarget GameAI::pickBestGrowthTarget() const
{
    auto aiPlayer = m_aiPlayer.lock();
    if (!aiPlayer) return GrowthTarget{};

    const QPointF aiPos = getPlayerPosition(aiPlayer);
    const float healthRatio = (float)aiPlayer->hp / aiPlayer->getMaxHp();

    GrowthTarget best;
    for (const auto& drop : m_availableDrops) {
        const float score = scoreGrowthDrop(drop.itemType, drop.position, aiPos, healthRatio);
        if (score > best.score) {
            best.score = score;
            best.position = drop.position;
            if (drop.itemType == "modifier") best.type = GrowthType::MODIFIER;
            else if (drop.itemType == "chainmail" || drop.itemType == "vest") best.type = GrowthType::ARMOR;
            else if (drop.itemType == "bandage" || drop.itemType == "medkit" || drop.itemType == "adrenaline") best.type = GrowthType::SUPPLY;
            else best.type = GrowthType::WEAPON;
        }
    }

    if (best.score < 0.0f) return GrowthTarget{};
    return best;
}

float GameAI::scoreGrowthDrop(const QString& itemType, const QPointF& dropPos, const QPointF& aiPos, float healthRatio) const
{
    const float dropDistance = distance(aiPos, dropPos);
    if (dropDistance > 500.0f) return -1.0f;

    float score = (500.0f - dropDistance) / 500.0f;

    if (itemType == "modifier") {
        score += 0.35f;
        if (m_personality == AIPersonality::TRICKSTER) score += 0.30f;
        if (m_personality == AIPersonality::RUSH) score += 0.15f;
    } else if (itemType == "chainmail" || itemType == "vest") {
        score += 0.20f;
        if (m_personality == AIPersonality::KITE) score += 0.35f;
    } else if (itemType == "bandage" || itemType == "medkit" || itemType == "adrenaline") {
        score += (1.0f - healthRatio) * 0.80f;
        if (m_personality == AIPersonality::KITE) score += 0.20f;
        if (m_personality == AIPersonality::RUSH) score -= 0.10f;
    } else {
        const int currentPriority = getWeaponPriority(m_currentWeapon.isEmpty() ? "punch" : m_currentWeapon);
        const int dropPriority = getWeaponPriority(itemType);
        if (dropPriority <= currentPriority) return -1.0f;
        score += 0.20f * (dropPriority - currentPriority);
        if (m_personality == AIPersonality::RUSH) score += 0.20f;
        if (m_personality == AIPersonality::SCAVENGER) score += 0.10f;
    }

    return score;
}

int GameAI::getWeaponPriority(const QString& weaponType) const
{
    if (weaponType == "sniper") return 5;
    if (weaponType == "rifle") return 4;
    if (weaponType == "ball") return 3;
    if (weaponType == "knife") return 2;
    if (weaponType == "punch") return 1;
    return 0; // 未知武器
}

void GameAI::executeAttack(MoveIntent& moveIntent, AttackIntent& attackIntent)
{
    auto aiPlayer = m_aiPlayer.lock();
    auto targetPlayer = m_targetPlayer.lock();
    if (!aiPlayer || !targetPlayer) return;

    // 检查是否是远程武器
    if (isRangedWeapon(m_currentWeapon)) {
        executeRangedAttack(moveIntent, attackIntent);
        return;
    }

    // 原有的近战逻辑保持不变...
    QPointF aiPos = getPlayerPosition(aiPlayer);
    QPointF targetPos = getPlayerPosition(targetPlayer);
    float horizontalDist = horizontalDistance(aiPos, targetPos);
    float dist = distance(aiPos, targetPos);

    float attackRange = 40.0f;
    float attackRangeWithError = attackRange + (m_dis(m_gen) - 0.5) * 40.0f;

    if (horizontalDist <= attackRangeWithError && dist <= attackRangeWithError) { // 使得ai有可能在错误的距离发动攻击，降低命中率
        attackIntent = true;

        if (m_attackMoveTimer <= 0) {
            if (aiPos.x() < targetPos.x()) {
                m_lastAttackMove = (m_dis(m_gen) > 0.3) ? MoveIntent::MOVING_LEFT : MoveIntent::MOVING_RIGHT;
            } else {
                m_lastAttackMove = (m_dis(m_gen) > 0.3) ? MoveIntent::MOVING_RIGHT : MoveIntent::MOVING_LEFT;
            }
            m_attackMoveTimer = 20 + (int)(m_dis(m_gen) * 30);
        }

        if (m_dis(m_gen) > 0.3) {
            moveIntent = m_lastAttackMove;
        }
        m_attackMoveTimer--;
    }
    else {
        attackIntent = false;
        executeMoveTo(targetPos, moveIntent, 35.0f);
    }
}

bool GameAI::isRangedWeapon(const QString& weaponType) const
{
    return (weaponType == "ball" || weaponType == "rifle" || weaponType == "sniper");
}

void GameAI::executeRangedAttack(MoveIntent& moveIntent, AttackIntent& attackIntent)
{
    auto aiPlayer = m_aiPlayer.lock();
    auto targetPlayer = m_targetPlayer.lock();
    if (!aiPlayer || !targetPlayer) return;

    QPointF aiPos = getPlayerPosition(aiPlayer);
    QPointF targetPos = getPlayerPosition(targetPlayer);
    float dist = distance(aiPos, targetPos);
    float maxRange = getRangedAttackDistance(m_currentWeapon);

    // 距离太近，需要拉开距离
    if (dist < 150.0f) {
        // 简单后退：往反方向移动
        float dx = aiPos.x() - targetPos.x();
        if (abs(dx) > 10.0f) {
            moveIntent = (dx > 0) ? MoveIntent::MOVING_RIGHT : MoveIntent::MOVING_LEFT;
        }
        attackIntent = false;
        return;
    }

    // 在合适的攻击距离内
    if (m_currentWeapon == "ball") {
        // 实心球：简单预判，不追求精确
        // 只要大概比对手高就开火
        if (targetPos.y() >= aiPos.y() - 20) {
            if (abs(aiPlayer->vx) > 10){ // 跑起来再开火
                attackIntent = true;
            }
            else{
                float dx = aiPos.x() - targetPos.x();
                moveIntent = (dx < 0) ? MoveIntent::MOVING_RIGHT : MoveIntent::MOVING_LEFT;
            }
        }
        else{
            executeFollow(moveIntent, attackIntent); // 至少得和对手一样高
            return;
        }
    }
    else { // rifle或sniper
        // 枪械：精确瞄准
        if (abs(targetPos.y() - aiPos.y()) <= 60.0f) {
            if ((aiPlayer->direction) == (targetPos.x() - aiPos.x() > 0)){ // 确保射击方向正确
                attackIntent = true;
            }
            else{
                float dx = aiPos.x() - targetPos.x();
                moveIntent = (dx < 0) ? MoveIntent::MOVING_RIGHT : MoveIntent::MOVING_LEFT;
            }
        }
        else{
            executeFollow(moveIntent, attackIntent); // 至少得和对手一样高
            return;
        }
    }
}

float GameAI::getRangedAttackDistance(const QString& weaponType) const
{
    if (weaponType == "sniper") return 400.0f;
    if (weaponType == "rifle") return 300.0f;
    if (weaponType == "ball") return 250.0f;
    return 100.0f;
}

void GameAI::handleStealthTarget(MoveIntent& moveIntent, AttackIntent& attackIntent) {
    auto aiPlayer = m_aiPlayer.lock();
    if (!aiPlayer) return;

    attackIntent = false;  // 看不到目标不攻击

    QPointF aiPos = getPlayerPosition(aiPlayer);
    float dist = distance(aiPos, m_lastKnownTargetPos);

    if (dist > 40.f) {
        // 还没到最后已知位置，继续走过去
        executeMoveTo(m_lastKnownTargetPos, moveIntent, 40.f);
    } else {
        executeAttack(moveIntent, attackIntent);
    }
}

void GameAI::updateSpellIntent()
{
    m_castSpellIntent = false;

    switch (m_aiSpell) {
    case GameSession::Spell::FREEZE:
        m_castSpellIntent = shouldCastFreeze();
        break;
    case GameSession::Spell::STEALTH:
        m_castSpellIntent = shouldCastStealth();
        break;
    case GameSession::Spell::IRON_BODY:
        m_castSpellIntent = shouldCastIronBody();
        break;
    default:
        m_castSpellIntent = false;
        break;
    }
}

bool GameAI::shouldCastFreeze()
{
    auto aiPlayer = m_aiPlayer.lock();
    auto targetPlayer = m_targetPlayer.lock();
    if (!aiPlayer || !targetPlayer) return false;

    if (aiPlayer->spellState.isOnCooldown()) return false;
    if (aiPlayer->spellState.isFrozen) return false;
    if (targetPlayer->spellState.isFrozen) return false;
    if (!m_targetVisible) return false;

    QPointF aiPos = getPlayerPosition(aiPlayer);
    QPointF targetPos = getPlayerPosition(targetPlayer);
    float dist = distance(aiPos, targetPos);
    float dy = std::abs(targetPos.y() - aiPos.y());

    // FREEZE 先做窗口型施法：中近距离、同层优先
    if (dist < 70.f || dist > 300.f) return false;
    if (dy > 90.f) return false;

    // 优化：不在空中定身持近战武器的敌人（容易逃脱）
    if (!targetPlayer->onGround && 
        (targetPlayer->weapon == Player::WeaponType::punch || 
         targetPlayer->weapon == Player::WeaponType::knife)) {
        return false;  // 敌人空中做近战，不定身
    }

    float aiHpRatio = static_cast<float>(aiPlayer->hp) / aiPlayer->getMaxHp();
    float targetHpRatio = static_cast<float>(targetPlayer->hp) / targetPlayer->getMaxHp();

    float score = 0.15f;
    if (targetHpRatio > aiHpRatio + 0.1f) score += 0.25f;
    if (targetPlayer->weapon == Player::WeaponType::rifle ||
        targetPlayer->weapon == Player::WeaponType::sniper) {
        score += 0.2f;
    }
    if (dist >= 110.f && dist <= 240.f) {
        score += 0.25f;
    }
    if (targetHpRatio < 0.15f) {
        score -= 0.2f;
    }

    // 性格驱动的 FREEZE 时机
    if (m_personality == AIPersonality::TRICKSTER) {
        // 诡术型：冷却好了就用，追求频繁施法
        score += 0.3f;  // 大幅提高欲望
    } else {
        // 其他性格：等待高伤武器或词条后再用
        if (aiPlayer->weapon == Player::WeaponType::rifle || 
            aiPlayer->weapon == Player::WeaponType::sniper) {
            score += 0.15f;  // 持远程武器时提高定身欲望
        }
        if (aiPlayer->modifiers.damageBonusMultiplier > 1.2f) {
            score += 0.1f;  // 有高伤词条时更想定身
        }
    }

    score = qBound(0.1f, score, 0.85f);
    return m_dis(m_gen) < score;
}

bool GameAI::shouldCastStealth()
{
    auto aiPlayer = m_aiPlayer.lock();
    auto targetPlayer = m_targetPlayer.lock();
    if (!aiPlayer || !targetPlayer) return false;

    if (aiPlayer->spellState.isOnCooldown()) return false;
    if (aiPlayer->spellState.isFrozen) return false;
    if (aiPlayer->spellState.stealthActive) return false;
    if (aiPlayer->spellState.ironBodyActive) return false;

    const QPointF aiPos = getPlayerPosition(aiPlayer);
    const QPointF targetPos = getPlayerPosition(targetPlayer);
    const float dist = distance(aiPos, targetPos);
    const float dy = std::abs(targetPos.y() - aiPos.y());
    const float hpRatio = static_cast<float>(aiPlayer->hp) / aiPlayer->getMaxHp();
    const float risk = calculateRisk(aiPlayer, targetPlayer);
    const bool firstHitReady = aiPlayer->modifiers.stealthFirstHit && !aiPlayer->spellState.stealthFirstHitUsed;

    float score = m_tuning.stealthBase;
    score += risk * m_tuning.stealthRiskScale;
    score += (1.0f - hpRatio) * m_tuning.stealthLowHpScale;

    if (firstHitReady) {
        // 有破影一击时，隐身也用于主动造击杀窗口。
        if (dist > m_tuning.stealthWindowMinDist && dist < m_tuning.stealthWindowMaxDist &&
            dy < m_tuning.stealthWindowMaxDy) {
            score += m_tuning.stealthFirstHitWindowBonus;
        }
        if (m_currentState == AIState::CHASE || m_currentState == AIState::ATTACK) {
            score += m_tuning.stealthFirstHitStateBonus;
        }
    } else {
        // 没有破影一击时，隐身更偏保命与转线。
        if (m_currentState == AIState::SURVIVE || m_currentState == AIState::GROW) {
            score += m_tuning.stealthDefensiveStateBonus;
        }
    }

    if (m_personality == AIPersonality::TRICKSTER) score += m_tuning.stealthTricksterBonus;
    if (targetPlayer->spellState.ironBodyActive && dist < m_tuning.stealthVsIronBodyRange) {
        score += m_tuning.stealthVsEnemyIronBodyBonus;
    }

    score = qBound(m_tuning.stealthMinScore, score, m_tuning.stealthMaxScore);
    return m_dis(m_gen) < score;
}

bool GameAI::shouldCastIronBody()
{
    auto aiPlayer = m_aiPlayer.lock();
    auto targetPlayer = m_targetPlayer.lock();
    if (!aiPlayer || !targetPlayer) return false;

    if (aiPlayer->spellState.isOnCooldown()) return false;
    if (aiPlayer->spellState.isFrozen) return false;
    if (aiPlayer->spellState.ironBodyActive) return false;
    if (aiPlayer->spellState.stealthActive) return false;

    const QPointF aiPos = getPlayerPosition(aiPlayer);
    const QPointF targetPos = getPlayerPosition(targetPlayer);
    const float dist = distance(aiPos, targetPos);
    const float hpRatio = static_cast<float>(aiPlayer->hp) / aiPlayer->getMaxHp();
    const float risk = calculateRisk(aiPlayer, targetPlayer);

    float score = m_tuning.ironBodyBase;
    if (dist < m_tuning.ironBodyCloseDist) score += m_tuning.ironBodyCloseBonus;
    if (targetPlayer->weapon == Player::WeaponType::punch || targetPlayer->weapon == Player::WeaponType::knife) {
        score += m_tuning.ironBodyVsMeleeBonus;
    }
    score += risk * m_tuning.ironBodyRiskScale;
    score += (1.0f - hpRatio) * m_tuning.ironBodyLowHpScale;

    if (aiPlayer->modifiers.ironBodyThorns && dist < m_tuning.ironBodyThornsRange) {
        score += m_tuning.ironBodyThornsBonus;
    }
    if (aiPlayer->modifiers.ironBodyHardened && m_currentState == AIState::CHASE) {
        score += m_tuning.ironBodyHardenedBonus;
    }
    if (m_personality == AIPersonality::RUSH) score += m_tuning.ironBodyRushBonus;

    score = qBound(m_tuning.ironBodyMinScore, score, m_tuning.ironBodyMaxScore);
    return m_dis(m_gen) < score;
}

// === 武器槽位系统 ===

bool GameAI::shouldSwitchWeapon()
{
    auto aiPlayer = m_aiPlayer.lock();
    auto targetPlayer = m_targetPlayer.lock();
    if (!aiPlayer || !targetPlayer || m_weaponSlots.isEmpty()) return false;

    // 武器切槽冷却中，不切换
    if (m_weaponSwitchCooldown > 0) {
        m_weaponSwitchCooldown--;
        return false;
    }

    // 当前槽位不可用的异常处理
    if (m_activeSlotIndex >= m_weaponSlots.size()) {
        m_activeSlotIndex = 0;
    }

    Player::WeaponType currentWeapon = m_weaponSlots[m_activeSlotIndex];
    int currentUtility = evaluateWeaponUtility(currentWeapon);

    // 遍历其他槽位，找最优武器
    int bestUtility = currentUtility;
    int bestSlotIndex = m_activeSlotIndex;

    for (int i = 0; i < m_weaponSlots.size(); i++) {
        if (i == m_activeSlotIndex) continue; // 跳过当前槽位

        Player::WeaponType weaponType = m_weaponSlots[i];
        int utility = evaluateWeaponUtility(weaponType);

        // 只有效能差异超过阈值，才切换
        if (utility > bestUtility) {
            // 计算切换的风险成本
            // 如果当前正在攻击，没有充分理由就不切 (60% 阈值)
            // 如果在闪避或移动，更容易切 (40% 阈值)
            float switchThreshold = (m_currentState == AIState::ATTACK) ? 0.60f : 0.40f;
            float utilityRatio = (float)utility / qMax(currentUtility, 1);

            if (utilityRatio > switchThreshold && m_dis(m_gen) < 0.5f) {
                // 50% 概率锁定，避免频繁切换引发的颤抖现象
                bestUtility = utility;
                bestSlotIndex = i;
            }
        }
    }

    // 如果找到了更好的武器，执行切换
    if (bestSlotIndex != m_activeSlotIndex) {
        m_activeSlotIndex = bestSlotIndex;
        m_weaponSwitchCooldown = 20; // 冷却 20 帧 (~330ms)
        return true;
    }

    return false;
}

int GameAI::evaluateWeaponUtility(Player::WeaponType w) const
{
    auto aiPlayer = m_aiPlayer.lock();
    auto targetPlayer = m_targetPlayer.lock();
    if (!aiPlayer || !targetPlayer) return 0;

    QPointF aiPos = getPlayerPosition(aiPlayer);
    QPointF targetPos = getPlayerPosition(targetPlayer);
    float dist = distance(aiPos, targetPos);
    float heightDiff = abs(targetPos.y() - aiPos.y());

    int utility = 0;

    switch (w) {
    case Player::WeaponType::punch: {
        // 近战物理：近距离高效，远距离无用
        if (dist < 80.0f) {
            utility = 70;
        } else if (dist < 150.0f) {
            utility = 40;
        } else {
            utility = 10;
        }
        // 对连锁甲无效
        if (targetPlayer->armor == Player::ArmorType::chainmail) {
            utility /= 2;
        }
        break;
    }
    case Player::WeaponType::knife: {
        // 刀：中近距离，有穿甲能力
        if (dist < 100.0f) {
            utility = 75; // 比拳头好但需要更近
        } else if (dist < 180.0f) {
            utility = 45;
        } else {
            utility = 15;
        }
        // 对连锁甲有穿甲
        break;
    }
    case Player::WeaponType::ball: {
        // 实心球：抛物线，中距离，对护甲有效，但需要弹药
        if (aiPlayer->ballCount <= 0) {
            utility = 5; // 没弹药，几乎无用
            break;
        }
        // 高度有利时效果最好
        if (heightDiff < 40.0f && dist > 80.0f && dist < 350.0f) {
            utility = 80;
        } else if (dist < 300.0f) {
            utility = 50;
        } else {
            utility = 20;
        }
        // 对护甲更有效
        if (targetPlayer->armor != Player::ArmorType::noArmor) {
            utility += 15;
        }
        break;
    }
    case Player::WeaponType::rifle: {
        // 步枪：远距离，连射，需要弹药
        if (aiPlayer->rifleCount <= 0) {
            utility = 5;
            break;
        }
        // 高度接近、距离中远时最优
        if (heightDiff < 80.0f && dist > 120.0f && dist < 400.0f) {
            utility = 75;
        } else if (dist < 500.0f) {
            utility = 50;
        } else {
            utility = 30;
        }
        break;
    }
    case Player::WeaponType::sniper: {
        // 狙击枪：极远距离，精确，需要弹药，装填慢
        if (aiPlayer->sniperCount <= 0) {
            utility = 5;
            break;
        }
        // 距离越远越好，但近距离几乎无用
        if (dist < 150.0f) {
            utility = 15; // 太近了，难以使用
        } else if (dist > 250.0f) {
            utility = 85; // 远距离狙击最佳
        } else {
            utility = 45;
        }
        // 如果目标在掩护下或持盾，效能下降
        break;
    }
    default:
        utility = 0;
        break;
    }

    // 应用状态修正系数
    // 保命状态更偏好远程武器
    if (m_currentState == AIState::SURVIVE) {
        if (w == Player::WeaponType::rifle || w == Player::WeaponType::sniper) {
            utility += 20;
        } else if (w == Player::WeaponType::punch) {
            utility -= 10;
        }
    }
    // 攻击状态偏好近程武器
    else if (m_currentState == AIState::ATTACK && dist < 150.0f) {
        if (w == Player::WeaponType::punch || w == Player::WeaponType::knife) {
            utility += 15;
        }
    }

    // 确保值在合理范围内
    return qBound(0, utility, 100);
}
