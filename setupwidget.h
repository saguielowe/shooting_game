#ifndef SETUPWIDGET_H
#define SETUPWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QButtonGroup>
#include <QSpinBox>
#include <QComboBox>
#include <QGroupBox>
#include <QTextEdit>
#include "gamesession.h"
#include "endlessrecordmanager.h"

// ============================================================
//  SetupWidget — 开局配置页面（嵌在 QStackedWidget 里）
//  用户选择模式、局数、法术后点"开始"，
//  发出 sessionReady 信号，MainWindow 切换到 GameWidget
// ============================================================

class SetupWidget : public QWidget {
    Q_OBJECT

public:
    explicit SetupWidget(QWidget* parent = nullptr);

    // 把当前界面选择的参数打包成 GameSession 返回
    GameSession buildSession() const;

signals:
    void sessionReady(GameSession session);  // 点"开始"时发出
    void backToMenu();                       // 点"返回"时发出

private slots:
    void onModeChanged(int id);   // 模式切换时更新法术可选项
    void onStartClicked();

public slots:
    void refreshEndlessRecordDisplay();

private:
    void buildUI();
    void updateSpellOptions();    // 根据模式过滤法术列表

    // ---- 模式选择 ----------------------------------------
    QButtonGroup* modeGroup;
    QPushButton*  btnAI;
    QPushButton*  btnPVP;
    QPushButton*  btnEndless;

    // ---- 局数选择 ----------------------------------------
    QButtonGroup* bestOfGroup;
    QGroupBox*    bestOfBox;
    QPushButton*  btnBO1;
    QPushButton*  btnBO3;
    QPushButton*  btnBO5;
    QPushButton*  btnBO7;
    QSpinBox*     spinCustom;     // 自定义局数

    // ---- 法术选择 ----------------------------------------
    QComboBox*    comboSpellP1;
    QComboBox*    comboSpellP2;
    QLabel*       labelSpellP2;   // 双人时显示，人机时隐藏

    // ---- 操作按钮 ----------------------------------------
    QPushButton*  btnStart;
    QPushButton*  btnBack;
    QTextEdit*    endlessRules;
    QLabel*       labelBestRecord;   // 无尽模式最佳纪录显示

    // ---- 当前模式缓存 ------------------------------------
    GameSession::Mode currentMode = GameSession::Mode::AI;
    
    // ---- 成绩管理 ----------------------------------------
    EndlessRecordManager recordManager;
};

#endif // SETUPWIDGET_H
