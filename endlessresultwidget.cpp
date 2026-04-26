#include "endlessresultwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QKeyEvent>
#include <QScroller>
#include <QScrollArea>
#include <QTextEdit>
#include <QPixmap>

// 样式常量
static const int   RESULT_W      = 800;
static const int   RESULT_H      = 640;
static const QColor BG_COLOR     = QColor(10, 10, 20, 240);
static const QColor BORDER_COLOR = QColor(120, 120, 200);
static const QColor TEXT_TITLE   = QColor(220, 220, 255);
static const QColor TEXT_NORMAL  = QColor(170, 170, 200);
static const QColor TEXT_ACCENT  = QColor(100, 200, 100);
static const QColor RECORD_COLOR = QColor(255, 215, 0);  // 金色

// 武器名称映射
static const QString DAMAGE_NAMES[6] = {
    "拳", "刀", "球", "步", "狙", "其他"
};

// 武器颜色
static const QColor DAMAGE_COLORS[6] = {
    QColor(255, 100, 100),  // 拳 - 红
    QColor(200, 100, 255),  // 刀 - 紫
    QColor(100, 150, 255),  // 球 - 蓝
    QColor(100, 255, 150),  // 步 - 绿
    QColor(255, 200, 100),  // 狙 - 橙
    QColor(160, 160, 160)   // 其他 - 灰
};

EndlessResultWidget::EndlessResultWidget(QWidget* parent)
    : QWidget(parent)
{
    setFixedSize(RESULT_W, RESULT_H);
    setAttribute(Qt::WA_TranslucentBackground);
    setFocusPolicy(Qt::StrongFocus);
    hide();
    
    buildLayout();
}

void EndlessResultWidget::buildLayout() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(40, 20, 40, 24);
    mainLayout->setSpacing(15);
    
    // ===== 标题 =====
    titleLabel = new QLabel(this);
    titleLabel->setText("无尽挑战 结算");
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(
        "font-size: 28px; font-weight: bold; color: rgba(220,220,255,255);"
    );
    mainLayout->addWidget(titleLabel);
    
    // ===== 核心统计数据 =====
    statsLabel = new QLabel(this);
    statsLabel->setAlignment(Qt::AlignCenter);
    statsLabel->setStyleSheet(
        "font-size: 16px; color: rgba(170,170,200,255); "
        "background: rgba(40,40,70,200); border-radius: 8px; padding: 12px;"
    );
    mainLayout->addWidget(statsLabel);

    recordLabel = new QLabel(this);
    recordLabel->setAlignment(Qt::AlignCenter);
    recordLabel->setWordWrap(true);
    recordLabel->setStyleSheet(
        "font-size: 13px; color: rgba(255,215,0,255); "
        "background: rgba(40,40,70,200); border-radius: 8px; padding: 10px;"
    );
    mainLayout->addWidget(recordLabel);

    spellLabel = new QLabel(this);
    spellLabel->setAlignment(Qt::AlignCenter);
    spellLabel->setStyleSheet(
        "font-size: 14px; color: rgba(220,220,255,255); "
        "background: rgba(35,35,60,200); border-radius: 8px; padding: 10px;"
    );
    mainLayout->addWidget(spellLabel);

    damagePieLabel = new QLabel(this);
    damagePieLabel->setAlignment(Qt::AlignCenter);
    damagePieLabel->setMinimumHeight(170);
    damagePieLabel->setStyleSheet(
        "background: rgba(35,35,60,200); border-radius: 8px; padding: 8px;"
    );
    mainLayout->addWidget(damagePieLabel);
    
    // ===== 词条信息（可滚动） =====
    QLabel* modifiersTitle = new QLabel("双方词条", this);
    modifiersTitle->setStyleSheet("font-size: 14px; color: rgba(220,220,255,255); font-weight: bold;");
    mainLayout->addWidget(modifiersTitle);
    
    modifiersLabel = new QLabel(this);
    modifiersLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    modifiersLabel->setWordWrap(true);
    modifiersLabel->setStyleSheet(
        "font-size: 12px; color: rgba(170,170,200,255); "
        "background: rgba(40,40,70,200); border-radius: 8px; padding: 10px;"
    );
    modifiersLabel->setMinimumHeight(120);
    mainLayout->addWidget(modifiersLabel, 1);
    
    // ===== 按钮区 =====
    auto* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(20);
    
    btnRestart = new QPushButton("重新开始", this);
    btnRestart->setStyleSheet(
        "QPushButton { background: rgba(100,200,100,200); color: white; "
        "font-size: 14px; padding: 10px 30px; border-radius: 5px; font-weight: bold; }"
        "QPushButton:hover { background: rgba(120,220,120,255); }"
    );
    connect(btnRestart, &QPushButton::clicked, this, &EndlessResultWidget::restartRequested);
    
    btnMenu = new QPushButton("返回菜单", this);
    btnMenu->setStyleSheet(
        "QPushButton { background: rgba(100,100,200,200); color: white; "
        "font-size: 14px; padding: 10px 30px; border-radius: 5px; font-weight: bold; }"
        "QPushButton:hover { background: rgba(120,120,220,255); }"
    );
    connect(btnMenu, &QPushButton::clicked, this, &EndlessResultWidget::menuRequested);
    
    btnLayout->addStretch();
    btnLayout->addWidget(btnRestart);
    btnLayout->addWidget(btnMenu);
    btnLayout->addStretch();
    
    mainLayout->addLayout(btnLayout);
}

void EndlessResultWidget::setResult(const EndlessGameResult& result) {
    currentResult = result;
    updateContent();
}

void EndlessResultWidget::showResult(const EndlessGameResult& result) {
    setResult(result);
    
    // 居中显示
    if (parentWidget()) {
        QPoint center = parentWidget()->rect().center();
        move(center.x() - width() / 2, center.y() - height() / 2 - 24);
    }
    
    show();
    setFocus();
}

void EndlessResultWidget::updateContent() {
    // ===== 更新统计数据标签 =====
    QString statsText = QString(
        "生存时间：%1 秒 | 总伤害：%2"
    ).arg(currentResult.survivalTime, 0, 'f', 1)
     .arg(currentResult.totalDamage, 0, 'f', 1);
    
    statsLabel->setText(statsText);

    spellLabel->setText(
        QString("法术选择：P1 %1 | P2 %2")
            .arg(currentResult.spellPlayer1.isEmpty() ? "无法术" : currentResult.spellPlayer1)
            .arg(currentResult.spellPlayer2.isEmpty() ? "无法术" : currentResult.spellPlayer2)
    );

    QString recordText;
    if (currentResult.isNewRecord) {
        recordText = QString("<b style='color: rgb(255,215,0);'>新纪录！</b><br>当前最佳：%1秒 / %2伤害")
                         .arg(currentResult.bestSurvivalTime, 0, 'f', 1)
                         .arg(currentResult.bestTotalDamage, 0, 'f', 1);
    } else if (currentResult.bestSurvivalTime > 0 || currentResult.bestTotalDamage > 0) {
        recordText = QString("最佳纪录：%1秒 / %2伤害")
                         .arg(currentResult.bestSurvivalTime, 0, 'f', 1)
                         .arg(currentResult.bestTotalDamage, 0, 'f', 1);
    } else {
        recordText = "暂无最佳纪录";
    }
    recordLabel->setText(recordText);

    damagePieLabel->setPixmap(buildDamagePiePixmap());

    float totalDmg = 0.f;
    for (int i = 0; i < 6; ++i) {
        totalDmg += currentResult.damageByWeapon[0][i];
    }
    
    // ===== 更新词条标签 =====
    QString modsText;
    modsText += "<b style='color: rgb(255,200,100)'>玩家1词条：</b> ";
    modsText += currentResult.modifiersPlayer1.isEmpty() 
              ? "<i>无词条</i>"
              : currentResult.modifiersPlayer1.join(" | ");
    modsText += "<br><br>";
    
    modsText += "<b style='color: rgb(255,200,100)'>玩家2（AI）词条：</b> ";
    modsText += currentResult.modifiersPlayer2.isEmpty()
              ? "<i>无词条</i>"
              : currentResult.modifiersPlayer2.join(" | ");
    
    modifiersLabel->setText(modsText);
}

QPixmap EndlessResultWidget::buildDamagePiePixmap() const {
    const int w = 320;
    const int h = 170;
    QPixmap pixmap(w, h);
    pixmap.fill(Qt::transparent);

    float totalDmg = 0.f;
    for (int i = 0; i < 6; ++i) {
        totalDmg += currentResult.damageByWeapon[0][i];
    }

    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing, true);

    if (totalDmg <= 0.001f) {
        p.setPen(QColor(180, 180, 200));
        p.drawText(pixmap.rect(), Qt::AlignCenter, "无伤害数据");
        return pixmap;
    }

    QRectF pieRect(10, 8, 120, 120);
    int startAngle = 90 * 16;
    for (int i = 0; i < 6; ++i) {
        const float dmg = currentResult.damageByWeapon[0][i];
        if (dmg <= 0.f) {
            continue;
        }
        const float ratio = dmg / totalDmg;
        const int span = static_cast<int>(ratio * 360.f * 16.f);
        p.setBrush(DAMAGE_COLORS[i]);
        p.setPen(Qt::NoPen);
        p.drawPie(pieRect, startAngle, -span);
        startAngle -= span;
    }

    // 中心挖空，做成环形饼图，视觉更清晰
    p.setBrush(QColor(25, 25, 45));
    p.drawEllipse(QRectF(41, 37, 62, 62));

    // 右侧简短图例
    p.setPen(QColor(210, 210, 230));
    p.setFont(QFont("Microsoft YaHei", 9));
    int y = 18;
    for (int i = 0; i < 6; ++i) {
        const float dmg = currentResult.damageByWeapon[0][i];
        const float pct = (dmg / totalDmg) * 100.f;
        p.setBrush(DAMAGE_COLORS[i]);
        p.setPen(Qt::NoPen);
        p.drawRect(148, y + 2, 10, 10);
        p.setPen(QColor(210, 210, 230));
        p.drawText(164, y + 12, QString("%1 %2%").arg(DAMAGE_NAMES[i]).arg(pct, 0, 'f', 1));
        y += 24;
    }
    return pixmap;
}

void EndlessResultWidget::paintEvent(QPaintEvent* event) {
    // 绘制半透明背景
    QPainter painter(this);
    painter.fillRect(rect(), BG_COLOR);
    
    // 绘制边框
    QPen borderPen(BORDER_COLOR, 2);
    painter.setPen(borderPen);
    painter.drawRect(0, 0, width() - 1, height() - 1);
    
    QWidget::paintEvent(event);
}

void EndlessResultWidget::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape || event->key() == Qt::Key_Return) {
        emit menuRequested();
    }
    QWidget::keyPressEvent(event);
}
