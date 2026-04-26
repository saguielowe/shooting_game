#include "modifieroverlay.h"
#include <QKeyEvent>
#include <QPainter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

// ── 样式常量 ────────────────────────────────────────────────
static const int   OVERLAY_W     = 700;
static const int   OVERLAY_H     = 280;
static const int   CARD_W        = 200;
static const int   CARD_H        = 200;
static const QColor BG_COLOR     = QColor(10, 10, 20, 210);   // 半透明深色
static const QColor CARD_NORMAL  = QColor(40, 40, 70, 230);
static const QColor CARD_BORDER  = QColor(120, 120, 200);
static const QColor TEXT_TITLE   = QColor(220, 220, 255);
static const QColor TEXT_DESC    = QColor(170, 170, 200);
static const QColor TEXT_HINT    = QColor(100, 200, 100);
static const QColor CD_COLOR     = QColor(255, 200, 80);

ModifierOverlay::ModifierOverlay(QWidget* parent)
    : QWidget(parent)
{
    // 固定尺寸，居中由 showOptions() 定位
    setFixedSize(OVERLAY_W, OVERLAY_H);
    setAttribute(Qt::WA_TranslucentBackground);
    setFocusPolicy(Qt::StrongFocus);
    hide();

    // 倒计时 timer，每秒 tick
    countdownTimer = new QTimer(this);
    countdownTimer->setInterval(1000);
    connect(countdownTimer, &QTimer::timeout, this, &ModifierOverlay::onTimerTick);

    buildLayout();
}

void ModifierOverlay::buildLayout() {
    // ── 整体垂直布局 ────────────────────────────────────────
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(20, 14, 20, 14);
    root->setSpacing(8);

    // 标题行
    auto* topRow = new QHBoxLayout();
    titleLabel = new QLabel(this);
    titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    titleLabel->setStyleSheet("color: rgba(220,220,255,255); font-size: 16px; font-weight: bold;");

    countdownLabel = new QLabel(this);
    countdownLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    countdownLabel->setStyleSheet("color: rgba(255,200,80,255); font-size: 15px; font-weight: bold;");

    topRow->addWidget(titleLabel);
    topRow->addStretch();
    topRow->addWidget(countdownLabel);
    root->addLayout(topRow);

    // 三张卡片横排
    auto* cardRow = new QHBoxLayout();
    cardRow->setSpacing(16);

    const QString hintTexts[3] = { "按 1 选择", "按 2 选择", "按 3 选择" };

    for (int i = 0; i < 3; ++i) {
        auto* frame = new QWidget(this);
        frame->setFixedSize(CARD_W, CARD_H);
        frame->setStyleSheet(QString(
            "QWidget { background: rgba(40,40,70,230); "
            "border: 2px solid rgba(120,120,200,200); "
            "border-radius: 10px; }"));

        auto* vl = new QVBoxLayout(frame);
        vl->setContentsMargins(12, 12, 12, 12);
        vl->setSpacing(6);

        auto* nameL = new QLabel(frame);
        nameL->setWordWrap(true);
        nameL->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
        nameL->setStyleSheet("color: rgba(220,220,255,255); font-size: 15px; "
                             "font-weight: bold; background: transparent; border: none;");

        auto* descL = new QLabel(frame);
        descL->setWordWrap(true);
        descL->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        descL->setStyleSheet("color: rgba(170,170,200,255); font-size: 12px; "
                             "background: transparent; border: none;");

        auto* hintL = new QLabel(hintTexts[i], frame);
        hintL->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
        hintL->setStyleSheet("color: rgba(100,200,100,255); font-size: 12px; "
                             "background: transparent; border: none;");

        vl->addWidget(nameL);
        vl->addWidget(descL, 1);
        vl->addWidget(hintL);

        cards[i] = { frame, nameL, descL, hintL };
        cardRow->addWidget(frame);
    }

    root->addLayout(cardRow);
}

void ModifierOverlay::showOptions(int pIndex, const QVector<ModifierData>& options) {
    Q_ASSERT(options.size() == 3);
    currentOptions = options;
    playerIndex    = pIndex;

    // 更新标题
    titleLabel->setText(QString("玩家 %1 — 选择词条").arg(pIndex + 1));

    // 更新卡片内容
    for (int i = 0; i < 3; ++i) {
        cards[i].nameLabel->setText(options[i].displayName);
        cards[i].descLabel->setText(options[i].description);
    }

    // 重置并启动倒计时
    remainSeconds = 10;
    updateCountdownLabel();
    countdownTimer->start();

    // 居中显示在父控件内
    if (parentWidget()) {
        QPoint center = parentWidget()->rect().center();
        move(center.x() - width() / 2, center.y() - height() / 2);
    }

    show();
    setFocus();
}

void ModifierOverlay::onTimerTick() {
    remainSeconds--;
    updateCountdownLabel();
    if (remainSeconds <= 0) {
        // 时间到，随机选一个
        int rand = QRandomGenerator::global()->bounded(3);
        selectOption(rand);
    }
}

void ModifierOverlay::selectOption(int index) {
    countdownTimer->stop();
    hide();
    emit optionChosen(currentOptions[index]);
}

void ModifierOverlay::updateCountdownLabel() {
    countdownLabel->setText(QString("剩余 %1 秒").arg(remainSeconds));
}

void ModifierOverlay::keyPressEvent(QKeyEvent* event) {
    switch (event->key()) {
    case Qt::Key_1: selectOption(0); break;
    case Qt::Key_2: selectOption(1); break;
    case Qt::Key_3: selectOption(2); break;
    default: QWidget::keyPressEvent(event); break;
    }
}

void ModifierOverlay::paintEvent(QPaintEvent*) {
    // 圆角半透明背景（卡片已有自己的背景，这里只画外框底色）
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(BG_COLOR);
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(rect(), 14, 14);
}
