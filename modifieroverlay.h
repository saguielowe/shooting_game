#pragma once
#include <QWidget>
#include <QVector>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include "modifierdata.h"

// ============================================================
//  ModifierOverlay
//  在 Widget 构造时 new 一次，之后反复 showOptions() / hide()
//  选完后 emit optionChosen(ModifierData)
//  10秒倒计时到0自动随机选一个
// ============================================================

class ModifierOverlay : public QWidget {
    Q_OBJECT
public:
    explicit ModifierOverlay(QWidget* parent = nullptr);

    // 展示三个词条选项，同时开始10秒倒计时
    // playerIndex: 0 或 1，显示在标题上
    void showOptions(int playerIndex, const QVector<ModifierData>& options);

signals:
    void optionChosen(const ModifierData& chosen);

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private slots:
    void onTimerTick();
    void selectOption(int index);

private:
    void updateCountdownLabel();
    void buildLayout();

    // UI元素
    QLabel*      titleLabel;
    QLabel*      countdownLabel;

    // 三张卡片，每张：外框QWidget + 名称Label + 描述Label + 提示Label
    struct Card {
        QWidget* frame;
        QLabel*  nameLabel;
        QLabel*  descLabel;
        QLabel*  hintLabel;
    };
    Card cards[3];

    // 数据
    QVector<ModifierData> currentOptions;
    int  remainSeconds = 10;
    int  playerIndex   = 0;
    QTimer* countdownTimer;
};
