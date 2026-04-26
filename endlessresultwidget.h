#ifndef ENDLESSRESULTWIDGET_H
#define ENDLESSRESULTWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVector>
#include <QString>
#include <QPixmap>

// ============================================================
//  EndlessResultWidget — 无尽模式结算界面
//  显示生存时间、总伤害、伤害分布、双方词条
// ============================================================

struct EndlessGameResult {
    float survivalTime = 0.f;
    float totalDamage = 0.f;
    QString spellPlayer1;
    QString spellPlayer2;
    
    // 伤害分布：[玩家][武器类型]
    // 分类顺序: 0=拳, 1=刀, 2=球, 3=步枪, 4=狙击枪, 5=其他
    float damageByWeapon[2][6] = { {0}, {0} };
    
    // 词条摘要（字符串列表）
    QStringList modifiersPlayer1;
    QStringList modifiersPlayer2;
    
    // 最佳纪录对比（可选，外部填充）
    float bestSurvivalTime = 0.f;
    float bestTotalDamage = 0.f;
    bool isNewRecord = false;
};

class EndlessResultWidget : public QWidget {
    Q_OBJECT

public:
    explicit EndlessResultWidget(QWidget* parent = nullptr);
    
    // 设置结算数据并显示
    void setResult(const EndlessGameResult& result);
    
    // 模态显示（类似 ModifierOverlay）
    void showResult(const EndlessGameResult& result);

signals:
    void restartRequested();   // 用户点"重新开始"
    void menuRequested();      // 用户点"返回菜单"

protected:
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    void buildLayout();
    void updateContent();
    QPixmap buildDamagePiePixmap() const;
    
    // UI 元素
    QLabel* titleLabel;
    QLabel* statsLabel;        // 生存时间、总伤害
    QLabel* recordLabel;       // 最佳纪录 / 新纪录提示
    QLabel* spellLabel;        // 玩家法术信息
    QLabel* damagePieLabel;    // 饼图展示
    QLabel* modifiersLabel;     // 词条信息（滚动文本）
    QPushButton* btnRestart;
    QPushButton* btnMenu;
    
    // 记录当前数据
    EndlessGameResult currentResult;
};

#endif // ENDLESSRESULTWIDGET_H
