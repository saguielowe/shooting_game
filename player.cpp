#include "player.h"
#include <QDebug>
#include <QtMath>
Player::Player(float x, float y, int id,
               float initMaxHp, int initMaxBalls,
               int initMaxBullets, int initMaxSnipers,
               bool initCanHide, float initSpeedRatio)
    : id(id)
    , x(x), y(y), dt(0)
    , hp(initMaxHp)
    , maxHp(initMaxHp)
    , maxBalls(initMaxBalls)
    , maxBullets(initMaxBullets)
    , maxSnipers(initMaxSnipers)
    , canHide(initCanHide)
    , velocityratio(initSpeedRatio)
    , selfvelocityratio(1.0f)
    , ballCount(0)
    , rifleCount(0)
    , sniperCount(0)
    , hpRegenerate(false)
    , regenerateTime(0)
    , direction(1)
    , weapon(WeaponType::punch)
    , armor(ArmorType::noArmor)
    , vestHardness(100)
{
    qDebug()<<"loading animation assets……";
    animation.load("idle", ":/assets/assets/FreeKnight_v1/Colour1/Outline/120x80_PNGSheets/_Idle.png", 120, 10);
    animation.load("run", ":/assets/assets/FreeKnight_v1/Colour1/Outline/120x80_PNGSheets/_Run.png", 120, 10);
    animation.load("jump", ":/assets/assets/FreeKnight_v1/Colour1/Outline/120x80_PNGSheets/_Jump.png", 120, 3);
    animation.load("crouch", ":/assets/assets/FreeKnight_v1/Colour1/Outline/120x80_PNGSheets/_Crouch.png", 120, 1);
    animation.load("fall", ":/assets/assets/FreeKnight_v1/Colour1/Outline/120x80_PNGSheets/_Fall.png", 120, 3);
    animation.load("hurt", ":/assets/assets/FreeKnight_v1/Colour1/Outline/120x80_PNGSheets/_SlideFull.png", 120, 4);
    animation.load("knife", ":/assets/assets/FreeKnight_v1/Colour1/Outline/120x80_PNGSheets/_Attack2NoMovement.png", 120, 6);
    animation.load("punch", ":/assets/assets/FreeKnight_v1/Colour1/Outline/120x80_PNGSheets/_punch.png", 120, 2);
    animation.load("gun", ":/assets/assets/FreeKnight_v1/Colour1/Outline/120x80_PNGSheets/_gun.png", 120, 2);

    animation.play("idle");
    weaponSlots.append(WeaponType::punch); // 初始武器放进槽位
}
void Player::setDt(float t){
    dt = t;
}

void Player::applyGravity(){
    if (!onGround){
        vy += gravity * dt;
    }
}

void Player::crouch(){
    vx = 0;
}

void Player::stop(){
    const float resistanceUsed = onGround == true ? resistance : airResistance;
    if (vx > 0){
        vx = fmax(0, vx - resistanceUsed);
    }
    else if (vx < 0){
        vx = fmin(0, vx + resistanceUsed);
    }
}

void Player::moveLeft(){
    if (vx > 0){ // 如果人物在向右，给它快速拉回左。
        vx -= 4 * acceleration;
    }
    else{
        vx = fmax(-moveSpeed * velocityratio * selfvelocityratio, vx - acceleration); // 往左不能快于moveSpeed
    }
}

void Player::moveRight(){
    if (vx < 0){
        vx += 4 * acceleration;
    }
    else{
        vx = fmin(moveSpeed * velocityratio * selfvelocityratio, vx + acceleration);
    }
}

void Player::update(){ // player更新自身位置，animation更新自身动画
    dt = fmin(dt, 0.033f); // 防止帧率突降时角色穿透，最多移动1/30s的距离
    if (hpRegenerate){
        if (regenerateTime > 0){
            regenerateTime -= dt;
            hp = fmin(maxHp, hp + 3 * dt);
        }
        else{
            hpRegenerate = false;
            regenerateTime = 0;
            selfvelocityratio = 1;
        }
    }
    if (vx > 0 && direction == false){ // 只有当速度方向与人物朝向不一致时才更改朝向
        direction = true;
    }
    else if (vx < 0 && direction == true){
        direction = false;
    }
    bool loop = true;
    if (state.moveState == "hurt"){
        animation.play("hurt");
        loop = false;
    }
    if (state.shootState){
        if (weapon == WeaponType::knife){
            animation.play("knife", true);
        }
        else if (weapon == WeaponType::punch){
            animation.play("punch", true);
        }
        else if (weapon == WeaponType::rifle || weapon == WeaponType::sniper){
            animation.play("gun", true);
        }
    }
    if (state.moveState == "stop" && vx == 0){
        state.moveState = "null"; // 如果减速至0则变为静止
    }
    if (state.moveState == "jump" && vy > 0 && onGround == false){
        state.moveState = "fall";
    }

    if (state.moveState == "jump" && onGround){ // 在地面才给推一把
        vy = - jumpSpeed;
        animation.play("jump");
    }
    if (state.moveState == "fall"){
        animation.play("fall");
    }
    if (state.moveState == "crouch"){
        crouch();
        animation.play("crouch");
    }
    if (state.moveState == "moving_left"){
        moveLeft();
        animation.play("run");
    }
    if (state.moveState == "moving_right"){ // 这里的right和实际运动方向无关，只是加速度方向
        moveRight();
        animation.play("run");
    }
    if (state.moveState == "stop"){
        stop();
        animation.play("run");
    }
    if (state.moveState == "null"){
        animation.play("idle");
    }
    y += vy * dt;
    x += vx * dt;

    animation.update(loop);
    hitbox = animation.getTargetRect();
}

void Player::draw(QPainter& painter) {
    if (canHide && state.moveState == "crouch") return;

    // ── 隐身：整体半透明 ─────────────────────────────────────
    // 在 painter 上设置透明度，draw完再恢复
    bool isStealthed = spellState.stealthActive;
    if (isStealthed) {
        painter.save();
        painter.setOpacity(0.35);  // 35%不透明度，能看到轮廓但明显变淡
    }

    // ── 原有武器加载逻辑（不变）─────────────────────────────
    if (weapon == WeaponType::knife)       animation.loadWeapon("knife");
    else if (weapon == WeaponType::ball)   animation.loadWeapon("ball");
    else if (weapon == WeaponType::rifle)  animation.loadWeapon("rifle");
    else if (weapon == WeaponType::sniper) animation.loadWeapon("sniper");
    else if (weapon == WeaponType::punch)  animation.loadWeapon("no_weapon");

    // ── 铜头铁臂：人物金属化叠色（不画矩形）──────────────────
    bool isIronBody = spellState.ironBodyActive;
    if (isIronBody) {
        painter.save();
    }

    // ── 原有动画绘制（不变）─────────────────────────────────
    bool armed = (state.moveState == "null") && (!state.shootState);
    if (direction)
        animation.draw(painter, x, y, false, armed);
    else
        animation.draw(painter, x, y, true, armed);

    if (isIronBody) {
        float pulse = 0.65f + 0.35f * qSin(spellState.ironBodyRemain * 12.f);
        QColor metalTint(255, 200, 90, int(70 + 80 * pulse));
        painter.setCompositionMode(QPainter::CompositionMode_Screen);
        painter.setOpacity(0.75);
        if (direction)
            animation.draw(painter, x, y, false, armed);
        else
            animation.draw(painter, x, y, true, armed);
        painter.restore();
    }

    // ── 隐身恢复透明度 ───────────────────────────────────────
    if (isStealthed) {
        painter.restore();
        // restore之后再画"隐身中"淡紫色轮廓，不受透明度影响
        painter.save();
        QPen glowPen(QColor(180, 100, 255, 120), 2, Qt::DashLine);
        painter.setPen(glowPen);
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(hitbox);
        painter.restore();
    }

    // ── 定身：冰蓝色光晕（画在人物周围）────────────────────
    if (spellState.isFrozen) {
        painter.save();

        // 外圈光晕：大一圈的矩形，半透明冰蓝
        QRect glowRect = hitbox.adjusted(-6, -6, 6, 6);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(100, 200, 255, 60));
        painter.drawRoundedRect(glowRect, 8, 8);

        // 内圈边框：实线冰蓝
        QPen icePen(QColor(140, 220, 255, 200), 2);
        painter.setPen(icePen);
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(hitbox.adjusted(-3, -3, 3, 3), 5, 5);

        // 定身剩余时间（显示在头顶血条上方）
        painter.setPen(QColor(140, 220, 255));
        QFont f = painter.font();
        f.setPixelSize(11);
        painter.setFont(f);
        painter.drawText(
            hitbox.left(), hitbox.top() - 55,
            QString("冰冻 %1s").arg(spellState.frozenRemain, 0, 'f', 1)
            );

        painter.restore();
    }

    // ── 原有血条逻辑（不变）─────────────────────────────────
    if (!endlessImmortal){
    QRect backgroundRect(hitbox.left(), hitbox.top() - 20, 100, 15);
    painter.setBrush(Qt::gray);
    painter.drawRect(backgroundRect);

    int barWidth = static_cast<int>(100 * (fmax(0, hp) / maxHp));
    QRect healthRect(hitbox.left(), hitbox.top() - 20, barWidth, 15);
    painter.setBrush(Qt::red);
    painter.drawRect(healthRect);
    painter.drawText(healthRect, QString::number(int(hp)));
    painter.drawText(hitbox.left() - 10, hitbox.top(), (id ? "右" : "左"));
    }

    if (armor == ArmorType::noArmor) return;

    QRect backgroundRect2(hitbox.left(), hitbox.top() - 40, 100, 15);
    painter.setBrush(Qt::gray);
    if (armor == ArmorType::chainmail) {
        painter.setBrush(Qt::white);
        painter.drawRect(hitbox.left(), hitbox.top() - 40, 100, 15);
    }
    else if (armor == ArmorType::vest) {
        painter.setBrush(QColor(165, 42, 42));
        painter.drawRect(hitbox.left(), hitbox.top() - 40, int(vestHardness), 15);
    }
}

bool Player::isDead() const{
    if (endlessImmortal) return false;
    return hp<=0 ;
}

void Player::applyModifier(const ModifierData& mod) {
    switch (mod.type) {

    // ── 通用词条 ──────────────────────────────────────────
    case ModifierType::SPELL_COOLDOWN_REDUCE:
        modifiers.spellCooldownReduce += 5;
        spellState.cooldownMax    = qMax(5.f, spellState.cooldownMax - 5.f);    // 总CD缩短
        spellState.cooldownRemain = qMax(0.f, spellState.cooldownRemain - 5.f); // 当前剩余也立即缩短
        break;

    case ModifierType::MAX_HP_UP:
        modifiers.maxHpMultiplier += 0.2f;
        maxHp = maxHp * modifiers.maxHpMultiplier;
        hp *= modifiers.maxHpMultiplier; // 血条上限与现有血量同步增加
        break;

    case ModifierType::MOVE_SPEED_UP:
        modifiers.moveSpeedMultiplier += 0.2f;
        selfvelocityratio = velocityratio * modifiers.moveSpeedMultiplier;
        break;

    case ModifierType::PUNCH_DAMAGE_UP:
        modifiers.punchDmgMultiplier += 0.8f; // +100%
        break;

    case ModifierType::KNIFE_DAMAGE_UP:
        modifiers.knifeDmgMultiplier += 0.4f;
        break;

    case ModifierType::BALL_DAMAGE_UP:
        modifiers.ballDmgMultiplier += 0.2f;
        break;

    case ModifierType::GUN_DAMAGE_UP:
        modifiers.gunDmgMultiplier += 0.3f;
        break;

    case ModifierType::DAMAGE_BONUS_UP:
        modifiers.damageBonusMultiplier += 0.25f;
        break;

    case ModifierType::DAMAGE_REDUCTION_UP:
        modifiers.damageReduction = qMin(0.6f, modifiers.damageReduction + 0.2f);
        break;

    case ModifierType::DOUBLE_EDGE:
        modifiers.doubleEdge = true;
        break;

    case ModifierType::EXTRA_WEAPON_SLOT:
        modifiers.extraWeaponSlots++;
        updateSlots(); // 直接更新槽位，给予新武器
        break;

    // ── 定身词条 ──────────────────────────────────────────
    case ModifierType::FREEZE_DURATION_UP:
        modifiers.freezeDurationBonus += 1.5f;
        break;
    case ModifierType::FREEZE_DAMAGE_UP:
        modifiers.freezeDmgMultiplier += 0.2f;
        break;
    case ModifierType::FREEZE_BREAK_CDR:
        modifiers.freezeBreakCDR = true;
        break;

    // ── 隐身词条 ──────────────────────────────────────────
    case ModifierType::STEALTH_DURATION_UP:
        modifiers.stealthDurationBonus += 2.f;
        break;
    case ModifierType::STEALTH_SPEED_UP:
        modifiers.stealthSpeedMultiplier += 0.2f;
        break;
    case ModifierType::STEALTH_REGEN:
        modifiers.stealthRegenPerSec += 2.f;
        break;
    case ModifierType::STEALTH_FIRST_HIT:
        modifiers.stealthFirstHit = true;
        break;

    // ── 安身词条 ──────────────────────────────────────────
    case ModifierType::BARRIER_DURATION_UP:
        modifiers.barrierDurationBonus += 5.f;
        break;
    case ModifierType::BARRIER_REGEN_UP:
        modifiers.barrierRegenPerSec += 2.f;
        break;
    case ModifierType::BARRIER_REDUCTION_UP:
        modifiers.barrierDmgReduction = qMin(1.0f, modifiers.barrierDmgReduction + 0.2f);
        break;

    // ── 铜头铁臂词条 ────────────────────────────────────────
    case ModifierType::IRON_BODY_DURATION_UP:
        modifiers.ironBodyDurationBonus += 1.f;
        break;
    case ModifierType::IRON_BODY_REFLECT_CDR:
        modifiers.ironBodyReflectCDR = true;
        break;
    case ModifierType::IRON_BODY_THORNS:
        modifiers.ironBodyThorns = true;
        break;
    case ModifierType::IRON_BODY_HARDENED:
        modifiers.ironBodyHardened = true;
        break;

    // ── 身外身词条 ────────────────────────────────────────
    case ModifierType::CLONE_CAN_CAST_SPELL:
        modifiers.cloneCanCastSpell = true;
        break;
    case ModifierType::CLONE_DAMAGE_UP:
        modifiers.cloneDmgMultiplier += 0.2f;
        break;
    case ModifierType::CLONE_DURATION_UP:
        modifiers.cloneDurationBonus += 5.f;
        break;
    case ModifierType::CLONE_DAMAGE_EXTEND:
        modifiers.cloneDamageExtend = true;
        break;

    // ── 禁字法 ────────────────────────────────────────────
    case ModifierType::FORBIDDEN_WORD:
        modifiers.forbiddenWord = true;
        modifiers.damageBonusMultiplier += 0.5f; // +100%
        break;

    default:
        break;
    }
}

void Player::applyEnemyModifier(const ModifierData& mod) {
    // 对手词条：减少我的属性
    switch (mod.type) {
    case ModifierType::ENEMY_MAX_HP_DOWN:
        modifiers.maxHpMultiplier = qMax(0.2f, modifiers.maxHpMultiplier - 0.2f);
        maxHp = maxHp * modifiers.maxHpMultiplier;
        hp = qMin(hp, maxHp);
        break;
    case ModifierType::ENEMY_MOVE_SPEED_DOWN:
        modifiers.moveSpeedMultiplier = qMax(0.3f, modifiers.moveSpeedMultiplier - 0.15f);
        selfvelocityratio = velocityratio * modifiers.moveSpeedMultiplier;
        break;
    default:
        break;
    }
}

// ── getAttackDamage() : 处理武器的基础攻击力，以及与攻击方相关的所有伤害加成 ─────────────────────────────
float Player::getAttackDamage(){
    float base = 0.f;
    switch (weapon) {
    case WeaponType::punch:  base = 5.f * modifiers.punchDmgMultiplier; break;
    case WeaponType::knife:  base = 15.f * modifiers.knifeDmgMultiplier; break;
    case WeaponType::ball:   base = 1.f * modifiers.ballDmgMultiplier;  break;
    case WeaponType::rifle:  base = 1.f * modifiers.gunDmgMultiplier;   break;
    case WeaponType::sniper: base = 1.f * modifiers.gunDmgMultiplier;   break;
    }
    base *= modifiers.damageBonusMultiplier;
    if (modifiers.doubleEdge) base *= 1.5f;
    if (spellState.stealthActive && modifiers.stealthFirstHit && !spellState.stealthFirstHitUsed) {
        // 隐身总时长
        float totalDuration = 15.f + modifiers.stealthDurationBonus;
        if (modifiers.stealthFirstHit) totalDuration /= 2.f;

        // 已经过了多少时间
        float elapsed = totalDuration - spellState.stealthRemain;
        float ratio   = qMin(elapsed / totalDuration, 1.0f);

        // 线性从1.0增长到2.0
        float bonus = 1.0f + ratio;
        base *= bonus; // combat manager计算伤害时会调用此函数，触发后会被标记为已用
        spellState.stealthFirstHitUsed = true;
        spellState.stealthActive = false; // 破影一击触发后隐身效果消失
        spellState.stealthRemain = 0;
    }
    // 法术加成由各法术在激活时额外乘，这里不处理
    qDebug()<<"player's basic attack damage (weapon specific):"<<base;
    return base;
}

// ── getDefenseMultiplier() 修改版 ────────────────────────────
float Player::getDefenseMultiplier(WeaponType weaponType) const {

    float totalReduction = qMin(1.0f, modifiers.damageReduction);
    float multiplier = 1.0f - totalReduction;
    if (modifiers.doubleEdge) multiplier *= 1.5f;
    return multiplier;
}

void Player::switchWeaponSlot() {
    if (weaponSlots.size() <= 1) return;
    activeSlotIndex = (activeSlotIndex + 1) % weaponSlots.size();
    weapon = weaponSlots[activeSlotIndex];
}

void Player::weaponControll(QString type){
    if (type == "knife"){
        pickupWeapon(WeaponType::knife);
    }
    else if (type == "ball"){
        ballCount = maxBalls;
        pickupWeapon(WeaponType::ball);
    }
    else if (type == "bandage"){
        hp = fmin(hp + 30, maxHp);
    }
    else if (type == "rifle"){
        pickupWeapon(WeaponType::rifle);
        rifleCount = maxBullets;
    }
    else if (type == "medkit"){
        hp = maxHp;
    }
    else if (type == "sniper"){
        pickupWeapon(WeaponType::sniper);
        sniperCount = maxSnipers;
    }
    else if (type == "adrenaline"){
        selfvelocityratio = 1.2;
        hpRegenerate = true;
        regenerateTime = 20;
    }
    else if (type == "chainmail"){
        armor = ArmorType::chainmail;
    }
    else if (type == "vest"){
        armor = ArmorType::vest;
        vestHardness = 100;
    }
}
bool Player::pickupWeapon(WeaponType w) {
    // 已有此武器，不重复捡
    if (weaponSlots.contains(w)) return false;
 
    if (weaponSlots.size() < maxWeaponSlots()) {
        // 有空槽，直接加入
        weaponSlots.append(w);
    } else {
        // 无空槽，替换当前槽
        weaponSlots[activeSlotIndex] = w;
    }
    // 激活刚捡的武器
    activeSlotIndex = weaponSlots.indexOf(w);
    weapon = w;
    return true;
}

void Player::updateSlots(){
    if (maxWeaponSlots() >= 2){
        if (!weaponSlots.contains(WeaponType::punch)){
            weaponSlots.append(WeaponType::punch); // 有多槽，给予保底攻击手段
        }
    }
    if (ballCount <= 0 && weaponSlots.contains(WeaponType::ball)){
        weaponSlots.removeAll(WeaponType::ball);
        if (weaponSlots.size() == 0) weaponSlots.append(Player::WeaponType::punch); // 先填补一个攻击手段回去
        activeSlotIndex = fmax(activeSlotIndex - 1, 0);
        weapon = weaponSlots[activeSlotIndex]; // 切换到剩余槽位的武器
    }
    if (rifleCount <= 0 && weaponSlots.contains(WeaponType::rifle)){
        weaponSlots.removeAll(WeaponType::rifle);
        if (weaponSlots.size() == 0) weaponSlots.append(Player::WeaponType::punch);
        activeSlotIndex = fmax(activeSlotIndex - 1, 0);
        weapon = weaponSlots[activeSlotIndex];
    }
    if (sniperCount <= 0 && weaponSlots.contains(WeaponType::sniper)){
        weaponSlots.removeAll(WeaponType::sniper);
        if (weaponSlots.size() == 0) weaponSlots.append(Player::WeaponType::punch);
        activeSlotIndex = fmax(activeSlotIndex - 1, 0);
        weapon = weaponSlots[activeSlotIndex];
    }
}
// ── 词条摘要 ─────────────────────────────────────────────────

QStringList Player::getModifierSummary() const {
    QStringList lines;
    const auto& m = modifiers;

    // ── 速度/血量 ─────────────────────────────────────────
    if (m.moveSpeedMultiplier != 1.f)
        lines << QString("速度 +%1%").arg(int((m.moveSpeedMultiplier - 1.f) * 100));
    if (m.maxHpMultiplier != 1.f)
        lines << QString("血上限 %1%").arg(int(m.maxHpMultiplier * 100));

    // ── 伤害/减伤 ─────────────────────────────────────────
    if (m.damageBonusMultiplier != 1.f)
        lines << QString("伤害 +%1%").arg(int((m.damageBonusMultiplier - 1.f) * 100));
    if (m.damageReduction != 0.f)
        lines << QString("减伤 %1%").arg(int(m.damageReduction * 100));

    // ── 武器伤害 ──────────────────────────────────────────
    if (m.punchDmgMultiplier != 1.f)
        lines << QString("拳 +%1%").arg(int((m.punchDmgMultiplier  - 1.f) * 100));
    if (m.knifeDmgMultiplier != 1.f)
        lines << QString("刀 +%1%").arg(int((m.knifeDmgMultiplier  - 1.f) * 100));
    if (m.ballDmgMultiplier != 1.f)
        lines << QString("球 +%1%").arg(int((m.ballDmgMultiplier   - 1.f) * 100));
    if (m.gunDmgMultiplier != 1.f)
        lines << QString("枪 +%1%").arg(int((m.gunDmgMultiplier    - 1.f) * 100));

    // ── 特殊词条 ──────────────────────────────────────────
    if (m.doubleEdge)
        lines << "两败俱伤";
    if (m.extraWeaponSlots > 0)
        lines << QString("武器槽 +%1").arg(m.extraWeaponSlots);
    if (m.spellCooldownReduce > 0)
        lines << QString("法术CD -%1s").arg(m.spellCooldownReduce);

    // ── 定身词条 ──────────────────────────────────────────
    if (m.freezeDurationBonus > 0.f)
        lines << QString("定身时长 +%1s").arg(int(m.freezeDurationBonus));
    if (m.freezeDmgMultiplier != 1.f)
        lines << QString("定身伤害 +%1%").arg(int((m.freezeDmgMultiplier - 1.f) * 100));
    if (m.freezeStunBonus > 0.f)
        lines << QString("破定硬直 +%.1fs").arg(m.freezeStunBonus);
    if (m.freezeBreakCDR)
        lines << "破定减CD";

    // ── 隐身词条 ──────────────────────────────────────────
    if (m.stealthDurationBonus > 0.f)
        lines << QString("隐身时长 +%1s").arg(int(m.stealthDurationBonus));
    if (m.stealthSpeedMultiplier != 1.f)
        lines << QString("隐身速度 +%1%").arg(int((m.stealthSpeedMultiplier - 1.f) * 100));
    if (m.stealthRegenPerSec > 0.f)
        lines << QString("隐身回血 %1/s").arg(int(m.stealthRegenPerSec));
    if (m.stealthFirstHit)
        lines << "破影一击";

    // ── 安身词条 ──────────────────────────────────────────
    if (m.barrierDurationBonus > 0.f)
        lines << QString("屏障时长 +%1s").arg(int(m.barrierDurationBonus));
    if (m.barrierRegenPerSec != 1.f)
        lines << QString("屏障回血 %1/s").arg(int(m.barrierRegenPerSec));
    if (m.barrierDmgReduction != 0.2f)
        lines << QString("屏障减伤 %1%").arg(int(m.barrierDmgReduction * 100));

    // ── 铜头词条 ────────────────────────────────────────────
    if (m.ironBodyDurationBonus > 0.f)
        lines << QString("铜头时长 +%1s").arg(int(m.ironBodyDurationBonus));
    if (m.ironBodyReflectCDR)
        lines << "弹反减CD";
    if (m.ironBodyThorns)
        lines << "荆棘";
    if (m.ironBodyHardened)
        lines << "硬化";

    // ── 克隆词条 ──────────────────────────────────────────
    if (m.cloneCanCastSpell)
        lines << "分身施法";
    if (m.cloneDmgMultiplier != 1.f)
        lines << QString("分身伤害 +%1%").arg(int((m.cloneDmgMultiplier - 1.f) * 100));
    if (m.cloneDurationBonus > 0.f)
        lines << QString("分身时长 +%1s").arg(int(m.cloneDurationBonus));
    if (m.cloneDamageExtend)
        lines << "以伤续命";

    // ── 禁字法 ────────────────────────────────────────────
    if (m.forbiddenWord)
        lines << "禁字法";

    return lines;
}
