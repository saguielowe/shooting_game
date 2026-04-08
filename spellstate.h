#ifndef SPELLSTATE_H
#define SPELLSTATE_H

// ============================================================
//  SpellState — 玩家法术相关的运行时状态
//  Player 持有一个实例，各系统只读此结构
// ============================================================

struct SpellState {

    // ── 冷却 ────────────────────────────────────────────────
    float cooldownRemain = 0.f;   // 当前剩余冷却（秒），>0 不能施法
    float cooldownMax    = 0.f;   // 满冷却时间（受词条影响后的值）

    // ── 被定身（由对手施加）─────────────────────────────────
    bool  isFrozen            = false;
    float frozenRemain        = 0.f;   // 剩余定身时间（秒）
    float frozenBreakHpLeft   = 0.f;   // 还需受到多少伤害才破定身
    float frozenTotalDamage    = 0.f;   // 新增：定身期间累计受伤，用于算硬直

    // ── 隐身（自身激活）─────────────────────────────────────
    bool  stealthActive  = false;
    float stealthRemain  = 0.f;   // 剩余隐身时间（秒）
    bool  stealthFirstHitUsed  = false;  // 本次隐身第一击是否已触发，新增

    // ── 接口预留（后续法术填充）─────────────────────────────
    bool  barrierActive  = false;
    float barrierRemain  = 0.f;

    bool  ironBodyActive = false;
    float ironBodyRemain = 0.f;
    bool  ironBodyCdrUsed = false;

    bool  cloneActive    = false;
    float cloneRemain    = 0.f;

    // ── 工具方法 ─────────────────────────────────────────────
    bool isOnCooldown()  const { return cooldownRemain > 0.f; }
    float cooldownRatio()const {
        if (cooldownMax <= 0.f) return 0.f;
        return 1.f - (cooldownRemain / cooldownMax); // 0=刚用完，1=可用
    }

    void tick(float dt) {
        if (cooldownRemain > 0.f) cooldownRemain -= dt;
        if (frozenRemain   > 0.f) frozenRemain   -= dt;
        if (stealthRemain  > 0.f) stealthRemain  -= dt;
        if (barrierRemain  > 0.f) barrierRemain  -= dt;
        if (ironBodyRemain > 0.f) ironBodyRemain -= dt;
        if (cloneRemain    > 0.f) cloneRemain    -= dt;

        // 状态到期自动关闭
        if (frozenRemain  <= 0.f) isFrozen      = false;
        if (stealthRemain <= 0.f) stealthActive  = false;
        if (barrierRemain <= 0.f) barrierActive  = false;
        if (ironBodyRemain<= 0.f) ironBodyActive = false;
        if (cloneRemain   <= 0.f) cloneActive    = false;
    }
};

#endif // SPELLSTATE_H
