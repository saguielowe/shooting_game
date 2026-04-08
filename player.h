#ifndef PLAYER_H
#define PLAYER_H
#include<QPainter>
#include<QRectF>
#include "animation.h"
#include "map.h"
#include "modifierdata.h"
#include "gamesession.h"
#include "spellstate.h"

class Player
{
public:
    Player(float x, float y, int id,
           float maxHp, int maxBalls, int maxBullets, int maxSnipers,
           bool canHide, float speedRatio); // id=1 左，id=2 右
    enum class WeaponType{
        punch, knife, ball, rifle, sniper
    };
    enum class ArmorType{
        chainmail, vest, noArmor
    };

    void update();  // 每帧更新位置
    void jump();
    void crouch();
    void moveLeft();
    void moveRight();
    void stop();
    void applyGravity();
    void setDt(float t);
    void weaponControll(QString type);
    bool isDead() const;
    void modifyvelocity(float ratio){
        velocityratio = ratio;
    }
    float getAttackDamage();
    float getDefenseMultiplier(WeaponType attackerWeapon) const;


    bool isShooting = false;
    bool onGround = false;

    struct State{
        QString moveState;
        bool shootState;
    };
    State state; // 玩家待改变状态，注意区分惯性状态（动画绘制的是新状态，惯性是被位置使用的）

    float x, y, dt;
    float vx = 0, vy = 0;
    float hp = maxHp;
    float maxHp;
    int   maxBalls;
    int   maxBullets;
    int   maxSnipers;
    int ballCount = 0, rifleCount = 0, sniperCount = 0;
    bool hpRegenerate = false; // 回血状态
    bool direction = 1; // 1：右，0：左
    float regenerateTime = 0;
    const int id;
    QRect hitbox;
    float getMaxHp() const { return maxHp; }
    // ModifierData modifier;  ← 等 ModifierData 定义好再加
    bool  canHide;
    float velocityratio;   // 出厂速度倍率，从 session 读，不变
    float selfvelocityratio = 1.0f;  // 词条/法术修改这个


    WeaponType weapon = WeaponType::punch;
    ArmorType armor = ArmorType::noArmor;
    int vestHardness = 100;

    void draw(QPainter& painter); // 可以隐藏的场景如草地
    struct ModifierState {
        int   spellCooldownReduce   = 0;    // 累计减少法术冷却秒数
        float maxHpMultiplier       = 1.0f; // 血量上限倍率
        float moveSpeedMultiplier   = 1.0f; // 移动速度倍率
        float punchDmgMultiplier    = 1.0f;
        float knifeDmgMultiplier    = 1.0f;
        float ballDmgMultiplier     = 1.0f;
        float gunDmgMultiplier      = 1.0f;
        float damageBonusMultiplier = 1.0f; // 通用伤害加成
        float damageReduction       = 0.0f; // 0~1，上限 1.0
        bool  doubleEdge            = false;
        int   extraWeaponSlots      = 0;

        // 法术专属（各法术实现时读取）
        float freezeDurationBonus   = 0.f;
        float freezeDmgMultiplier   = 1.0f;
        float freezeStunBonus       = 0.f;
        bool  freezeBreakCDR        = false;

        float stealthDurationBonus  = 0.f;
        float stealthSpeedMultiplier= 1.0f;
        float stealthDmgMultiplier  = 1.0f;
        float stealthRegenPerSec    = 0.f;
        bool  stealthFirstHit       = false; // 破影一击

        float barrierDurationBonus  = 0.f;
        float barrierRegenPerSec    = 1.0f; // 基础1点/秒
        float barrierDmgReduction   = 0.2f; // 基础20%

        float ironBodyDurationBonus = 0.f;
        bool  ironBodyReflectCDR    = false;
        bool  ironBodyThorns        = false;
        bool  ironBodyHardened      = false;

        bool  cloneCanCastSpell     = false;
        float cloneDmgMultiplier    = 1.0f;
        float cloneDurationBonus    = 0.f;
        bool  cloneDamageExtend     = false;

        bool  forbiddenWord         = false; // 禁字法
    };
    ModifierState modifiers;

    // 用于对手词条（ENEMY_MAX_HP_DOWN / ENEMY_MOVE_SPEED_DOWN）
    // Widget 层在 optionChosen 回调里拿到 ModifierData，
    // 判断是 ENEMY_* 类型后调用 opponent->applyEnemyModifier()
    void applyEnemyModifier(const ModifierData& mod);

    // ── player.h 修改后的接口 ───────────────────────────────────
    void  applyModifier(const ModifierData& mod);
    SpellState spellState;
    GameSession::Spell spell = GameSession::Spell::NONE; // 本局法术，从session读
 
    // 武器槽位
    QVector<WeaponType> weaponSlots;   // 当前持有的武器列表
    int activeSlotIndex = 0;           // 当前激活槽
 
    void switchWeaponSlot();           // C/N键调用，循环切换
    bool pickupWeapon(WeaponType w);   // 捡武器时调用，返回是否成功
    int  maxWeaponSlots() const {      // 当前最大槽数
        return 1 + modifiers.extraWeaponSlots;
    }
    WeaponType currentWeapon() const { // 当前激活武器
        if (weaponSlots.isEmpty()) return WeaponType::punch;
        return weaponSlots[activeSlotIndex];
    }
 
    // 词条摘要，供状态栏绘制
    QStringList getModifierSummary() const;
 
    // initMaxHp：出厂血量上限，词条倍率基于它，不要基于当前maxHp叠乘
    float initMaxHp = 100.f;  // 在构造函数里赋值为传入的maxHp参数
    void updateSlots();

    /* mode=0 正常模式
     * mode=1 投掷模式：实心球可投掷数量翻倍
     * mode=2 枪战模式：生命值提高至200，手枪可以射击5次
     */

private:
    Animation animation;
    float width = 40, height = 60;

    const float gravity = 1500.0f;
    const float moveSpeed = 400.0f; // 最大移速
    const float jumpSpeed = 800.0f;
    const float resistance = 20.0f; // 不按按键时，左右移动速度的衰减
    const float airResistance = 30.0f; // 不按按键时，空中左右移动速度的衰减
    const float acceleration = 30.0f; // 左右移动时，速度的增加




};

#endif // PLAYER_H
