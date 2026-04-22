# AI 统一决策收尾文档（2026-04-22）

## 1. 范围

本文档用于交接当前 AI 重构后的能力与后续工作重点，核心文件为：
- gameai.h
- gameai.cpp

## 2. 已完成能力

### 2.1 五目标统一状态机

AI 顶层目标统一为：
- WAIT
- SURVIVE
- GROW
- CHASE
- ATTACK

不再依赖旧的 seek 分叉状态，状态选择集中在 determineNextState()。

### 2.2 单帧统一意图流程

当前每帧决策顺序：
1. 选战略状态
2. 计算施法意图
3. 计算切武器意图
4. 执行移动/攻击

冲突规则：
- 默认切武器优先
- 高风险下，防御法术（STEALTH / IRON_BODY）可抢占切武器

### 2.3 法术锁

setCurrentSpell(...) 已实现“本局法术锁”：
- 首次传入非 NONE 法术后即锁定
- 本局后续调用不再改变 AI 法术

### 2.4 统一风险模型

calculateRisk(...) 已作为统一风险入口，被以下模块共用：
- determineNextState()
- shouldCastStealth()
- shouldCastIronBody()
- 切槽与施法冲突判定

风险项已覆盖：
- 敌方武器威胁
- 敌方护甲
- 敌方伤害词条压力
- 敌方施法状态（隐身/铜头）
- 自身被冻结状态
- 自身低血压力

### 2.5 施法决策扩展

updateSpellIntent() 已支持三法术：
- FREEZE -> shouldCastFreeze()
- STEALTH -> shouldCastStealth()
- IRON_BODY -> shouldCastIronBody()

STEALTH：
- 无破影一击时偏保命/转线
- 有破影一击（stealthFirstHit）且未触发时，允许主动隐身找首击窗口

IRON_BODY：
- 近距对拼、高风险场景优先
- 与 ironBodyThorns / ironBodyHardened 词条联动

### 2.6 卡住处理

isStuck 触发后，已从“随机救急动作”改为“换目标/换状态”：
- 清空预定动作
- 重置状态承诺窗口
- 切换战略目标
- 立即执行一帧新目标动作

目标是降低对手连续跳跃导致的无头抖动。

## 3. 静态检查结论

当前 Problems 主要为 Qt includePath/环境索引问题（QObject/QTimer 等），
未发现本轮逻辑改动引入的明确语法错误提示。

说明：
- 本次收尾未进行构建与运行验证
- 后续主工作应为实机测试与权重调优

## 4. 测试清单（建议）

### 4.1 稳定性

- 对手中距离持续跳跃扰动
- 观察状态切换是否明显降低抖动

### 4.2 法术锁

- 开局设置 AI 法术
- 对局中重复调用 setCurrentSpell
- 验证法术不变

### 4.3 隐身 + 破影一击

- Case A: stealthFirstHit=false，验证隐身偏保命
- Case B: stealthFirstHit=true 且未触发，验证隐身更主动找首击窗口

### 4.4 铜头铁臂

- 近战压力下验证触发倾向
- 有/无 thorns、hardened 词条时行为是否分化

### 4.5 施法与切槽冲突

- 制造“切槽与施法都想做”的场景
- 验证默认切槽优先，高风险防御法术可抢占

### 4.6 对手施法响应

- 对手开隐身、开铜头
- 验证 risk 与状态评分有明显偏移

## 5. 调权重优先级

建议按以下顺序调整，减少耦合噪声：
1. 风险模型系数
2. 五目标状态分数系数
3. 三法术阈值与加分项
4. 性格乘子

## 6. 快速调权重方法（新增）

现在已经提供“单入口调参”，不需要在 gameai.cpp 里到处改常量。

### 6.1 集中配置入口

在 gameai.h 的 GameAI::AITuning 里统一定义所有关键系数：
- risk 系数
- 五目标状态分数系数
- 法术评分系数（STEALTH/IRON_BODY）
- 性格乘子
- 危机阈值与状态承诺帧数

测试时有两种方式：
1. 手动改 AITuning 默认值（适合长期基线）
2. 对局启动时 setTuning(...) 注入（适合快速实验）

### 6.2 一键预设（快速 A/B）

新增预设接口：
- applyTuningPreset(DEFAULT)
- applyTuningPreset(AGGRESSIVE)
- applyTuningPreset(DEFENSIVE)
- applyTuningPreset(GROWTH_FOCUSED)

建议在初始化 AI 时快速切预设做横向对比。

示例：

```cpp
// 默认
ai->applyTuningPreset(GameAI::TuningPreset::DEFAULT);

// 激进
ai->applyTuningPreset(GameAI::TuningPreset::AGGRESSIVE);

// 保守
ai->applyTuningPreset(GameAI::TuningPreset::DEFENSIVE);

// 成长导向
ai->applyTuningPreset(GameAI::TuningPreset::GROWTH_FOCUSED);
```

### 6.3 精细实验建议

如果需要精细调参，建议直接复制一份 tuning：

```cpp
auto t = ai->getTuning();
t.surviveRiskScale += 0.1f;
t.attackRiskPenalty += 0.1f;
t.stealthFirstHitWindowBonus += 0.1f;
ai->setTuning(t);
```

这样可以做到“只改几项，其他保持不变”，非常适合回归测试。

## 7. 完成度评估

从架构和功能点看，当前已“功能闭环”：
- 统一状态机
- 法术锁
- 风险统一入口
- FREEZE/STEALTH/IRON_BODY 决策
- 词条联动（含破影一击）
- 卡住重定向

从产品完成度看，仍差最后一步：
- 需要实机测试覆盖与权重回归
- 主要工作是参数调优，而不是再补大功能
