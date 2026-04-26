#ifndef ENDLESSRECORDMANAGER_H
#define ENDLESSRECORDMANAGER_H

#include <QString>
#include <QJsonObject>

// ============================================================
//  EndlessRecordManager — 无尽模式成绩持久化
//  保存最高生存时间和伤害记录
// ============================================================

struct EndlessRecord {
    float maxSurvivalTime = 0.f;
    float maxTotalDamage = 0.f;
    QString lastUpdateTime;  // ISO 8601 格式
};

class EndlessRecordManager {
public:
    EndlessRecordManager();
    
    // 从磁盘加载记录
    void load();
    
    // 检查新成绩是否超越最佳纪录
    bool isNewRecord(float survivalTime, float totalDamage) const;
    
    // 更新记录（如果新成绩更好）
    void updateIfBetter(float survivalTime, float totalDamage);
    
    // 获取当前最佳纪录
    const EndlessRecord& getRecord() const { return m_record; }
    
    // 保存到磁盘
    void save();
    
    // 清空记录
    void reset() { m_record = EndlessRecord{}; }

private:
    QString getRecordFilePath() const;
    QJsonObject toJson() const;
    void fromJson(const QJsonObject& obj);
    
    EndlessRecord m_record;
    bool m_modified = false;  // 标志是否有未保存的改动
};

#endif // ENDLESSRECORDMANAGER_H
