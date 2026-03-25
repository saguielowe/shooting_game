#include "setupwidget.h"
#include <QStandardItemModel>

SetupWidget::SetupWidget(QWidget* parent)
    : QWidget(parent)
{
    buildUI();
}

void SetupWidget::buildUI() {
    auto* root = new QVBoxLayout(this);
    root->setSpacing(24);
    root->setContentsMargins(60, 40, 60, 40);

    // ---- 标题 --------------------------------------------
    auto* title = new QLabel("游戏设置", this);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size: 28px; font-weight: bold; color: white;");
    root->addWidget(title);

    // ---- 模式选择 ----------------------------------------
    auto* modeBox = new QGroupBox("游戏模式", this);
    modeBox->setStyleSheet("QGroupBox { color: white; font-size: 14px; }");
    auto* modeLayout = new QHBoxLayout(modeBox);

    modeGroup = new QButtonGroup(this);
    btnAI      = new QPushButton("人机对战", this);
    btnPVP     = new QPushButton("双人对战", this);
    btnEndless = new QPushButton("无尽模式", this);

    for (auto* btn : {btnAI, btnPVP, btnEndless}) {
        btn->setCheckable(true);
        btn->setFixedHeight(40);
        btn->setStyleSheet(
            "QPushButton { background: #444; color: white; border-radius: 6px; font-size: 13px; }"
            "QPushButton:checked { background: #2980b9; }"
            "QPushButton:hover { background: #555; }"
        );
        modeLayout->addWidget(btn);
    }

    modeGroup->addButton(btnAI,      0);
    modeGroup->addButton(btnPVP,     1);
    modeGroup->addButton(btnEndless, 2);
    btnAI->setChecked(true);

    connect(modeGroup, &QButtonGroup::idClicked,
            this, &SetupWidget::onModeChanged);

    root->addWidget(modeBox);

    // ---- 局数选择 ----------------------------------------
    auto* boBox = new QGroupBox("局数", this);
    boBox->setStyleSheet("QGroupBox { color: white; font-size: 14px; }");
    auto* boLayout = new QHBoxLayout(boBox);

    bestOfGroup = new QButtonGroup(this);
    btnBO1 = new QPushButton("BO1", this);
    btnBO3 = new QPushButton("BO3", this);
    btnBO5 = new QPushButton("BO5", this);
    btnBO7 = new QPushButton("BO7", this);

    for (auto* btn : {btnBO1, btnBO3, btnBO5, btnBO7}) {
        btn->setCheckable(true);
        btn->setFixedHeight(40);
        btn->setStyleSheet(
            "QPushButton { background: #444; color: white; border-radius: 6px; font-size: 13px; }"
            "QPushButton:checked { background: #27ae60; }"
            "QPushButton:hover { background: #555; }"
        );
        boLayout->addWidget(btn);
    }

    bestOfGroup->addButton(btnBO1, 1);
    bestOfGroup->addButton(btnBO3, 3);
    bestOfGroup->addButton(btnBO5, 5);
    bestOfGroup->addButton(btnBO7, 7);
    btnBO3->setChecked(true);

    // 自定义局数
    boLayout->addWidget(new QLabel("自定义:", this));
    spinCustom = new QSpinBox(this);
    spinCustom->setRange(1, 99);
    spinCustom->setValue(3);
    spinCustom->setFixedHeight(40);
    spinCustom->setStyleSheet("background: #444; color: white; border-radius: 6px;");
    // 点自定义 spinbox 时取消预设按钮选中
    connect(spinCustom, &QSpinBox::valueChanged, this, [this]() {
        if (bestOfGroup->checkedButton())
            bestOfGroup->checkedButton()->setChecked(false);
    });
    boLayout->addWidget(spinCustom);

    root->addWidget(boBox);

    // ---- 法术选择 ----------------------------------------
    auto* spellBox = new QGroupBox("法术选择", this);
    spellBox->setStyleSheet("QGroupBox { color: white; font-size: 14px; }");
    auto* spellLayout = new QHBoxLayout(spellBox);

    auto makeSpellCombo = [this]() {
        auto* combo = new QComboBox(this);
        combo->setFixedHeight(36);
        combo->setStyleSheet(
            "QComboBox { background: #444; color: white; border-radius: 6px; padding: 4px; }"
            "QComboBox QAbstractItemView { background: #333; color: white; }"
        );
        combo->addItem("无法术",  static_cast<int>(GameSession::Spell::NONE));
        combo->addItem("定身",    static_cast<int>(GameSession::Spell::FREEZE));
        combo->addItem("隐身",    static_cast<int>(GameSession::Spell::STEALTH));
        combo->addItem("安身法",  static_cast<int>(GameSession::Spell::BARRIER));
        combo->addItem("铜头铁臂",static_cast<int>(GameSession::Spell::IRON_BODY));
        combo->addItem("身外身法",static_cast<int>(GameSession::Spell::CLONE));
        combo->addItem("禁字法",  static_cast<int>(GameSession::Spell::FORBIDDEN));
        return combo;
    };

    spellLayout->addWidget(new QLabel("玩家1:", this));
    comboSpellP1 = makeSpellCombo();
    spellLayout->addWidget(comboSpellP1);

    spellLayout->addSpacing(20);

    labelSpellP2 = new QLabel("玩家2:", this);
    spellLayout->addWidget(labelSpellP2);
    comboSpellP2 = makeSpellCombo();
    spellLayout->addWidget(comboSpellP2);

    // 人机模式下隐藏 P2 法术选择
    labelSpellP2->setVisible(false);
    comboSpellP2->setVisible(false);

    root->addWidget(spellBox);

    // ---- 操作按钮 ----------------------------------------
    auto* btnLayout = new QHBoxLayout();
    btnBack = new QPushButton("返回", this);
    btnStart = new QPushButton("开始游戏", this);

    btnBack->setFixedSize(120, 44);
    btnStart->setFixedSize(180, 44);

    btnBack->setStyleSheet(
        "QPushButton { background: #555; color: white; border-radius: 8px; font-size: 14px; }"
        "QPushButton:hover { background: #666; }"
    );
    btnStart->setStyleSheet(
        "QPushButton { background: #e74c3c; color: white; border-radius: 8px; font-size: 14px; font-weight: bold; }"
        "QPushButton:hover { background: #c0392b; }"
    );

    connect(btnBack,  &QPushButton::clicked, this, &SetupWidget::backToMenu);
    connect(btnStart, &QPushButton::clicked, this, &SetupWidget::onStartClicked);

    btnLayout->addWidget(btnBack);
    btnLayout->addStretch();
    btnLayout->addWidget(btnStart);
    root->addLayout(btnLayout);

    root->addStretch();
}

void SetupWidget::onModeChanged(int id) {
    currentMode = static_cast<GameSession::Mode>(id);
    updateSpellOptions();

    // 双人/无尽模式显示 P2 法术
    bool showP2 = (currentMode == GameSession::Mode::PVP);
    labelSpellP2->setVisible(showP2);
    comboSpellP2->setVisible(showP2);
}

void SetupWidget::updateSpellOptions() {
    // 人机模式：隐身可用；双人/无尽模式：隐身不可用，强制改为无
    auto filterStealth = [this](QComboBox* combo) {
        int stealthIdx = combo->findData(static_cast<int>(GameSession::Spell::STEALTH));
        if (stealthIdx < 0) return;

        bool pvpOrEndless = (currentMode != GameSession::Mode::AI);
        // 用 setItemData 的 Qt::UserRole+1 标记是否禁用
        auto* model = qobject_cast<QStandardItemModel*>(combo->model());
        if (!model) return;
        auto* item = model->item(stealthIdx);
        if (!item) return;

        if (pvpOrEndless) {
            item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
            item->setText("隐身（仅人机）");
            // 如果当前选中的是隐身，重置为无
            if (combo->currentData().toInt() == static_cast<int>(GameSession::Spell::STEALTH))
                combo->setCurrentIndex(0);
        } else {
            item->setFlags(item->flags() | Qt::ItemIsEnabled);
            item->setText("隐身");
        }
    };

    filterStealth(comboSpellP1);
    filterStealth(comboSpellP2);
}

void SetupWidget::onStartClicked() {
    emit sessionReady(buildSession());
}

GameSession SetupWidget::buildSession() const {
    GameSession s;

    s.mode = currentMode;

    // 局数：优先用预设按钮，没有选中则用 spinbox
    if (auto* checked = bestOfGroup->checkedButton()) {
        s.bestOf = bestOfGroup->id(checked);
    } else {
        s.bestOf = spinCustom->value();
    }

    s.spellP1 = static_cast<GameSession::Spell>(comboSpellP1->currentData().toInt());
    s.spellP2 = (currentMode == GameSession::Mode::PVP)
                ? static_cast<GameSession::Spell>(comboSpellP2->currentData().toInt())
                : GameSession::Spell::NONE;

    return s;
}
