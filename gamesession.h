#ifndef GAMESESSION_H
#define GAMESESSION_H
#include <QString>

// ============================================================
//  GameSession — 跨局持久状态，由 MainWindow 持有
//  Widget 每局开始时读取，结束时通过信号汇报结果
// ============================================================

struct GameSession {

    // ---- 游戏模式 ----------------------------------------
    enum class Mode {
        AI,       // 人机对战
        PVP,      // 双人对战
        ENDLESS   // 无尽模式（AI无限血，统计伤害）
    };

    // ---- 法术（占坑，逻辑后续实现）----------------------
    enum class Spell {
        NONE,
        FREEZE,    // 定身
        STEALTH,   // 隐身（仅人机）
        BARRIER,   // 安身法
        IRON_BODY, // 铜头铁臂
        CLONE,     // 身外身法
        FORBIDDEN  // 禁字法
    };

    // ---- 基础参数 ----------------------------------------
    Mode    mode    = Mode::AI;
    int     bestOf  = 3;        // BO3/5/7/自定义
    QString scene   = "default";

    // ---- 初始装备参数（原来 initSettings 的内容）--------
    float   maxHp      = 100.f;
    int     maxBalls   = 3;
    int     maxBullets = 5;
    int     maxSnipers = 3;

    // ---- 法术选择 ----------------------------------------
    Spell   spellP1 = Spell::NONE;
    Spell   spellP2 = Spell::NONE;

    // ---- 比分（MainWindow 维护）-------------------------
    int     scoreP1 = 0;
    int     scoreP2 = 0;

    // ---- 无尽模式专用 ------------------------------------
    float   totalDamageDealt = 0.f;
    float   survivalTime     = 0.f;

    // ---- 工具方法 ----------------------------------------
    int winsNeeded() const { return bestOf / 2 + 1; }

    bool isMatchOver() const {
        return scoreP1 >= winsNeeded() || scoreP2 >= winsNeeded();
    }

    void addScore(int playerId) {
        if (playerId == 0) scoreP1++;
        else               scoreP2++;
    }

    void resetScores() {
        scoreP1 = 0;
        scoreP2 = 0;
    }

    // 法术是否对该模式可用（隐身仅人机）
    bool isSpellAvailable(Spell spell, Mode m) const {
        if (spell == Spell::STEALTH && m != Mode::AI) return false;
        return true;
    }
};

#endif // GAMESESSION_H
