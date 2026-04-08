#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QTimer>
#include <QElapsedTimer>
#include <QKeyEvent>
#include <QPainter>
#include "player.h"
#include "ball.h"
#include "map.h"
#include "playercontroller.h"
#include "gamesession.h"
#include "combatmanager.h"
#include "dropitem.h"
#include "bullet.h"
#include <QRandomGenerator>
#include "gameai.h"
#include "modifieroverlay.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(const GameSession& session, QWidget *parent = nullptr);
    void resetGame(const GameSession& session);
    void showMatchResult(const QString& text);
    void updateDrops(float dt);
    void updateBalls(float dt);
    void drawDrops(QPainter& painter);
    QVector<QPair<QPointF, QString>> getAvailableDrops() const;
    ~Widget();

protected:
    void paintEvent(QPaintEvent*) override;
    void keyPressEvent(QKeyEvent*) override;
    void keyReleaseEvent(QKeyEvent*) override;

private slots:
    void gameLoop();
    void onPlayerRequest(float x, float y, float vx, float vy, Player::WeaponType weapon, float initDamage, float frozenBonus, int id);
signals:
    void keyPressed();
    void roundEnded(int winnerId);  // 一局结束，传赢家 id
    void matchResultConfirmed(); // 整场结束，玩家确认结果，准备返回菜单
private:
    void applySession(const GameSession& session);
    void initGame(); // 读取game session信息后，给玩家赋初值。
    void cleanupGame();
    void spawnDrop();
    void updateAIInfo();
    void gameEnd(int id);
    void updateIntents();
    void triggerModifierChoice(int player);
    QVector<ModifierData> filteredModifierPoolForPlayer(int playerIndex, bool excludeMaxHpUp) const;
    void applyRandomModifierToAI();
    void onModifierChosen(const ModifierData& chosen);
    void tryActivateSpell(int playerIndex);
    void tickSpells(float dt);

    GameSession currentSession;
    ModifierOverlay* modifierOverlay;   // 构造时 new 一次
    int pendingModifierPlayer = -1;
    struct inputIntent {
        MoveIntent moveIntent;
        bool attackIntent;
    };
    inputIntent intent[2];

    QVector<std::shared_ptr<Player>> players;
    QVector<std::shared_ptr<PlayerController>> controllers;

    QVector<std::weak_ptr<Ball>> balls;
    QVector<std::weak_ptr<DropItem>> drops;
    QVector<std::weak_ptr<Bullet>> bullets;
    QVector<std::shared_ptr<Entity>> entities;

    QTimer* timer;
    QElapsedTimer lastTime;
    float m_gameTime        = 0.f;  // 当前局内时间（秒）
    float m_timeSinceLastDrop = 0.f; // 距上次掉落经过的时间
    static const float DROP_GUARANTEE_TIME; // 保底时间，20秒
    static const float ENDLESS_AI_MODIFIER_INTERVAL;
    float m_endlessAiModifierTimer = 0.f;
    CombatManager cm;

    QString currentScene;
    bool keyLeft = false, keyRight = false, crouching = false; // 辅助变量来判断玩家的按键输入对应哪种状态
    std::shared_ptr<GameAI> ai = std::make_shared<GameAI>(); // 赋空初始值
    Ui::Widget *ui;

    int m_maxHp, m_maxBalls, m_maxBullets, m_maxSnipers;
    QPixmap scenePixmap;

    QSet<int> pressedKeys;
    void drawStatusBar(QPainter& painter, int playerIndex);
    QString spellName(GameSession::Spell spell) const;
};
#endif // WIDGET_H
