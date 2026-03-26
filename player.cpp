#include "player.h"
#include <QDebug>
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
    , bulletCount(0)
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

void Player::draw(QPainter& painter) { // 传窗口painter的引用
    if (canHide && state.moveState == "crouch"){
        return;
    }
    if (weapon == WeaponType::knife){
        animation.loadWeapon("knife");
    }
    else if (weapon == WeaponType::ball){
        animation.loadWeapon("ball");
    }
    else if (weapon == WeaponType::rifle){
        animation.loadWeapon("rifle");
    }
    else if (weapon == WeaponType::sniper){
        animation.loadWeapon("sniper");
    }
    else if (weapon == WeaponType::punch){
        animation.loadWeapon("no_weapon"); // 无武器时不渲染
    }

    if (direction){
        animation.draw(painter, x, y, false, (state.moveState == "null") && (!state.shootState));
        // if (state.shootState && (weapon == WeaponType::rifle || weapon == WeaponType::sniper)){
        //     animation.paintGun(painter, x+53, y+12);
        // }
    }
    else{
        animation.draw(painter, x, y, true, (state.moveState == "null") && (!state.shootState)); // 播放动画
    }
    // 绘制灰色背景
    QRect backgroundRect(hitbox.left(), hitbox.top() - 20, 100, 15);
    painter.setBrush(Qt::gray);
    painter.drawRect(backgroundRect);

    // 绘制红色血条
    int barWidth = static_cast<int>(100 * (fmax(0, hp) / maxHp));
    QRect healthRect(hitbox.left(), hitbox.top() - 20, barWidth, 15);
    painter.setBrush(Qt::red);
    painter.drawRect(healthRect);
    painter.drawText(healthRect, QString::number(int(hp)));
    painter.drawText(hitbox.left() - 10, hitbox.top(), (id?"右":"左"));

    if (armor == ArmorType::noArmor) return;

    // 绘制护盾条
    QRect backgroundRect2(hitbox.left(), hitbox.top() - 40, 100, 15);
    painter.setBrush(Qt::gray);

    if (armor == ArmorType::chainmail){
        painter.setBrush(Qt::white);
        painter.drawRect(hitbox.left(), hitbox.top() - 40, 100, 15);
    }
    else if (armor == ArmorType::vest){
        painter.setBrush(QColor(165, 42, 42));
        painter.drawRect(hitbox.left(), hitbox.top() - 40, int(vestHardness), 15);
    }
}

void Player::weaponControll(QString type){
    if (type == "knife"){
        weapon = WeaponType::knife;
    }
    else if (type == "ball"){
        weapon = WeaponType::ball;
        ballCount = maxBalls;
    }
    else if (type == "bandage"){
        hp = fmin(hp + 30, maxHp);
    }
    else if (type == "rifle"){
        weapon = WeaponType::rifle;
        bulletCount = maxBullets;
    }
    else if (type == "medkit"){
        hp = maxHp;
    }
    else if (type == "sniper"){
        weapon = WeaponType::sniper;
        bulletCount = maxSnipers;
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

bool Player::isDead() const{
    return hp<=0 ;
}

void Player::applyModifier(const ModifierData& mod) {
    switch (mod.type) {

    // ── 通用词条 ──────────────────────────────────────────
    case ModifierType::SPELL_COOLDOWN_REDUCE:
        modifiers.spellCooldownReduce += 5;
        break;

    case ModifierType::MAX_HP_UP:
        modifiers.maxHpMultiplier += 0.2f;
        maxHp = maxHp * modifiers.maxHpMultiplier;
        hp *= modifiers.maxHpMultiplier; // 血条上限与现有血量同步增加
        break;

    case ModifierType::MOVE_SPEED_UP:
        modifiers.moveSpeedMultiplier += 0.15f;
        selfvelocityratio = velocityratio * modifiers.moveSpeedMultiplier;
        break;

    case ModifierType::PUNCH_DAMAGE_UP:
        modifiers.punchDmgMultiplier += 1.0f; // +100%
        break;

    case ModifierType::KNIFE_DAMAGE_UP:
        modifiers.knifeDmgMultiplier += 0.5f;
        break;

    case ModifierType::BALL_DAMAGE_UP:
        modifiers.ballDmgMultiplier += 0.3f;
        break;

    case ModifierType::GUN_DAMAGE_UP:
        modifiers.gunDmgMultiplier += 0.3f;
        break;

    case ModifierType::DAMAGE_BONUS_UP:
        modifiers.damageBonusMultiplier += 0.2f;
        break;

    case ModifierType::DAMAGE_REDUCTION_UP:
        modifiers.damageReduction = qMin(1.0f, modifiers.damageReduction + 0.2f);
        break;

    case ModifierType::DOUBLE_EDGE:
        modifiers.doubleEdge = true;
        break;

    case ModifierType::EXTRA_WEAPON_SLOT:
        modifiers.extraWeaponSlots++;
        break;

    // ── 定身词条 ──────────────────────────────────────────
    case ModifierType::FREEZE_DURATION_UP:
        modifiers.freezeDurationBonus += 2.f;
        break;
    case ModifierType::FREEZE_DAMAGE_UP:
        modifiers.freezeDmgMultiplier += 0.2f;
        break;
    case ModifierType::FREEZE_STUN_UP:
        modifiers.freezeStunBonus += 0.5f;
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
    case ModifierType::STEALTH_DAMAGE_REDUCTION:
        modifiers.stealthDmgReduction = qMin(1.0f, modifiers.stealthDmgReduction + 0.2f);
        break;
    case ModifierType::STEALTH_DAMAGE_UP:
        modifiers.stealthDmgMultiplier += 0.2f;
        break;
    case ModifierType::STEALTH_REGEN:
        modifiers.stealthRegenPerSec += 1.f;
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
        modifiers.damageBonusMultiplier += 1.0f; // +100%
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
        hp *= modifiers.maxHpMultiplier;
        break;
    case ModifierType::ENEMY_MOVE_SPEED_DOWN:
        modifiers.moveSpeedMultiplier = qMax(0.3f, modifiers.moveSpeedMultiplier - 0.15f);
        selfvelocityratio = velocityratio * modifiers.moveSpeedMultiplier;
        break;
    default:
        break;
    }
}

// ── getAttackDamage() 修改版 ─────────────────────────────────
float Player::getAttackDamage() const {
    float base = 0.f;
    switch (weapon) {
    case WeaponType::punch:  base = 5.f * modifiers.punchDmgMultiplier; break;
    case WeaponType::knife:  base = 15.f * modifiers.knifeDmgMultiplier; break;
    case WeaponType::ball:   base = 1.f * modifiers.ballDmgMultiplier;  break;
    case WeaponType::rifle:  base = 1.f * modifiers.gunDmgMultiplier;   break;
    case WeaponType::sniper: base = 1.f * modifiers.gunDmgMultiplier;   break;
    }
    base *= modifiers.damageBonusMultiplier;
    if (modifiers.doubleEdge) base *= 2.f;
    // 法术加成由各法术在激活时额外乘，这里不处理
    return base;
}

// ── getDefenseMultiplier() 修改版 ────────────────────────────
float Player::getDefenseMultiplier(WeaponType weaponType) const {

    float totalReduction = qMin(1.0f, modifiers.damageReduction);
    float multiplier = 1.0f - totalReduction;
    if (modifiers.doubleEdge) multiplier *= 2.f;
    return multiplier;
}

void Player::switchWeaponSlot() {
    if (weaponSlots.size() <= 1) return;
    activeSlotIndex = (activeSlotIndex + 1) % weaponSlots.size();
    weapon = weaponSlots[activeSlotIndex];
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
 
// ── 词条摘要 ─────────────────────────────────────────────────
 
QStringList Player::getModifierSummary() const {
    QStringList lines;
    const auto& m = modifiers;
 
    if (m.moveSpeedMultiplier != 1.f)
        lines << QString("速度 %1%").arg(int((m.moveSpeedMultiplier - 1.f) * 100));
    if (m.damageBonusMultiplier != 1.f)
        lines << QString("伤害 +%1%").arg(int((m.damageBonusMultiplier - 1.f) * 100));
    if (m.damageReduction != 0.f)
        lines << QString("减伤 %1%").arg(int(m.damageReduction * 100));
    if (m.maxHpMultiplier != 1.f)
        lines << QString("血上限 %1%").arg(int(m.maxHpMultiplier * 100));
    if (m.punchDmgMultiplier != 1.f)
        lines << QString("拳 x%.1f").arg(m.punchDmgMultiplier);
    if (m.knifeDmgMultiplier != 1.f)
        lines << QString("刀 x%.1f").arg(m.knifeDmgMultiplier);
    if (m.ballDmgMultiplier != 1.f)
        lines << QString("球 x%.1f").arg(m.ballDmgMultiplier);
    if (m.gunDmgMultiplier != 1.f)
        lines << QString("枪 x%.1f").arg(m.gunDmgMultiplier);
    if (m.doubleEdge)
        lines << "两败俱伤";
    if (m.extraWeaponSlots > 0)
        lines << QString("武器槽 +%1").arg(m.extraWeaponSlots);
    if (m.freezeDurationBonus > 0)
        lines << QString("定身 +%1s").arg(int(m.freezeDurationBonus));
    if (m.stealthDurationBonus > 0)
        lines << QString("隐身 +%1s").arg(int(m.stealthDurationBonus));
 
    return lines;
}