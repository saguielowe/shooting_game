#include "widget.h"
#include "soundmanager.h"
#include <QMessageBox>
#include <QPushButton>
#include <QLabel>
#include "./ui_widget.h"
#include <QScreen>
static QString spellEnumToString(GameSession::Spell spell) {
    switch (spell) {
    case GameSession::Spell::FREEZE:    return "freeze";
    case GameSession::Spell::STEALTH:   return "stealth";
    case GameSession::Spell::BARRIER:   return "barrier";
    case GameSession::Spell::IRON_BODY: return "iron_body";
    case GameSession::Spell::CLONE:     return "clone";
    case GameSession::Spell::FORBIDDEN: return "forbidden";
    default:                     return "";
    }
}
    void Widget::applyBestModifierToAI() {
        if (players.size() < 2) return;
        constexpr int aiIndex = 1;
        auto& aiPlayer = players[aiIndex];
        auto& playerPlayer = players[0];
    
        QVector<ModifierData> pool = filteredModifierPoolForPlayer(aiIndex, true);
        if (pool.isEmpty()) return;

        // AI 修饰符选择策略：根据当前血量和敌方状况智能选择
        float aiHealthRatio = (float)aiPlayer->hp / aiPlayer->getMaxHp();
        float playerHealthRatio = (float)playerPlayer->hp / playerPlayer->getMaxHp();
    
        ModifierData bestModifier = pool[0];
            float bestScore = evaluateModifierForAIWithPersonality(bestModifier, aiHealthRatio, playerHealthRatio);

        for (int i = 1; i < pool.size(); ++i) {
                float score = evaluateModifierForAIWithPersonality(pool[i], aiHealthRatio, playerHealthRatio);
            if (score > bestScore) {
                bestScore = score;
                bestModifier = pool[i];
            }
        }

        aiPlayer->applyModifier(bestModifier);
        qDebug().noquote() << QString("[无尽AI词条-智选] 获得：%1 (评分: %2)")
                                  .arg(bestModifier.displayName)
                                  .arg(bestScore, 0, 'f', 1);
    }

    float Widget::evaluateModifierForAI(const ModifierData& mod, float aiHpRatio, float playerHpRatio) const
    {
        float score = 50.0f; // 基础分
    
        // 分类评估不同类型修饰符
        if (mod.displayName.contains("伤害", Qt::CaseInsensitive) || 
            mod.displayName.contains("攻击", Qt::CaseInsensitive)) {
            // 攻击增强：当敌方血量充足时好，可以尽快击杀
            score += (1.0f - playerHpRatio) * 30.0f;
            // 敌方没有防具时优先度提高
            if (players.size() >= 2 && players[0]->armor == Player::ArmorType::noArmor) {
                score += 15.0f;
            }
        }
        else if (mod.displayName.contains("冷却", Qt::CaseInsensitive) || 
                 mod.displayName.contains("CDR", Qt::CaseInsensitive)) {
            // 冷却减少：法术能更频繁释放，很有价值
            score += 35.0f;
        }
        else if (mod.displayName.contains("生命", Qt::CaseInsensitive) || 
                 mod.displayName.contains("血", Qt::CaseInsensitive) ||
                 mod.displayName.contains("HP", Qt::CaseInsensitive)) {
            // 生命值增加：血量越低越需要
            score += (1.0f - aiHpRatio) * 40.0f;
        }
        else if (mod.displayName.contains("护甲", Qt::CaseInsensitive) || 
                 mod.displayName.contains("防御", Qt::CaseInsensitive)) {
            // 防御增强：血量越低越需要
            score += (1.0f - aiHpRatio) * 30.0f;
        }
        else if (mod.displayName.contains("速度", Qt::CaseInsensitive) || 
                 mod.displayName.contains("移动", Qt::CaseInsensitive)) {
            // 移动速度：通用价值，增加逃脱/追击能力
            score += 20.0f + (1.0f - aiHpRatio) * 15.0f;
        }
        else if (mod.displayName.contains("范围", Qt::CaseInsensitive) ||
                 mod.displayName.contains("大小", Qt::CaseInsensitive)) {
            // 范围增强：有一定价值
            score += 15.0f;
        }
        else if (mod.displayName.contains("反伤", Qt::CaseInsensitive) ||
                 mod.displayName.contains("反弹", Qt::CaseInsensitive)) {
            // 反伤/反弹：对手武器强时很重要
            if (players.size() >= 2) {
                Player::WeaponType enemyWeapon = players[0]->weapon;
                if (enemyWeapon == Player::WeaponType::rifle || 
                    enemyWeapon == Player::WeaponType::sniper ||
                    enemyWeapon == Player::WeaponType::ball) {
                    score += 25.0f;
                }
            }
        }
        else if (mod.displayName.contains("克制", Qt::CaseInsensitive) ||
                 mod.displayName.contains("压制", Qt::CaseInsensitive)) {
            // 压制敌方的效果
            score += 20.0f;
        }
        else {
            // 其他未分类修饰符，给予中等评分
            score += 10.0f;
        }

        // 稳定性因子：引入少量随机性避免过度优化
        score += QRandomGenerator::global()->bounded(10.0f);
    
        return score;
    }

static GameSession::Spell weightedCounterPick(GameSession::Spell playerSpell) {
    int roll = QRandomGenerator::global()->bounded(100);
    switch (playerSpell) {
    case GameSession::Spell::FREEZE:
        // 对定身：偏向隐身规避，其次铜头
        if (roll < 70) return GameSession::Spell::STEALTH;
        return GameSession::Spell::IRON_BODY;
    case GameSession::Spell::STEALTH:
        // 对隐身：偏向定身控节奏
        if (roll < 75) return GameSession::Spell::FREEZE;
        return GameSession::Spell::IRON_BODY;
    case GameSession::Spell::IRON_BODY:
        // 对铜头：偏向定身拉开换血节奏
        if (roll < 75) return GameSession::Spell::FREEZE;
        return GameSession::Spell::STEALTH;
    default:
        // 玩家无法术时，AI更偏攻击性控场
        return GameSession::Spell::FREEZE;
    }
}

// 检查玩家是否已拥有某唯一词条
static bool playerHasUniqueModifier(const std::shared_ptr<Player>& p, ModifierType t) {
    switch (t) {
    case ModifierType::DOUBLE_EDGE:         return p->modifiers.doubleEdge;
    case ModifierType::FREEZE_BREAK_CDR:    return p->modifiers.freezeBreakCDR;
    case ModifierType::CLONE_CAN_CAST_SPELL:return p->modifiers.cloneCanCastSpell;
    case ModifierType::CLONE_DAMAGE_EXTEND: return p->modifiers.cloneDamageExtend;
    case ModifierType::FORBIDDEN_WORD:      return p->modifiers.forbiddenWord;
    case ModifierType::IRON_BODY_REFLECT_CDR:return p->modifiers.ironBodyReflectCDR;
    case ModifierType::IRON_BODY_THORNS:    return p->modifiers.ironBodyThorns;
    case ModifierType::IRON_BODY_HARDENED:  return p->modifiers.ironBodyHardened;
    default: return false;
    }
}
static QString weaponShortName(Player::WeaponType w) { // 武器名称简写，用于状态栏显示
    switch (w) {
    case Player::WeaponType::punch:  return "拳";
    case Player::WeaponType::knife:  return "刀";
    case Player::WeaponType::ball:   return "球";
    case Player::WeaponType::rifle:  return "步枪";
    case Player::WeaponType::sniper: return "狙击";
    }
    return "?";
}
const float Widget::DROP_GUARANTEE_TIME = 1.f;
const float Widget::ENDLESS_AI_MODIFIER_INTERVAL = 25.f;

// ── 各阶段掉落配置 ───────────────────────────────────────────
struct DropConfig {
    // 大类概率（加起来=100，剩余=不掉落）
    int weaponChance;
    int armorChance;
    int modifierChance;
    int medicineChance;

    // 武器子类概率（加起来=100）
    int knifeChance;
    int ballChance;
    int rifleChance;
    int sniperChance;
};

// 每帧触发概率（千分之X）
static const int DROP_CHANCE_PER_FRAME = 100;
float Widget::evaluateModifierForAIWithPersonality(const ModifierData& mod, float aiHpRatio, float playerHpRatio) const
{
    // 基础评分（不考虑性格）
    float baseScore = evaluateModifierForAI(mod, aiHpRatio, playerHpRatio);
    
    // 根据 AI 性格应用权重调整
    GameAI::AIPersonality personality = ai ? ai->getPersonality() : GameAI::AIPersonality::RUSH;
    float personalityMultiplier = 1.0f;
    
    if (mod.displayName.contains("伤害", Qt::CaseInsensitive) || 
        mod.displayName.contains("攻击", Qt::CaseInsensitive)) {
        switch (personality) {
        case GameAI::AIPersonality::RUSH:       personalityMultiplier = 1.4f; break;  // 冲锋：特别喜欢伤害
        case GameAI::AIPersonality::KITE:       personalityMultiplier = 0.8f; break;  // 风筝：伤害次要
        case GameAI::AIPersonality::SCAVENGER:  personalityMultiplier = 1.0f; break;  // 拾荒：正常
        case GameAI::AIPersonality::TRICKSTER:  personalityMultiplier = 0.9f; break;  // 诡术：偏好法术相关
        }
    }
    else if (mod.displayName.contains("冷却", Qt::CaseInsensitive) || 
             mod.displayName.contains("CDR", Qt::CaseInsensitive)) {
        switch (personality) {
        case GameAI::AIPersonality::RUSH:       personalityMultiplier = 0.9f; break;   // 冲锋：冷却次要
        case GameAI::AIPersonality::KITE:       personalityMultiplier = 1.1f; break;   // 风筝：有一定价值
        case GameAI::AIPersonality::SCAVENGER:  personalityMultiplier = 1.0f; break;   // 拾荒：正常
        case GameAI::AIPersonality::TRICKSTER:  personalityMultiplier = 1.5f; break;   // 诡术：法术能手，CDR 最爱
        }
    }
    else if (mod.displayName.contains("生命", Qt::CaseInsensitive) || 
             mod.displayName.contains("血", Qt::CaseInsensitive) ||
             mod.displayName.contains("HP", Qt::CaseInsensitive)) {
        switch (personality) {
        case GameAI::AIPersonality::RUSH:       personalityMultiplier = 0.8f; break;   // 冲锋：血量不重要
        case GameAI::AIPersonality::KITE:       personalityMultiplier = 1.2f; break;   // 风筝：生存很重要
        case GameAI::AIPersonality::SCAVENGER:  personalityMultiplier = 1.3f; break;   // 拾荒：重视生存
        case GameAI::AIPersonality::TRICKSTER:  personalityMultiplier = 1.0f; break;   // 诡术：均衡
        }
    }
    else if (mod.displayName.contains("护甲", Qt::CaseInsensitive) || 
             mod.displayName.contains("防御", Qt::CaseInsensitive)) {
        switch (personality) {
        case GameAI::AIPersonality::RUSH:       personalityMultiplier = 0.7f; break;   // 冲锋：防御不需要
        case GameAI::AIPersonality::KITE:       personalityMultiplier = 1.3f; break;   // 风筝：防御很好
        case GameAI::AIPersonality::SCAVENGER:  personalityMultiplier = 1.2f; break;   // 拾荒：有帮助
        case GameAI::AIPersonality::TRICKSTER:  personalityMultiplier = 0.9f; break;   // 诡术：普通
        }
    }
    else if (mod.displayName.contains("速度", Qt::CaseInsensitive) || 
             mod.displayName.contains("移动", Qt::CaseInsensitive)) {
        switch (personality) {
        case GameAI::AIPersonality::RUSH:       personalityMultiplier = 1.2f; break;   // 冲锋：速度帮助推进
        case GameAI::AIPersonality::KITE:       personalityMultiplier = 1.4f; break;   // 风筝：速度是必须
        case GameAI::AIPersonality::SCAVENGER:  personalityMultiplier = 1.1f; break;   // 拾荒：有帮助
        case GameAI::AIPersonality::TRICKSTER:  personalityMultiplier = 1.0f; break;   // 诡术：普通
        }
    }
    // 其他修饰符默认 multiplier = 1.0f
    
    return baseScore * personalityMultiplier + (QRandomGenerator::global()->bounded(5LL) - 2.5f);  // 减少随机性范围
}

// 三个阶段配置，直接改这里调试
static const DropConfig PHASE_CONFIGS[3] = {
    // 阶段0：0~20秒，词条+近战为主，无枪械
    {
        20,  // 武器20%
        20,  // 护甲20%
        50,  // 词条50%
        10,  // 药品10%
        60, 40, 0, 0  // 武器：刀60%，球40%，无枪
    },
    // 阶段1：20~40秒，球类加入，词条减少
    {
        35,  // 武器35%
        20,  // 护甲20%
        35,  // 词条35%
        10,  // 药品10%
        30, 50, 15, 5  // 武器：刀30%，球50%，步枪15%，狙5%
    },
    // 阶段2：40秒+，枪械全开，词条进一步减少
    {
        45,  // 武器45%
        15,  // 护甲15%
        25,  // 词条25%
        15,  // 药品15%
        15, 35, 35, 15  // 武器：刀15%，球35%，步枪35%，狙15%
    },
};

Widget::Widget(const GameSession& session, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget) // 初始化已定义但未赋值的变量，大括号内是要做的操作
{

    this->setFixedSize(1024, 1024);
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &Widget::gameLoop);
    applySession(session);
    initGame();
    qDebug() <<"game initialized.";
    setFocusPolicy(Qt::StrongFocus);  // 获取键盘焦点

    SoundManager::instance().preload("crash");
    SoundManager::instance().preload("rifle");
    SoundManager::instance().preload("knife");
    SoundManager::instance().preload("sniper");
    SoundManager::instance().preload("hit");
    SoundManager::instance().preload("stealth"); // 隐身使用音效
    SoundManager::instance().preload("freeze"); // 定身使用音效
    SoundManager::instance().preload("frozen"); // 定身破碎音效
    SoundManager::instance().preload("tongtou"); // 铜头使用音效
    SoundManager::instance().preload("tanfan"); // 弹反音效


    qDebug() <<"scene loaded";

    modifierOverlay = new ModifierOverlay(this);
    connect(modifierOverlay, &ModifierOverlay::optionChosen,
            this, &Widget::onModifierChosen);
    
    endlessResultWidget = new EndlessResultWidget(this);
    
    ui->setupUi(this);
}
 
QString Widget::spellName(GameSession::Spell spell) const {
    switch (spell) {
    case GameSession::Spell::FREEZE:    return "定身";
    case GameSession::Spell::STEALTH:   return "隐身";
    case GameSession::Spell::BARRIER:   return "安身法";
    case GameSession::Spell::IRON_BODY: return "铜头铁臂";
    case GameSession::Spell::CLONE:     return "身外身法";
    case GameSession::Spell::FORBIDDEN: return "禁字法";
    default:                            return "无法术";
    }
}
 
void Widget::drawStatusBar(QPainter& painter, int playerIndex) { // 绘制玩家状态栏（武器槽、法术CD、词条等）
    if (playerIndex >= players.size()) return;
    auto& p = players[playerIndex];
 
    // ── 布局参数 ─────────────────────────────────────────────
    const int panelW  = 130;
    const int marginX = 6;
    const int startY  = 60;   // 顶部留给血条
    const int padX    = 8;
    const int padY    = 6;
    const int lineH   = 18;
 
    // 右侧面板：x从右边计算
    int panelX = (playerIndex == 0)
        ? marginX
        : width() - marginX - panelW;
 
    // ── 计算总高度（动态，取决于词条行数）───────────────────
    QStringList modLines = p->getModifierSummary();
    // 武器槽区：每个槽一行 + 标题1行
    int slotCount = p->weaponSlots.size();
    int sectionWeapon  = lineH + slotCount * lineH + padY;
    // 法术CD区：标题1行 + 进度条1行 + 数字1行
    int sectionSpell   = lineH + lineH + lineH + padY;
    // 词条区：标题1行 + 词条行
    int sectionModifier= lineH + modLines.size() * lineH + padY;
    int totalH = padY + sectionWeapon + sectionSpell + sectionModifier + padY;
 
    // ── 半透明背景 ───────────────────────────────────────────
    painter.save();
    painter.setBrush(QColor(0, 0, 0, 150));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(panelX, startY, panelW, totalH, 8, 8);
 
    // 文字基础设置
    QFont font = painter.font();
    font.setPixelSize(13);
    painter.setFont(font);
 
    int cx = panelX + padX;       // 文字左起点（右侧面板也用同一套，mirror靠对齐方式）
    int textW = panelW - padX * 2;
    int y = startY + padY;
 
    auto drawLine = [&](const QString& text, QColor color,
                        Qt::Alignment align = Qt::AlignLeft) {
        painter.setPen(color);
        painter.drawText(QRect(cx, y, textW, lineH), align | Qt::AlignVCenter, text);
        y += lineH;
    };
 
    // ── 武器槽 ───────────────────────────────────────────────
    drawLine("[ 武器槽 ]", QColor(180, 180, 255));
    for (int i = 0; i < p->weaponSlots.size(); ++i) {
        bool isActive = (i == p->activeSlotIndex);
        QString label = isActive
            ? QString("▶ %1").arg(weaponShortName(p->weaponSlots[i]))
            : QString("  %1").arg(weaponShortName(p->weaponSlots[i]));
        drawLine(label, isActive ? QColor(255, 230, 80) : QColor(170, 170, 170));
    }
    y += padY;
 
    // ── 法术 CD ──────────────────────────────────────────────
    GameSession::Spell mySpell = (playerIndex == 0)
        ? currentSession.spellP1
        : currentSession.spellP2;
 
    drawLine(spellName(mySpell), QColor(180, 180, 255));
 
    // 进度条
    const SpellState& ss = p->spellState;
    float ratio = ss.cooldownRatio(); // 0=刚用完/冷却中，1=可用
    if (!ss.isOnCooldown()) ratio = 1.f;
    int barW  = textW;
    int barH  = 10;
    int barY  = y + (lineH - barH) / 2;
 
    // 背景
    painter.setBrush(QColor(60, 60, 60));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(cx, barY, barW, barH, 3, 3);
 
    // 填充（可用=绿，冷却中=橙）
    QColor barColor = ss.isOnCooldown() ? QColor(220, 140, 40) : QColor(80, 200, 80);
    int fillW = int(barW * ratio);
    if (fillW > 0) {
        painter.setBrush(barColor);
        painter.drawRoundedRect(cx, barY, fillW, barH, 3, 3);
    }
    y += lineH;
 
    // 冷却数字
    QString cdText = ss.isOnCooldown()
        ? QString("CD: %1s").arg(ss.cooldownRemain, 0, 'f', 1)
        : "就绪";
    drawLine(cdText, ss.isOnCooldown() ? QColor(220, 140, 40) : QColor(80, 200, 80));
    y += padY;
 
    // ── 活跃状态提示（定身/隐身中）──────────────────────────
    if (ss.isFrozen)
        drawLine(QString("定身中 %1s").arg(ss.frozenRemain, 0, 'f', 1),
                 QColor(100, 200, 255));
    if (ss.stealthActive)
        drawLine(QString("隐身中 %1s").arg(ss.stealthRemain, 0, 'f', 1),
                 QColor(180, 100, 255));
    if (ss.ironBodyActive)
        drawLine(QString("铜头中 %1s").arg(ss.ironBodyRemain, 0, 'f', 1),
                 QColor(255, 190, 90));
 
    // ── 词条摘要 ─────────────────────────────────────────────
    drawLine("[ 词条 ]", QColor(180, 180, 255));
    if (modLines.isEmpty()) {
        drawLine("无", QColor(100, 100, 100));
    } else {
        for (auto& line : modLines)
            drawLine(line, QColor(200, 220, 200));
    }
 
    painter.restore();
}

void Widget::applySession(const GameSession& session) {
    currentSession = session;
    currentScene   = session.scene;

    // 场景决定速度倍率，其他参数直接从 session 读
    // 替代原来的 Player::initSettings（static，之后要改掉）
    // 现在先保留兼容，等 static 改成实例成员后这里直接删
    float speedRatio = (session.scene == "ice") ? 1.5f : 1.0f;

    scenePixmap = Map::getInstance().loadScene(session.scene);
    qDebug() << "session loaded.";
}

void Widget::initGame() {
    // ---- 重置 intent ------------------------------------
    intent[0] = {MoveIntent::NONE, false};
    intent[1] = {MoveIntent::NONE, false};

    bool   canHide    = (currentSession.scene == "grass");
    float  speedRatio = (currentSession.scene == "ice") ? 1.5f : 1.0f;

    auto p1 = std::make_shared<Player>(
        100, 600, 0,
        currentSession.maxHp,
        currentSession.maxBalls,
        currentSession.maxBullets,
        currentSession.maxSnipers,
        canHide, speedRatio
        );
    auto p2 = std::make_shared<Player>(
        800, 600, 1,
        currentSession.maxHp,
        currentSession.maxBalls,
        currentSession.maxBullets,
        currentSession.maxSnipers,
        canHide, speedRatio
        );
    p1->endlessImmortal = false;
    p2->endlessImmortal = (currentSession.mode == GameSession::Mode::ENDLESS);

    auto c1 = std::make_shared<PlayerController>();
    auto c2 = std::make_shared<PlayerController>();

    c1->bindPlayer(p1);
    c2->bindPlayer(p2);

    players.push_back(p1);
    players.push_back(p2);
    controllers.push_back(c1);
    controllers.push_back(c2);

    // 法术配置：P1沿用session；P2在非PVP模式可按策略解析。
    if (currentSession.mode != GameSession::Mode::PVP) {
        currentSession.spellP2 = resolveAISpellForMode();
    }
    p1->spell = currentSession.spellP1;
    p2->spell = currentSession.spellP2;

    qDebug() << "player spawned";

    // ---- 信号槽 -----------------------------------------
    connect(c1.get(), &PlayerController::requestThrowBall,
            this, &Widget::onPlayerRequest);
    connect(c2.get(), &PlayerController::requestThrowBall,
            this, &Widget::onPlayerRequest);
    auto handleFrozenBroken = [this](int victimId) {
        int casterIdx = 1 - victimId;
        if (casterIdx < 0 || casterIdx >= players.size()) return;
        auto& caster = players[casterIdx];
        if (caster->modifiers.freezeBreakCDR) {
            float cdReduce = 5.f;
            caster->spellState.cooldownRemain = qMax(0.f, caster->spellState.cooldownRemain - cdReduce);
            caster->spellState.cooldownMax = qMax(caster->spellState.cooldownMax - cdReduce, 5.f);
            qDebug().noquote() << QString("[破定减CD] P%1 | CD剩余:%2s | CD上限:%3s")
                                      .arg(caster->id + 1)
                                      .arg(caster->spellState.cooldownRemain, 0, 'f', 1)
                                      .arg(caster->spellState.cooldownMax, 0, 'f', 1);
        }
    };
    connect(c1.get(), &PlayerController::frozenBroken, this, handleFrozenBroken);
    connect(c2.get(), &PlayerController::frozenBroken, this, handleFrozenBroken);

    // ---- AI（人机/无尽模式启动，双人不启动）------------
    if (currentSession.mode != GameSession::Mode::PVP) { // 那为什么无尽模式AI似乎没反应？
        ai->setAIPlayer(players[1]);
        ai->setTargetPlayer(players[0]);
        ai->setFollowDistance(80.0f);
        ai->setDifficulty(5);
        
            // 设置 AI 性格（随机）
            GameAI::AIPersonality personality = GameAI::randomPersonality();
            ai->setPersonality(personality);
            const char* personalityNames[] = {"RUSH", "KITE", "SCAVENGER", "TRICKSTER"};
            qDebug() << QString("[AI初始化] 性格: %1").arg(personalityNames[static_cast<int>(personality)]);
        
        ai->setCurrentSpell(currentSession.spellP2);
        ai->startAI();
    }
    m_gameTime          = 0.f;
    m_timeSinceLastDrop = 0.f;
    m_endlessAiModifierTimer = 0.f;
    cm.resetStats();

    qDebug() << "ai is ready";

    // ---- 计时器 -----------------------------------------
    lastTime.restart();
    timer->start(16);
    qDebug()<<"timer started";
}

// widget.cpp
void Widget::showMatchResult(const QString& text) {
    QLabel* overlay = new QLabel(this);
    overlay->setGeometry(rect());
    overlay->setStyleSheet(
        "background: rgba(0, 0, 0, 180);"
        "color: white;"
        "font-size: 36px;"
        "font-weight: bold;"
    );
    overlay->setAlignment(Qt::AlignCenter);
    overlay->setText(text);
    overlay->show();

    grabKeyboard();
    connect(this, &Widget::keyPressed, overlay, [=]() {
        disconnect(this, &Widget::keyPressed, overlay, nullptr);
        releaseKeyboard();
        overlay->hide();
        overlay->deleteLater();
        QTimer::singleShot(0, this, [=]() {
            emit matchResultConfirmed();  // 新信号，玩家按键确认后
        });
    });
}

void Widget::cleanupGame() {
    timer->stop();
    ai->stopAI();

    // 断开 controller 的信号，防止 clear 后还有回调
    for (auto& c : controllers)
        disconnect(c.get(), nullptr, this, nullptr);

    players.clear();
    controllers.clear();
    entities.clear();
    drops.clear();
    balls.clear();
    bullets.clear();
}

// 新增 resetGame，供 MainWindow 调用
// 流程：gameEnd -- roundend signal -- resetGame -- cleanup -- apply+init -- start timer
void Widget::resetGame(const GameSession& session) {
    qDebug()<<"game reset for next round";
    cleanupGame();

    // 重新应用 session 参数并初始化
    applySession(session);
    // 上一轮结束后终止了timer连接，需要复原。
    connect(timer, &QTimer::timeout, this, &Widget::gameLoop);
    m_gameTime          = 0.f;
    m_timeSinceLastDrop = 0.f;
    initGame();

    timer->start(16);
}

void Widget::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    if (!scenePixmap.isNull()) {
        painter.drawPixmap(0, 0, scenePixmap);
    }

    if (players.size() > 0 && players[0]) {
        players[0]->draw(painter);
    }
    if (players.size() > 1 && players[1]) {
        players[1]->draw(painter);
    }

    // 保护 entities
    for (const auto& entity : entities) {
        if (entity) {
            entity->draw(painter);
        }
    }
    drawStatusBar(painter, 0);
    drawStatusBar(painter, 1);
}
/*
inputIntent: 输入意图。
输入意图与按键相关，与物理场景无关，例如下落不会是输入意图，但确实存在这一状态；
输入意图并不一定有效，例如跳起时不能下蹲，需要进行初步处理，忽略掉矛盾的意图。
但要注意，这里只是对按键矛盾做处理，例如空中按下蹲的无效判定不在此处处理，因为widget不会去查询player是否在空中。
同一时间可能存在多个合法输入意图，例如跳跃射击，但动作意图和射击意图只能存在其一。
其中，边跑边跳是由于既有运动状态导致的，不是玩家输入意图所必然导致的，此处不考虑也不存储这一意图。
*/
void Widget::keyPressEvent(QKeyEvent* event) {
    emit keyPressed();
    pressedKeys.insert(event->key());
    updateIntents();
}
void Widget::updateIntents() {
    // P1
    if (pressedKeys.contains(Qt::Key_W)) intent[0].moveIntent = MoveIntent::JUMP; // 优先处理跳跃下蹲，允许同时按下双键
    else if (pressedKeys.contains(Qt::Key_S)) { // player部分默认已经在widget处检查过合法状态转移
        intent[0].moveIntent = MoveIntent::CROUCH;
    }
    else if (pressedKeys.contains(Qt::Key_A)) intent[0].moveIntent = MoveIntent::MOVING_LEFT;
    else if (pressedKeys.contains(Qt::Key_D)) intent[0].moveIntent = MoveIntent::MOVING_RIGHT;
    else intent[0].moveIntent = MoveIntent::NONE;
    intent[0].attackIntent = pressedKeys.contains(Qt::Key_E);
    if (pressedKeys.contains(Qt::Key_C)){
        auto& p = players[0];
        p->activeSlotIndex = (p->activeSlotIndex + 1) % p->weaponSlots.size(); // 切换武器槽
        p->weapon = p->weaponSlots[p->activeSlotIndex]; // 切换武器
    }
    if (pressedKeys.contains(Qt::Key_Q))
        tryActivateSpell(0);

    // P2同理
    if (pressedKeys.contains(Qt::Key_I)) intent[1].moveIntent = MoveIntent::JUMP;
    else if (pressedKeys.contains(Qt::Key_K)) { // player部分默认已经在widget处检查过合法状态转移
        intent[1].moveIntent = MoveIntent::CROUCH;
    }
    else if (pressedKeys.contains(Qt::Key_J)) intent[1].moveIntent = MoveIntent::MOVING_LEFT;
    else if (pressedKeys.contains(Qt::Key_L)) intent[1].moveIntent = MoveIntent::MOVING_RIGHT;
    else intent[1].moveIntent = MoveIntent::NONE;
    intent[1].attackIntent = pressedKeys.contains(Qt::Key_O);
    if (pressedKeys.contains(Qt::Key_N)){
        auto& p = players[1];
        p->activeSlotIndex = (p->activeSlotIndex + 1) % p->weaponSlots.size(); // 切换武器槽
        p->weapon = p->weaponSlots[p->activeSlotIndex];
    }
    if (pressedKeys.contains(Qt::Key_U))
        tryActivateSpell(1);
}


void Widget::keyReleaseEvent(QKeyEvent* event) { // 按键 --> 意图
    pressedKeys.remove(event->key());
    updateIntents(); // 可以边走边打，但移动时不能跳起/蹲下，需要处理。
    if (event->key() == Qt::Key_A || event->key() == Qt::Key_S || event->key() == Qt::Key_W || event->key() == Qt::Key_D) {
        intent[0].moveIntent = MoveIntent::NONE;
    }

    if (event->key() == Qt::Key_J || event->key() == Qt::Key_K || event->key() == Qt::Key_L || event->key() == Qt::Key_I) {
        intent[1].moveIntent = MoveIntent::NONE;
    }

}

void Widget::gameLoop() {
    if (players.size() < 2) { qDebug() << "players not ready"; return; }

    if (!controllers[0] || !controllers[1]) { qDebug() << "controllers null"; return; }

    float dt = lastTime.restart() / 1000.0f;
    m_gameTime          += dt;
    m_timeSinceLastDrop += dt;
    if (currentSession.mode == GameSession::Mode::ENDLESS) {
        m_endlessAiModifierTimer += dt;
           // 动态间隔：基础 25s ± 5-10s 的随机变化
           float dynamicInterval = ENDLESS_AI_MODIFIER_INTERVAL + (QRandomGenerator::global()->bounded(10LL) - 5.f);
           if (m_endlessAiModifierTimer >= dynamicInterval) {
            m_endlessAiModifierTimer = 0.f;
            applyRandomModifierToAI();
        }
    }
    //qDebug() << dt;
    if (dt > 0.05) qDebug() << 1 / dt;
    if (QRandomGenerator::global()->bounded(1000) < 3) {
        spawnDrop(); // 每帧尝试生成掉落物
    }
    updateDrops(dt);
    if (players[0]->isDead()) {
        gameEnd(1);
        return;
    }
    if (players[1]->isDead()) {
        gameEnd(0);
        return;
    }
    players[0]->setDt(dt); // 万一卡顿根据真实帧数设置dt
    players[1]->setDt(dt);



    if(currentSession.mode == GameSession::Mode::AI || currentSession.mode == GameSession::Mode::ENDLESS){
        MoveIntent moveIntent = MoveIntent::NONE;
        AttackIntent attackIntent = false;
        
        // 更新AI的武器槽位信息
        ai->updateWeaponSlotInfo(players[1]->weaponSlots, players[1]->activeSlotIndex);
        
        ai->updateIntent(moveIntent, attackIntent);
        
        // 武器切槽意图处理
        int targetSlot = ai->consumeWeaponSwitchIntent();
        if (targetSlot >= 0 && targetSlot < players[1]->weaponSlots.size()) {
            players[1]->activeSlotIndex = targetSlot;
            players[1]->weapon = players[1]->weaponSlots[targetSlot];
            qDebug() << "AI switched to weapon slot:" << targetSlot;
        }
        
        if (ai->consumeSpellCastIntent()) {
            tryActivateSpell(1);
        }
        if (currentScene == "grass" && moveIntent == MoveIntent::NONE){
            moveIntent = MoveIntent::CROUCH; // 草地背景下不动就蹲下
        }
        intent[1].moveIntent = moveIntent;
        intent[1].attackIntent = attackIntent;
        //qDebug() << moveIntent << attackIntent;
    }
    controllers[0]->handleIntent(intent[0].moveIntent, intent[0].attackIntent);
    controllers[1]->handleIntent(intent[1].moveIntent, intent[1].attackIntent);
    cm.checkPlayerVsPlayerCollision(controllers[0].get(), controllers[1].get());
    updateBalls(dt);
    tickSpells(dt);   // 检查法术冷却
    //qDebug() << "Entity count: " << entities.size();
    entities.erase(std::remove_if(entities.begin(), entities.end(),
                               [](const std::shared_ptr<Entity>& e) {
                                   return e->shouldBeRemoved();
                               }), entities.end());
    drops.erase(std::remove_if(drops.begin(), drops.end(), [](auto& w) {
                    return w.expired(); // ✅ 清除掉失效的 weak_ptr
                }), drops.end());
    balls.erase(std::remove_if(balls.begin(), balls.end(), [](auto& w) {
                    return w.expired(); // ✅ 清除掉失效的 weak_ptr
                }), balls.end());
    bullets.erase(std::remove_if(bullets.begin(), bullets.end(), [](auto& w) {
                    return w.expired(); // ✅ 清除掉失效的 weak_ptr
                }), bullets.end());
    update();  // 触发 paintEvent()
    intent[1].attackIntent = false;
    intent[0].attackIntent = false;
}

GameSession::Spell Widget::resolveAISpellForMode() const {
    if (currentSession.aiFreezePilotOnly) {
        return GameSession::Spell::FREEZE;
    }

    auto randomPick = []() {
        const GameSession::Spell pool[] = {
            GameSession::Spell::FREEZE,
            GameSession::Spell::STEALTH,
            GameSession::Spell::IRON_BODY,
        };
        int idx = QRandomGenerator::global()->bounded(3);
        return pool[idx];
    };

    // 无尽默认锁定法术，避免局中切法术造成体感不公平。
    if (currentSession.mode == GameSession::Mode::ENDLESS && currentSession.endlessLockAiSpell) {
        if (currentSession.aiSpellPolicy == GameSession::AiSpellPolicy::CONFIG_PICK &&
            currentSession.spellP2 != GameSession::Spell::NONE) {
            return currentSession.spellP2;
        }
        if (currentSession.aiSpellPolicy == GameSession::AiSpellPolicy::COUNTER_PICK) {
            return weightedCounterPick(currentSession.spellP1);
        }
        return randomPick();
    }

    switch (currentSession.aiSpellPolicy) {
    case GameSession::AiSpellPolicy::CONFIG_PICK:
        return (currentSession.spellP2 == GameSession::Spell::NONE)
            ? GameSession::Spell::FREEZE
            : currentSession.spellP2;
    case GameSession::AiSpellPolicy::COUNTER_PICK:
        return weightedCounterPick(currentSession.spellP1);
    case GameSession::AiSpellPolicy::RANDOM_PICK:
    default:
        return randomPick();
    }
}

void Widget::spawnDrop() {
    // ── 判断是否触发掉落 ─────────────────────────────────────
    bool guaranteed = m_timeSinceLastDrop >= DROP_GUARANTEE_TIME;
    bool random     = QRandomGenerator::global()->bounded(1000) < DROP_CHANCE_PER_FRAME;
    if (!guaranteed && !random) return;

    // ── 选择当前阶段配置 ─────────────────────────────────────
    int phaseIndex = 0;
    if      (m_gameTime >= 40.f) phaseIndex = 2;
    else if (m_gameTime >= 20.f) phaseIndex = 1;

    // 无尽模式：循环阶段（每90秒一轮）
    if (currentSession.mode == GameSession::Mode::ENDLESS) {
        float loopTime = fmod(m_gameTime, 90.f);
        if      (loopTime >= 60.f) phaseIndex = 2;
        else if (loopTime >= 30.f) phaseIndex = 1;
        else                       phaseIndex = 0;
    }

    const DropConfig& cfg = PHASE_CONFIGS[phaseIndex];

    // ── 决定大类 ─────────────────────────────────────────────
    int categoryRoll = QRandomGenerator::global()->bounded(100);
    QString itemName;

    int weaponEnd   = cfg.weaponChance;
    int armorEnd    = weaponEnd   + cfg.armorChance;
    int modifierEnd = armorEnd    + cfg.modifierChance;
    int medicineEnd = modifierEnd + cfg.medicineChance;

    if (categoryRoll < weaponEnd) {
        // ── 武器子类 ─────────────────────────────────────────
        int r = QRandomGenerator::global()->bounded(100);
        int knifeEnd  = cfg.knifeChance;
        int ballEnd   = knifeEnd  + cfg.ballChance;
        int rifleEnd  = ballEnd   + cfg.rifleChance;

        if      (r < knifeEnd)  itemName = "knife";
        else if (r < ballEnd)   itemName = "ball";
        else if (r < rifleEnd)  itemName = "rifle";
        else                    itemName = "sniper";

    } else if (categoryRoll < armorEnd) {
        // ── 护甲子类 ─────────────────────────────────────────
        itemName = (QRandomGenerator::global()->bounded(2) == 0)
                       ? "chainmail" : "vest";

    } else if (categoryRoll < modifierEnd) {
        itemName = "modifier";

    } else if (categoryRoll < medicineEnd) {
        // ── 药品子类 ─────────────────────────────────────────
        int r = QRandomGenerator::global()->bounded(100);
        if      (r < 50) itemName = "bandage";
        else if (r < 70) itemName = "medkit";
        else             itemName = "adrenaline";

    } else {
        return; // 剩余概率：不掉落
    }

    // ── 生成掉落物 ───────────────────────────────────────────
    float x = QRandomGenerator::global()->bounded(100, 900);
    auto drop = std::make_shared<DropItem>(x, itemName);
    entities.push_back(drop);
    drops.push_back(drop);

    m_timeSinceLastDrop = 0.f;  // 重置保底计时器

    qDebug().noquote() << QString("[掉落] %1 | 阶段%2 | 游戏时间%3s %4")
                              .arg(itemName)
                              .arg(phaseIndex)
                              .arg(m_gameTime, 0, 'f', 1)
                              .arg(guaranteed ? "(保底)" : "");
}

void Widget::updateDrops(float dt){
    for (auto& drop : drops) {
        if (drop.lock() == nullptr){
            return;
        }
        if (drop.lock()->isCollectedBy(players[0].get()) && intent[0].moveIntent == MoveIntent::CROUCH) {
            drop.lock()->markForDeletion();  // ⚠️ 不立即删，留给后面统一处理
            if (drop.lock()->itemType == "modifier"){
                qDebug()<<"modifier collected!";
                triggerModifierChoice(0);
                break;
            }
            players[0]->weaponControll(drop.lock()->itemType);
        }
        else if (drop.lock()->isCollectedBy(players[1].get()) && intent[1].moveIntent == MoveIntent::CROUCH) {
            drop.lock()->markForDeletion();  // ⚠️ 不立即删，留给后面统一处理
            if (drop.lock()->itemType == "modifier"){
                // 修改：允许AI在PVP和ENDLESS模式下拾取修饰符
                // 在PVP模式下需要触发选择UI，在ENDLESS模式下直接应用
                if (currentSession.mode == GameSession::Mode::PVP){
                    triggerModifierChoice(1); // PVP模式：触发UI让玩家选择
                    break;
                }
                else if (currentSession.mode == GameSession::Mode::ENDLESS){
                    // ENDLESS模式：AI自动选择最优修饰符
                    applyBestModifierToAI();
                    break;
                }
            }
            players[1]->weaponControll(drop.lock()->itemType);
        }
        drop.lock()->setDt(dt);
        drop.lock()->update();
    }
    updateAIInfo();
}

void Widget::updateBalls(float dt){
    for (auto& ball : balls) {
        if (ball.lock() == nullptr) return;
        ball.lock()->setDt(dt);
        ball.lock()->update();
        cm.checkEntityVsPlayerCollision(ball.lock().get(), controllers[0].get());
        cm.checkEntityVsPlayerCollision(ball.lock().get(), controllers[1].get());
    }
    for (auto& bullet : bullets) {
        if (bullet.lock() == nullptr) return;
        bullet.lock()->setDt(dt);
        bullet.lock()->update();
        cm.checkEntityVsPlayerCollision(bullet.lock().get(), controllers[0].get());
        cm.checkEntityVsPlayerCollision(bullet.lock().get(), controllers[1].get());
    }
}

Widget::~Widget()
{
    delete ui;
}

void Widget::gameEnd(int id)
{
    if (currentSession.mode == GameSession::Mode::ENDLESS && players.size() >= 2) {
        currentSession.survivalTime = m_gameTime;
        currentSession.totalDamageDealt =
            cm.stats().byPlayerAndWeapon[0][0] + cm.stats().byPlayerAndWeapon[0][1] +
            cm.stats().byPlayerAndWeapon[0][2] + cm.stats().byPlayerAndWeapon[0][3] +
            cm.stats().byPlayerAndWeapon[0][4] + cm.stats().byPlayerAndWeapon[0][5];
        qDebug().noquote() << QString("[无尽结算] 生存:%1s 总伤害:%2")
                                  .arg(currentSession.survivalTime, 0, 'f', 1)
                                  .arg(currentSession.totalDamageDealt, 0, 'f', 1);
        qDebug().noquote() << QString("[无尽结算] P1伤害 拳:%1 刀:%2 球:%3 步:%4 狙:%5 其他:%6")
                                  .arg(cm.stats().byPlayerAndWeapon[0][0], 0, 'f', 1)
                                  .arg(cm.stats().byPlayerAndWeapon[0][1], 0, 'f', 1)
                                  .arg(cm.stats().byPlayerAndWeapon[0][2], 0, 'f', 1)
                                  .arg(cm.stats().byPlayerAndWeapon[0][3], 0, 'f', 1)
                                  .arg(cm.stats().byPlayerAndWeapon[0][4], 0, 'f', 1)
                                  .arg(cm.stats().byPlayerAndWeapon[0][5], 0, 'f', 1);
        qDebug().noquote() << QString("[无尽结算] P2词条: %1")
                                  .arg(players[1]->getModifierSummary().join(" | "));
        qDebug().noquote() << QString("[无尽结算] P1词条: %1")
                                  .arg(players[0]->getModifierSummary().join(" | "));
        qDebug().noquote() << QString("[无尽结算] 游戏时间:%1s").arg(m_gameTime, 0, 'f', 1);
        
        // 构建结算数据并显示UI
        EndlessGameResult result;
        result.survivalTime = currentSession.survivalTime;
        result.totalDamage = currentSession.totalDamageDealt;
        result.spellPlayer1 = spellName(currentSession.spellP1);
        result.spellPlayer2 = spellName(currentSession.spellP2);
        
        // 复制伤害分布
        for (int i = 0; i < 6; ++i) {
            result.damageByWeapon[0][i] = cm.stats().byPlayerAndWeapon[0][i];
            result.damageByWeapon[1][i] = cm.stats().byPlayerAndWeapon[1][i];
        }
        
        // 获取词条摘要
        result.modifiersPlayer1 = players[0]->getModifierSummary();
        result.modifiersPlayer2 = players[1]->getModifierSummary();
        
        // 先判定是否破纪录，再写入记录，避免比较基准被刚写入的新值覆盖
        result.isNewRecord = recordManager.isNewRecord(result.survivalTime, result.totalDamage);
        recordManager.updateIfBetter(result.survivalTime, result.totalDamage);
        const EndlessRecord& record = recordManager.getRecord();
        result.bestSurvivalTime = record.maxSurvivalTime;
        result.bestTotalDamage = record.maxTotalDamage;
        
        if (result.isNewRecord) {
            qDebug() << "[无尽结算] 🎉 新纪录！生存:" << result.survivalTime 
                     << "秒，伤害:" << result.totalDamage;
        }
        
        // 停掉计时器
        if (timer) {
            timer->stop();
            disconnect(timer, nullptr, this, nullptr);
        }
        
        // 抢键盘焦点
        grabKeyboard();
        
        // 显示结算UI
        endlessResultWidget->showResult(result);

        // 防止多次结束时重复连接
        disconnect(endlessResultWidget, nullptr, this, nullptr);
        
        // 连接按钮信号到重新开始/返回菜单
        connect(endlessResultWidget, &EndlessResultWidget::restartRequested, this, [=]() {
            releaseKeyboard();
            disconnect(endlessResultWidget, nullptr, this, nullptr);
            endlessResultWidget->hide();
            resetGame(currentSession);
            setFocus();
        });
        
        connect(endlessResultWidget, &EndlessResultWidget::menuRequested, this, [=]() {
            releaseKeyboard();
            disconnect(endlessResultWidget, nullptr, this, nullptr);
            endlessResultWidget->hide();
            emit matchResultConfirmed();  // 返回菜单
        });
        
        return;
    }
    
    // 非无尽模式流程：显示原有的Game Over界面
    // 1. 停掉计时器，断掉信号，避免旧逻辑还在跑
    if (timer) {
        timer->stop();
        disconnect(timer, nullptr, this, nullptr);
    }

    // 2. 不要提前清空 players/entities！
    //    先留着，paintEvent 还能画最后一帧完整画面
    // controllers.clear();
    // players.clear();
    // entities.clear();

    // 3. 叠加 Game Over 遮罩
    QLabel *overlay = new QLabel(this);
    overlay->setGeometry(rect());
    overlay->setStyleSheet(
        "background: rgba(0, 0, 0, 160);"
        "color: white;"
        "font-size: 40px;"
        "font-weight: bold;"
        );
    overlay->setAlignment(Qt::AlignCenter);
    overlay->setText("玩家"+ QString::number(2-id) +"死亡！\n\nGAME OVER\n\n按任意键重新开始");
    overlay->show();

    // 4. 抢键盘焦点，捕捉“任意键”
    grabKeyboard();

    connect(this, &Widget::keyPressed, overlay, [=]() {
        disconnect(this, &Widget::keyPressed, overlay, nullptr);
        releaseKeyboard();
        overlay->hide(); // 重新开始后遮罩解除
        overlay->deleteLater();
        QTimer::singleShot(0, this, [=]() {
            emit roundEnded(id);
        });
    });
}

void Widget::onPlayerRequest(float x, float y, float vx, float vy, Player::WeaponType weapon, float initDamage, float frozenBonus, int id) {
    //qDebug() <<"【onplayer】发射者： "<<id;
    if (weapon == Player::WeaponType::ball){
        if (vx > 0){
            vx += 500;
        }
        else if (vx < 0){
            vx -= 500;
        }
        /*if (vy > 0){
        vy += 200;
        } // 改动y值反而失去了趣味性，命中率感人
        else{
            vy -= 500;
        }*/
        auto ball = std::make_shared<Ball>(QPointF(x, y), QPointF(vx, vy), initDamage, frozenBonus, id);
        entities.push_back(ball);
        balls.push_back(ball);
    }
    else if (weapon == Player::WeaponType::rifle){
        if (vx >= 0){
            vx = 2000;
        }
        else if (vx < 0){
            vx = -2000;
        }
        auto bullet = std::make_shared<Bullet>(QPointF(x, y), vx, 1, initDamage, frozenBonus, id);
        entities.push_back(bullet);
        bullets.push_back(bullet);
    }
    else if (weapon == Player::WeaponType::sniper){
        if (vx >= 0){
            vx = 3000;
        }
        else if (vx < 0){
            vx = -3000;
        }
        auto bullet = std::make_shared<Bullet>(QPointF(x, y), vx, 2, initDamage, frozenBonus, id);
        entities.push_back(bullet);
        bullets.push_back(bullet);
    }
}

void Widget::updateAIInfo()
{
    if (!ai) return;

    // 收集掉落物信息
    QVector<GameAI::DropInfo> dropInfos;
    for (const auto& weakDrop : drops) {
        if (auto drop = weakDrop.lock()) {
            GameAI::DropInfo info;
            info.position = drop->getPos();
            info.itemType = drop->itemType;
            dropInfos.append(info);
        }
    }

    // 更新AI的掉落物信息
    ai->updateDropsInfo(dropInfos);

    // 更新AI玩家的当前武器（假设player2是AI）
    auto wp = players[1]->weapon;
    if (wp == Player::WeaponType::punch){
        ai->setCurrentWeapon("punch");
    }
    else if (wp == Player::WeaponType::knife){
        ai->setCurrentWeapon("knife");
    }
    else if (wp == Player::WeaponType::ball){
        ai->setCurrentWeapon("ball");
    }
    else if (wp == Player::WeaponType::rifle){
        ai->setCurrentWeapon("rifle");
    }
    else if (wp == Player::WeaponType::sniper){
        ai->setCurrentWeapon("sniper");
    }
}

// DropItem 被捡起时调用（在 updateDrops 里判断 itemType == "modifier"）
void Widget::triggerModifierChoice(int playerIndex) {
    // 暂停游戏
    timer->stop();

    // 获取该玩家的法术名（GameSession里存着）
    QString spellName = (playerIndex == 0)
                            ? spellEnumToString(currentSession.spellP1)
                            : spellEnumToString(currentSession.spellP2);

    // 从词条池随机抽3张（去掉已是唯一且已拥有的）
    QVector<ModifierData> pool = filteredModifierPoolForPlayer(playerIndex, false);

    if (pool.size() < 3) {
        // 词条池不足3张（极端情况），直接恢复游戏
        timer->start();
        return;
    }

    // 随机不重复抽3张
    QVector<ModifierData> options;
    QVector<int> indices;
    while (indices.size() < 3) {
        int idx = QRandomGenerator::global()->bounded(pool.size());
        if (!indices.contains(idx)) {
            indices.append(idx);
            options.append(pool[idx]);
        }
    }

    pendingModifierPlayer = playerIndex;
    modifierOverlay->showOptions(playerIndex, options);
}

QVector<ModifierData> Widget::filteredModifierPoolForPlayer(int playerIndex, bool excludeMaxHpUp) const {
    QString spellName = (playerIndex == 0)
                            ? spellEnumToString(currentSession.spellP1)
                            : spellEnumToString(currentSession.spellP2);
    QVector<ModifierData> pool = ModifierData::poolForPlayer(spellName);
    auto& p = players[playerIndex];

    pool.erase(std::remove_if(pool.begin(), pool.end(),
                              [&](const ModifierData& m) {
                                  if (excludeMaxHpUp && m.type == ModifierType::MAX_HP_UP) return true;
                                  if (!isUniqueModifier(m.type)) return false;
                                  return playerHasUniqueModifier(p, m.type);
                              }), pool.end());
    return pool;
}

void Widget::applyRandomModifierToAI() {
    if (players.size() < 2) return;
    constexpr int aiIndex = 1;
    QVector<ModifierData> pool = filteredModifierPoolForPlayer(aiIndex, true);
    if (pool.isEmpty()) return;

    int idx = QRandomGenerator::global()->bounded(pool.size());
    const ModifierData chosen = pool[idx];
    players[aiIndex]->applyModifier(chosen);
    qDebug().noquote() << QString("[无尽AI词条] 获得：%1").arg(chosen.displayName);
}

void Widget::onModifierChosen(const ModifierData& chosen) {
    if (pendingModifierPlayer < 0) return;

    auto& myPlayer  = players[pendingModifierPlayer];
    auto& oppPlayer = players[1 - pendingModifierPlayer];

    // 对手词条：作用在对手身上
    if (chosen.type == ModifierType::ENEMY_MAX_HP_DOWN ||
        chosen.type == ModifierType::ENEMY_MOVE_SPEED_DOWN) {
        oppPlayer->applyEnemyModifier(chosen);
    } else {
        myPlayer->applyModifier(chosen);
    }

    pendingModifierPlayer = -1;
    lastTime.restart(); // 重置计时器，避免因暂停导致的dt过大
    timer->start();  // 恢复游戏
}

void Widget::tryActivateSpell(int playerIndex) {
    auto& p  = players[playerIndex];
    auto& ss = p->spellState;

    GameSession::Spell spell = (playerIndex == 0)
                                   ? currentSession.spellP1
                                   : currentSession.spellP2;

    if (spell == GameSession::Spell::NONE) return;
    if (ss.isOnCooldown()) return;

    // ── 通用检查：被定身时不能释放任何法术 ─────────────────
    if (p->spellState.isFrozen) return;

    // ── 前置条件检查 ─────────────────────────────────────────
    auto& ctrl = controllers[playerIndex];
    bool inHurt = ctrl->isInHurt(); // 见第6节，PlayerController新增方法

    // 定身：非硬直状态才可用
    if (spell == GameSession::Spell::FREEZE && inHurt) return;

    // 隐身：非硬直状态才可用（定身检查已在上方，此处可省略）
    if (spell == GameSession::Spell::STEALTH && inHurt) return;

    // ── 激活法术 ─────────────────────────────────────────────
    switch (spell) {

    case GameSession::Spell::FREEZE: {
        SoundManager::instance().play("freeze", 0.7);
        // 对手进入定身
        int oppIdx = 1 - playerIndex;
        auto& opp  = players[oppIdx];
        auto& oppSS= opp->spellState;

        float duration = 5.f + p->modifiers.freezeDurationBonus;
        float breakThresh = opp->maxHp * 0.2f; // maxHp的20%破定身

        oppSS.isFrozen          = true;
        oppSS.frozenRemain      = duration;
        oppSS.frozenBreakHpLeft = breakThresh;

        // 对手速度清零、解除隐身
        opp->vx = 0;
        opp->vy = 0;
        if (oppSS.stealthActive) {
            oppSS.stealthActive = false;
            oppSS.stealthRemain = 0.f;
        }

        // 施法方进入冷却
        float cd = 40.f - p->modifiers.spellCooldownReduce;
        ss.cooldownMax    = qMax(5.f, cd);
        ss.cooldownRemain = ss.cooldownMax;
        break;
    }

    case GameSession::Spell::STEALTH: {
        SoundManager::instance().play("stealth", 10);
        float duration = 15.f + p->modifiers.stealthDurationBonus;
        if (p->modifiers.stealthFirstHit) duration /= 2.f; // 破影减半时长

        ss.stealthActive       = true;
        ss.stealthRemain       = duration;
        ss.stealthFirstHitUsed = false;  // 每次激活重置

        float cd = 45.f - p->modifiers.spellCooldownReduce;
        ss.cooldownMax    = qMax(5.f, cd);
        ss.cooldownRemain = ss.cooldownMax;
        break;
    }

    case GameSession::Spell::IRON_BODY: {
        SoundManager::instance().play("tongtou", 0.7);
        ss.ironBodyActive = true;
        ss.ironBodyRemain = 5.f + p->modifiers.ironBodyDurationBonus;
        ss.ironBodyCdrUsed = false;
        if (p->modifiers.ironBodyHardened) {
            p->selfvelocityratio = p->velocityratio * p->modifiers.moveSpeedMultiplier * 1.3f;
        }
        float cd = 20.f - p->modifiers.spellCooldownReduce;
        ss.cooldownMax    = qMax(5.f, cd);
        ss.cooldownRemain = ss.cooldownMax;
        break;
    }

    // ── 后续法术接口预留 ─────────────────────────────────────
    case GameSession::Spell::BARRIER:
    case GameSession::Spell::CLONE:
    case GameSession::Spell::FORBIDDEN:
        // TODO: 后续实现
        break;

    default: break;
    }
}

void Widget::tickSpells(float dt) {
    for (int i = 0; i < players.size(); ++i) {
        auto& p  = players[i];
        auto& ss = p->spellState;

        // ── 隐身回血（词条效果）──────────────────────────────
        if (ss.stealthActive && p->modifiers.stealthRegenPerSec > 0.f) {
            p->hp = qMin(p->maxHp,
                         p->hp + p->modifiers.stealthRegenPerSec * dt);
        }
        if (!ss.ironBodyActive && p->modifiers.ironBodyHardened) {
            p->selfvelocityratio = p->velocityratio * p->modifiers.moveSpeedMultiplier;
        }

        // ── tick 推进所有计时器（内部自动关闭到期状态）───────
        ss.tick(dt);
    }
}
