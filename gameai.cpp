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
    , m_stuckCounter(0)
    , m_lastDodgeTime(0)        // 新增初始化
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
    auto aiPlayer = m_aiPlayer.lock();
    if (!aiPlayer) return;

    qDebug() << "AI stuck detected! Executing random platform seek...";

    // 清除任何预定动作
    m_plannedMoves.clear();
    m_executingPlan = false;

    // 获取当前所在平台
    QPointF aiPos = getPlayerPosition(aiPlayer);
    int currentPlatform = Map::getInstance().checkPlatform(aiPos);
    
    // 获取所有平台（碰撞箱）
    const auto& allPlatforms = Map::getInstance().getCollisionBoxes();
    
    if (allPlatforms.isEmpty()) {
        m_stuckCounter = 0;
        return;
    }

    // 优先选择“更远”的平台，增加脱困成功率
    int targetPlatformIdx = -1;
    float bestScore = -1.f;
    for (int i = 0; i < allPlatforms.size(); ++i) {
        if (i == currentPlatform) continue;
        const QRect& r = allPlatforms[i];
        if (r.height() < 10 || r.width() < 80) continue;

        float centerX = r.center().x();
        float dx = std::abs(centerX - aiPos.x());
        float score = dx + static_cast<float>(m_dis(m_gen)) * 120.f;

        // 偏好更远的平台，至少拉开一定距离
        if (dx >= 220.f) score += 180.f;

        if (score > bestScore) {
            bestScore = score;
            targetPlatformIdx = i;
        }
    }

    if (targetPlatformIdx < 0) {
        // 无法找到合适的不同平台，执行简单救急动作
        QVector<MoveIntent> emergencyMoves;
        if (m_dis(m_gen) < 0.5) {
            emergencyMoves.append(MoveIntent::JUMP);
            emergencyMoves.append(MoveIntent::MOVING_LEFT);
        } else {
            emergencyMoves.append(MoveIntent::JUMP);
            emergencyMoves.append(MoveIntent::MOVING_RIGHT);
        }
        planMoveSequence(emergencyMoves);
        moveIntent = emergencyMoves[0];
    } else {
        // 在目标平台上选择一个随机点
        const QRect& targetRect = allPlatforms[targetPlatformIdx];
        int randomX = targetRect.left() + (m_gen() % (targetRect.width() > 0 ? targetRect.width() : 1));
        QPointF targetPoint(randomX, targetRect.top() - 20);

        // 记录一个持续生效的脱困目标，防止下一帧又改回追人导致抖动
        m_stuckEscapeTarget = targetPoint;
        m_stuckEscapeTimer = 90;
        executeMoveTo(m_stuckEscapeTarget, moveIntent, 30.0f);
    }

    m_stuckCounter = 0;  // 重置卡住计数器
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

    // 状态机执行
    switch (m_currentState) {
    case AIState::FOLLOW:
        executeFollow(moveIntent, attackIntent);
        break;
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
    case AIState::SEEK_SUPPLY:
        executeSeekSupply(moveIntent, attackIntent);
        break;

    default:
        break;
    }

    // 状态转换（目前只有FOLLOW状态）
    AIState nextState = determineNextState();
    if (nextState != m_currentState) {
        m_currentState = nextState;
    }

    // 状态决策后再判断施法，避免与移动/攻击意图冲突。
    updateSpellIntent();
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

void GameAI::processAI()
{
    // 这里可以做一些周期性的状态更新
    // 目前暂时留空，主要逻辑在updateIntent中
}

// === 当前实现：跟随逻辑 ===

void GameAI::executeFollow(MoveIntent& moveIntent, AttackIntent& attackIntent)
{
    // ── 施法后强制攻击模式 ──────────────────────────────────
    if (m_forcedAttackAfterSpell) {
        m_forcedAttackTimer--;
        if (m_forcedAttackTimer <= 0) {
            m_forcedAttackAfterSpell = false;  // 强制模式结束
        } else {
            // 在强制模式中：靠近并持续攻击
            QPointF aiPos = getPlayerPosition(m_aiPlayer.lock());
            QPointF targetPos = getPlayerPosition(m_targetPlayer.lock());
            float dist = distance(aiPos, targetPos);

            if (dist > 50.f) {
                // 还没靠近，继续寻路
                my_platform = Map::getInstance().checkPlatform(aiPos);
                next_platform = Map::getInstance().findPath(aiPos, targetPos);

                if (next_platform == -1 && my_platform != -1) {
                    moveIntent = MoveIntent::NONE;
                    attackIntent = true;
                    return;
                }

                if (my_platform == -1) {
                    moveIntent = calculateMoveDirection(next_target, false);
                } else {
                    if (my_platform == next_platform) {
                        moveIntent = calculateMoveDirection(targetPos);
                    } else {
                        next_target = Map::getInstance().findTarget(aiPos, next_platform);
                        moveIntent = calculateMoveDirection(next_target);
                    }
                }
            } else {
                // 已靠近，准备攻击
                moveIntent = MoveIntent::NONE;
            }
            attackIntent = true;  // 持续攻击意图
            return;
        }
    }

    // ── 脱困目标优先：短时间内持续向远平台移动，降低意图晃动 ───
    if (m_stuckEscapeTimer > 0) {
        m_stuckEscapeTimer--;
        QPointF aiPos = getPlayerPosition(m_aiPlayer.lock());
        if (distance(aiPos, m_stuckEscapeTarget) <= 45.f) {
            m_stuckEscapeTimer = 0;
        } else {
            executeMoveTo(m_stuckEscapeTarget, moveIntent, 30.0f);
            attackIntent = false;
            return;
        }
    }

    // ── 正常跟随逻辑 ──────────────────────────────────────
    if (!shouldMoveToTarget()) {
        return; // 距离合适，不需要移动
    }

    QPointF aiPos = getPlayerPosition(m_aiPlayer.lock());
    my_platform = Map::getInstance().checkPlatform(aiPos);

    // ── Stuck 检测与随机寻路 ──────────────────────────────
    if (isStuck()) {
        handleStuckSituation(moveIntent);
        return;
    }

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
    if (state == AIState::ATTACK){
        return "ATTACK";
    }
    else if (state == AIState::FOLLOW){
        return "FOLLOW";
    }
    else if (state == AIState::RETREAT){
        return "RETREAT";
    }
    else if (state == AIState::SEEK_WEAPON){
        return "SEEK_WEAPON";
    }
    else if (state == AIState::SEEK_SUPPLY){
        return "SEEK_SUPPLY";
    }
    else if (state == AIState::SEEK_ARMOR){
        return "SEEK_ARMOR";
    }
    else{
        return "NONE";
    }
}

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
    auto aiPlayer = m_aiPlayer.lock();
    auto targetPlayer = m_targetPlayer.lock();
    if (!aiPlayer || !targetPlayer) return AIState::FOLLOW;

    const bool endlessMode = aiPlayer->endlessImmortal;

    if (endlessMode) {
        // 无尽模式下，AI不再过度偏防守：优先找武器和攻击，只有明显威胁时才闪避
        if (shouldSeekWeapon()) {
            return AIState::SEEK_WEAPON;
        }
        if (shouldSeekArmor()) {
            return AIState::SEEK_ARMOR;
        }
        if (shouldSeekSupply()) {
            return AIState::SEEK_SUPPLY;
        }
        if (shouldDodge()) {
            return AIState::DODGE;
        }
        return AIState::ATTACK;
    }

    QPointF aiPos = getPlayerPosition(aiPlayer);
    QPointF targetPos = getPlayerPosition(targetPlayer);
    float dist = distance(aiPos, targetPos);
    float healthRatio = (float)aiPlayer->hp / aiPlayer->getMaxHp();

    // 血量充足时的激进策略
    if (healthRatio > 0.6f) { // 优先拾取武器
        if (shouldSeekWeapon()){
            return AIState::SEEK_WEAPON;
        }
        if (shouldSeekArmor()) {
            return AIState::SEEK_ARMOR;
        }
        if (targetPlayer->armor == Player::ArmorType::chainmail && aiPlayer->weapon == Player::WeaponType::punch){
            // 无法对敌人造成伤害则放弃进攻
            return AIState::FOLLOW;
        }
        return AIState::ATTACK;
    }

    // 血量中等时的谨慎策略
    else if (healthRatio > 0.3f) { // 优先找护甲
        if (shouldDodge()){
            return AIState::DODGE;
        }
        if (shouldSeekArmor()) {
            return AIState::SEEK_ARMOR;
        }
        if (shouldSeekWeapon()){
            return AIState::SEEK_WEAPON;
        }
        if (shouldSeekSupply()){ // 优先找补给
            return AIState::SEEK_SUPPLY;
        }
        if (targetPlayer->armor == Player::ArmorType::chainmail && aiPlayer->weapon == Player::WeaponType::punch){
            // 无法对敌人造成伤害则放弃进攻
            return AIState::RETREAT;
        }
        return AIState::ATTACK;
    }

    // 血量危险时的保守策略
    else {
        if (shouldDodge()){
            return AIState::DODGE;
        }
        if (shouldSeekSupply()){ // 优先找补给
            return AIState::SEEK_SUPPLY;
        }
        if (targetPlayer->armor == Player::ArmorType::chainmail && aiPlayer->weapon == Player::WeaponType::punch){
            // 无法对敌人造成伤害则放弃进攻
            return AIState::RETREAT;
        }
        return AIState::ATTACK;
    }
}

void GameAI::executeSeekWeapon(MoveIntent& moveIntent, AttackIntent& attackIntent)
{
    QPointF weaponPos = findBestWeapon();
    if (weaponPos == QPointF()) {
        executeFollow(moveIntent, attackIntent);
        return;
    }

    QPointF aiPos = getPlayerPosition(m_aiPlayer.lock());
    float distToWeaponX = abs(aiPos.x() - weaponPos.x());
    float distToWeapon = distance(aiPos, weaponPos);

    // 如果很接近武器，就蹲下拾取
    if (distToWeaponX < 20.0f && distToWeapon < 100.0f) {
        moveIntent = MoveIntent::CROUCH;
        attackIntent = false;
        return;
    }

    // 使用通用寻路函数前往武器位置
    executeMoveTo(weaponPos, moveIntent, 20.0f);
    attackIntent = false;
}

void GameAI::executeSeekArmor(MoveIntent& moveIntent, AttackIntent& attackIntent) {
    QPointF armorPos = findBestArmor();
    if (armorPos == QPointF()) {
        executeFollow(moveIntent, attackIntent);
        return;
    }

    QPointF aiPos = getPlayerPosition(m_aiPlayer.lock());
    float distToWeaponX = abs(aiPos.x() - armorPos.x());
    float distToWeapon = distance(aiPos, armorPos);

    // 如果很接近武器，就蹲下拾取
    if (distToWeaponX < 20.0f && distToWeapon < 100.0f) {
        moveIntent = MoveIntent::CROUCH;
        attackIntent = false;
        return;
    }

    // 使用通用寻路函数前往武器位置
    executeMoveTo(armorPos, moveIntent, 20.0f);
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
bool GameAI::shouldSeekWeapon()
{
    auto aiPlayer = m_aiPlayer.lock();
    if (!aiPlayer) return false;

    // 血量很低时优先保命
    float healthRatio = (float)aiPlayer->hp / aiPlayer->getMaxHp();
    if (healthRatio < 0.25f) return false;

    // 获取当前武器优先级
    int currentPriority = getWeaponPriority(m_currentWeapon.isEmpty() ? "punch" : m_currentWeapon);

    // 检查是否有更好的武器，有则拾取（武器带来的优势很大）
    for (const auto& drop : m_availableDrops) {
        int dropPriority = getWeaponPriority(drop.itemType);
        if (dropPriority > currentPriority) {
            return true;
        }
    }

    return false;
}

QPointF GameAI::findBestWeapon()
{
    auto aiPlayer = m_aiPlayer.lock();
    if (!aiPlayer) return QPointF();

    QPointF aiPos = getPlayerPosition(aiPlayer);
    int currentPriority = getWeaponPriority(m_currentWeapon.isEmpty() ? "punch" : m_currentWeapon);

    QPointF bestWeaponPos;
    int bestPriority = currentPriority;
    float bestScore = -1.0f;

    for (const auto& drop : m_availableDrops) {
        int dropPriority = getWeaponPriority(drop.itemType);
        float dropDistance = distance(aiPos, drop.position);

        // 只考虑更好的武器
        if (dropPriority > currentPriority && dropDistance < 500.0f) {
            // 评分：优先级差异 - 距离惩罚
            float score = (dropPriority - currentPriority);

            if (score > bestScore) {
                bestWeaponPos = drop.position;
                bestScore = score;
            }
        }
    }

    return bestWeaponPos;
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

void GameAI::executeSeekSupply(MoveIntent& moveIntent, AttackIntent& attackIntent){
    QPointF supplyPos = findBestSupply();
    if (supplyPos == QPointF()) {
        executeFollow(moveIntent, attackIntent);
        return;
    }

    QPointF aiPos = getPlayerPosition(m_aiPlayer.lock());
    float distToWeaponX = abs(aiPos.x() - supplyPos.x());
    float distToWeapon = distance(aiPos, supplyPos);

    // 如果很接近武器，就蹲下拾取
    if (distToWeaponX < 20.0f && distToWeapon < 100.0f) {
        moveIntent = MoveIntent::CROUCH;
        attackIntent = false;
        return;
    }

    executeMoveTo(supplyPos, moveIntent, 20.0f);
    attackIntent = false;

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
bool GameAI::shouldSeekArmor() {
    auto aiPlayer = m_aiPlayer.lock();
    if (!aiPlayer) return false;

    // 血量很低时优先保命
    float healthRatio = (float)aiPlayer->hp / aiPlayer->getMaxHp();
    if (healthRatio < 0.15f) return false;

    Player::WeaponType opponentWeapon = m_targetPlayer.lock()->weapon;
    int desiredArmor = 0;
    if (opponentWeapon == Player::WeaponType::punch || opponentWeapon == Player::WeaponType::knife){
        desiredArmor = 1;
    }
    else if (opponentWeapon == Player::WeaponType::sniper || opponentWeapon == Player::WeaponType::rifle){
        desiredArmor = 2;
    }

    if (!desiredArmor) return false; // 对手的武器无法用护甲防御，放弃

    for (const auto& drop : m_availableDrops) {
        if (drop.itemType == "chainmail" && desiredArmor == 1){
            return true;
        }
        else if (drop.itemType == "vest" && desiredArmor == 2){
            return true;
        }
    }

    return false; // 没有想要的护甲，放弃
}
bool GameAI::shouldSeekSupply() {
    auto aiPlayer = m_aiPlayer.lock();
    if (!aiPlayer) return false;

    // 血量很低时优先保命
    float healthRatio = (float)aiPlayer->hp / aiPlayer->getMaxHp();
    if (healthRatio > 0.5f) return false;
    if (m_targetPlayer.lock()->hp < 20){
        return false; // 对手也快没血了，激进策略
    }

    for (const auto& drop : m_availableDrops) {
        if (drop.itemType == "bandage" || drop.itemType == "medkit" || drop.itemType == "adrenaline"){
            return true;
        }
    }

   return false; // 没有补给掉落

}
QPointF GameAI::findBestArmor() {
    auto aiPlayer = m_aiPlayer.lock();
    if (!aiPlayer) return QPointF();
    Player::WeaponType opponentWeapon = m_targetPlayer.lock()->weapon;
    int desiredArmor = 0;
    if (opponentWeapon == Player::WeaponType::punch || opponentWeapon == Player::WeaponType::knife){
        desiredArmor = 1;
    }
    else if (opponentWeapon == Player::WeaponType::sniper || opponentWeapon == Player::WeaponType::rifle){
        desiredArmor = 2;
    }

    if (!desiredArmor) return QPointF(); // 对手的武器无法用护甲防御，放弃

    for (const auto& drop : m_availableDrops) {
        if (drop.itemType == "chainmail" && desiredArmor == 1){
            return drop.position;
        }
        else if (drop.itemType == "vest" && desiredArmor == 2){
            return drop.position;
        }
    }

    return QPointF(); // 没有想要的护甲，放弃
}
QPointF GameAI::findBestSupply() {
    auto aiPlayer = m_aiPlayer.lock();
    if (!aiPlayer) return QPointF();

    float healthRatio = (float)aiPlayer->hp / aiPlayer->getMaxHp();

    if (healthRatio > 0.6){
        return QPointF(); // 血量很充足，先作战
    }

    int bestType = 0;
    QPointF bestPos = QPointF();

    for (const auto& drop : m_availableDrops) {
        if (drop.itemType == "bandage"){
            if (1 > bestType){
                bestType = 1;
                bestPos = drop.position;
            }
        }
        else if (drop.itemType == "medkit"){
            if (2 > bestType){
                bestType = 2;
                bestPos = drop.position;
            }
        }
        else if (drop.itemType == "adrenaline" && healthRatio > 0.3){
            if (3 > bestType){
                bestType = 3;
                bestPos = drop.position;
            }
        }
    }

    return bestPos; // 没有想要的护甲，放弃
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

int GameAI::getWeaponStrength(Player::WeaponType weapon) const
{
    // 武器优先级（较量强度）
    switch (weapon) {
    case Player::WeaponType::sniper:  return 5;
    case Player::WeaponType::rifle:   return 4;
    case Player::WeaponType::ball:    return 3;
    case Player::WeaponType::knife:   return 2;
    case Player::WeaponType::punch:   return 1;
    default: return 0;
    }
}

void GameAI::updateSpellIntent()
{
    m_castSpellIntent = false;

    if (m_aiSpell == GameSession::Spell::FREEZE && shouldCastFreeze()) {
        m_castSpellIntent = true;
        m_forcedAttackAfterSpell = true;  // 施法后进入强制攻击模式
        m_forcedAttackTimer = FORCED_ATTACK_DURATION;
    }
}

bool GameAI::shouldCastFreeze()
{
    auto aiPlayer = m_aiPlayer.lock();
    auto targetPlayer = m_targetPlayer.lock();
    if (!aiPlayer || !targetPlayer) return false;

    // 基本条件检查
    if (aiPlayer->spellState.isOnCooldown()) return false;  // CD未好
    if (aiPlayer->spellState.isFrozen) return false;         // 自己已冻结
    if (targetPlayer->spellState.isFrozen) return false;     // 对手已冻结
    if (!m_targetVisible) return false;                      // 看不见对手

    // 距离检查
    QPointF aiPos = getPlayerPosition(aiPlayer);
    QPointF targetPos = getPlayerPosition(targetPlayer);
    float dist = distance(aiPos, targetPos);
    float dy = std::abs(targetPos.y() - aiPos.y());

    if (dist < 70.f || dist > 300.f) return false;  // 距离窗口
    if (dy > 90.f) return false;                     // 同层检查

    // 简化：基于相对武器强度的概率
    int aiWeaponStrength = getWeaponStrength(aiPlayer->weapon);
    int targetWeaponStrength = getWeaponStrength(targetPlayer->weapon);

    float castProbability = 0.0f;
    if (aiWeaponStrength >= targetWeaponStrength) {
        // 自己武器不差，每帧有1%概率释放
        castProbability = 0.01f;
    } else {
        // 自己武器较差，每帧有0.5%概率释放
        castProbability = 0.005f;
    }

    return m_dis(m_gen) < castProbability;
}
