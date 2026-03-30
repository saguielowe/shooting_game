#pragma once
#include <QString>
#include <QVector>

// ============================================================
//  ModifierData  —  一张词条的完整描述
//  applyModifier() 消费此结构，Player 内部按 type 分支处理
// ============================================================

enum class ModifierType {
    // ── 通用词条 ──────────────────────────────────────────
    SPELL_COOLDOWN_REDUCE,      // 减少法术回转时间（-5s/级）
    MAX_HP_UP,                  // 增加自身血量上限（+20%/级）
    ENEMY_MAX_HP_DOWN,          // 减少对手血量上限（-20%/级）
    MOVE_SPEED_UP,              // 增加自身移动速度
    ENEMY_MOVE_SPEED_DOWN,      // 减少对手移动速度

    PUNCH_DAMAGE_UP,            // 增加拳击伤害（+100%/级）
    KNIFE_DAMAGE_UP,            // 增加小刀伤害（+50%/级）
    BALL_DAMAGE_UP,             // 增加实心球伤害（+30%/级）
    GUN_DAMAGE_UP,              // 增加枪械伤害（+30%/级）

    DAMAGE_BONUS_UP,            // 提高伤害加成（+20%/级）
    DAMAGE_REDUCTION_UP,        // 增加伤害减免（+20%/级，内部上限100%）

    DOUBLE_EDGE,                // 造成伤害翻倍，受到伤害翻倍（唯一）
    EXTRA_WEAPON_SLOT,          // 获得一个武器槽位

    // ── 定身法专属词条 ────────────────────────────────────
    FREEZE_DURATION_UP,         // 增加定身持续时间（+2s/级）
    FREEZE_DAMAGE_UP,           // 增加定身时造成的伤害（+20%/级）
    FREEZE_BREAK_CDR,           // 定身破碎后减少冷却（-5s，唯一）

    // ── 隐身法专属词条 ────────────────────────────────────
    STEALTH_DURATION_UP,        // 增加隐身持续时间（+2s/级）
    STEALTH_SPEED_UP,           // 隐身时移动速度（+20%/级）
    STEALTH_REGEN,              // 隐身时缓慢回血（+2点/秒/级）
    STEALTH_FIRST_HIT,          // 破影一击（唯一）

    // ── 安身法专属词条 ────────────────────────────────────
    BARRIER_DURATION_UP,        // 增加屏障持续时间（+5s/级）
    BARRIER_REGEN_UP,           // 增加屏障内回血（+2点/秒/级）
    BARRIER_REDUCTION_UP,       // 增加屏障内伤害减免（+20%/级，上限100%）

    // ── 身外身法专属词条 ──────────────────────────────────
    CLONE_CAN_CAST_SPELL,       // 克隆体可使用技能一次（唯一）
    CLONE_DAMAGE_UP,            // 克隆体伤害（+20%/级）
    CLONE_DURATION_UP,          // 克隆体持续时间（+5s/级）
    CLONE_DAMAGE_EXTEND,        // 按造成伤害等比延长持续时间（唯一）

    // ── 禁字法专属（占位，禁字法本身在GameSession处理）──
    FORBIDDEN_WORD,             // 放弃所有法术，+100%伤害加成（唯一）
};

// 是否为"唯一词条"（只能拥有一级）
inline bool isUniqueModifier(ModifierType t) {
    switch (t) {
    case ModifierType::DOUBLE_EDGE:
    case ModifierType::FREEZE_BREAK_CDR:
    case ModifierType::CLONE_CAN_CAST_SPELL:
    case ModifierType::CLONE_DAMAGE_EXTEND:
    case ModifierType::FORBIDDEN_WORD:
    case ModifierType::STEALTH_FIRST_HIT:
        return true;
    default:
        return false;
    }
}

struct ModifierData {
    ModifierType type;
    QString      displayName;   // 卡片标题
    QString      description;   // 卡片描述文字
    int          level = 1;     // 叠加级数（由Player维护，此处传入时为1）

    // ── 工厂方法：生成所有通用词条 ──────────────────────
    static QVector<ModifierData> allGeneric();

    // ── 工厂方法：按法术类型生成专属词条 ────────────────
    // spellName: "freeze" / "stealth" / "barrier" / "iron_body"
    //            / "clone"  / "forbidden"
    // 法术接口预留，后续各法术实现时填充
    static QVector<ModifierData> forSpell(const QString& spellName);

    // ── 根据 session 中玩家的法术返回合法词条池 ─────────
    static QVector<ModifierData> poolForPlayer(const QString& spellName);
};

// ── 实现（header-only，避免多一个cpp）────────────────────

inline QVector<ModifierData> ModifierData::allGeneric() {
    return {
        { ModifierType::SPELL_COOLDOWN_REDUCE, "急转轮回",    "减少法术回转时间 5秒" },
        { ModifierType::MAX_HP_UP,             "厚积薄发",    "增加自身血量上限 20%" },
        { ModifierType::ENEMY_MAX_HP_DOWN,     "釜底抽薪",    "减少对手血量上限 20%" },
        { ModifierType::MOVE_SPEED_UP,         "风驰电掣",    "增加自身移动速度" },
        { ModifierType::ENEMY_MOVE_SPEED_DOWN, "缚风捉影",    "减少对手移动速度" },
        { ModifierType::PUNCH_DAMAGE_UP,       "铁拳无敌",    "增加拳击伤害 100%" },
        { ModifierType::KNIFE_DAMAGE_UP,       "刀光剑影",    "增加小刀伤害 50%" },
        { ModifierType::BALL_DAMAGE_UP,        "石破天惊",    "增加实心球伤害 30%" },
        { ModifierType::GUN_DAMAGE_UP,         "弹无虚发",    "增加枪械伤害 30%" },
        { ModifierType::DAMAGE_BONUS_UP,       "锋芒毕露",    "提高伤害加成 25%" },
        { ModifierType::DAMAGE_REDUCTION_UP,   "金刚不坏",    "增加伤害减免 20%" },
        { ModifierType::DOUBLE_EDGE,           "两败俱伤",    "造成与受到的伤害增加 50%" },
        { ModifierType::EXTRA_WEAPON_SLOT,     "百宝囊",      "获得一个额外武器槽位" },
    };
}

inline QVector<ModifierData> ModifierData::forSpell(const QString& spellName) {
    if (spellName == "freeze") {
        return {
            { ModifierType::FREEZE_DURATION_UP,  "长缚",     "增加定身持续时间 1.5秒" },
            { ModifierType::FREEZE_DAMAGE_UP,    "缚中伤",   "定身期间造成的伤害 +20%" },
            { ModifierType::FREEZE_BREAK_CDR,    "破而后立", "定身破碎后减少冷却 5秒（唯一）" },
        };
    }
    if (spellName == "stealth") {
        return {
                { ModifierType::STEALTH_DURATION_UP, "长隐",   "增加隐身持续时间 +2秒" },
                { ModifierType::STEALTH_SPEED_UP,    "影疾",   "隐身时移动速度 +20%" },
                { ModifierType::STEALTH_REGEN,       "潜息",   "隐身时每秒回血 +2点" },
                { ModifierType::STEALTH_FIRST_HIT,   "破影一击",
                 "隐身时长减半，但第一击伤害随隐身时长增加，并解除隐身（唯一）" },
                };
    }
    if (spellName == "barrier") {
        return {
            { ModifierType::BARRIER_DURATION_UP,   "久安",   "增加屏障持续时间 5秒" },
            { ModifierType::BARRIER_REGEN_UP,      "安中养", "屏障内每秒回血 +2点" },
            { ModifierType::BARRIER_REDUCTION_UP,  "安而固", "屏障内伤害减免 +20%" },
        };
    }
    if (spellName == "clone") {
        return {
            { ModifierType::CLONE_CAN_CAST_SPELL,  "分身施法", "克隆体可使用法术一次（唯一）" },
            { ModifierType::CLONE_DAMAGE_UP,       "分身锋利", "克隆体造成伤害 +20%" },
            { ModifierType::CLONE_DURATION_UP,     "长影",     "克隆体持续时间 +5秒" },
            { ModifierType::CLONE_DAMAGE_EXTEND,   "以伤续命", "按造成伤害等比延长持续时间（唯一）" },
        };
    }
    // iron_body / forbidden：暂无专属词条，后续填充
    return {};
}

inline QVector<ModifierData> ModifierData::poolForPlayer(const QString& spellName) {
    QVector<ModifierData> pool = allGeneric();
    pool += forSpell(spellName);
    return pool;
}
