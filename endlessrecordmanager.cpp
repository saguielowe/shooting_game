#include "endlessrecordmanager.h"
#include <QStandardPaths>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QDateTime>
#include <QDebug>

EndlessRecordManager::EndlessRecordManager()
{
    load();
}

QString EndlessRecordManager::getRecordFilePath() const {
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(appDataPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return appDataPath + "/endless_record.json";
}

void EndlessRecordManager::load() {
    QString filePath = getRecordFilePath();
    QFile file(filePath);
    
    if (!file.exists()) {
        qDebug() << "[EndlessRecord] 首次启动，无历史记录文件";
        m_record = EndlessRecord{};
        return;
    }
    
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "[EndlessRecord] 无法打开记录文件:" << filePath;
        m_record = EndlessRecord{};
        return;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isObject()) {
        fromJson(doc.object());
        qDebug() << "[EndlessRecord] 加载成功:" 
                 << "生存时间=" << m_record.maxSurvivalTime 
                 << "伤害=" << m_record.maxTotalDamage;
    } else {
        qWarning() << "[EndlessRecord] JSON格式错误";
        m_record = EndlessRecord{};
    }
}

void EndlessRecordManager::save() {
    if (!m_modified) {
        return;
    }
    
    QString filePath = getRecordFilePath();
    QFile file(filePath);
    
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "[EndlessRecord] 无法写入记录文件:" << filePath;
        return;
    }
    
    QJsonObject obj = toJson();
    QJsonDocument doc(obj);
    file.write(doc.toJson(QJsonDocument::Compact));
    file.close();
    
    m_modified = false;
    qDebug() << "[EndlessRecord] 已保存新纪录:" 
             << "生存时间=" << m_record.maxSurvivalTime 
             << "伤害=" << m_record.maxTotalDamage;
}

bool EndlessRecordManager::isNewRecord(float survivalTime, float totalDamage) const {
    // 至少满足一项即为新纪录：
    // 1. 生存时间更长
    // 2. 生存时间相同但伤害更高
    if (survivalTime > m_record.maxSurvivalTime) {
        return true;
    }
    if (survivalTime == m_record.maxSurvivalTime && totalDamage > m_record.maxTotalDamage) {
        return true;
    }
    return false;
}

void EndlessRecordManager::updateIfBetter(float survivalTime, float totalDamage) {
    bool updated = false;
    
    // 优先级1：更长生存时间
    if (survivalTime > m_record.maxSurvivalTime) {
        m_record.maxSurvivalTime = survivalTime;
        m_record.maxTotalDamage = totalDamage;  // 也更新伤害
        updated = true;
    }
    // 优先级2：生存时间相同但伤害更高
    else if (survivalTime == m_record.maxSurvivalTime && 
             totalDamage > m_record.maxTotalDamage) {
        m_record.maxTotalDamage = totalDamage;
        updated = true;
    }
    
    if (updated) {
        m_record.lastUpdateTime = QDateTime::currentDateTime().toString(Qt::ISODate);
        m_modified = true;
        save();
    }
}

QJsonObject EndlessRecordManager::toJson() const {
    QJsonObject obj;
    obj["maxSurvivalTime"] = m_record.maxSurvivalTime;
    obj["maxTotalDamage"] = m_record.maxTotalDamage;
    obj["lastUpdateTime"] = m_record.lastUpdateTime;
    return obj;
}

void EndlessRecordManager::fromJson(const QJsonObject& obj) {
    m_record.maxSurvivalTime = obj.value("maxSurvivalTime").toDouble(0.0f);
    m_record.maxTotalDamage = obj.value("maxTotalDamage").toDouble(0.0f);
    m_record.lastUpdateTime = obj.value("lastUpdateTime").toString("");
}
