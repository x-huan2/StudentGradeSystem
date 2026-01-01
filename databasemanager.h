#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QList>
#include <QMap>
#include <QDate>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>
#include <cmath>

struct StudentScore {
    int id;
    QString studentId;
    QString studentName;
    QString className;
    QString course;
    double score;
    QDate examDate;
};

class DatabaseManager : public QObject
{
    Q_OBJECT
public:
    static DatabaseManager* instance();

    bool initializeDatabase();
    bool addScore(const StudentScore& score);
    bool updateScore(int id, const StudentScore& score);
    bool deleteScore(int id);
    QList<StudentScore> getAllScores();
    QList<StudentScore> getScoresByFilter(const QString& className, const QString& course, const QString& keyword = "");

    // 统计功能
    QMap<QString, QVariant> calculateStatistics(const QString& className, const QString& course);
    QList<QMap<QString, QVariant>> getScoreDistribution(const QString& className, const QString& course, int bins = 10);
    QList<QMap<QString, QVariant>> getTrendData(const QString& studentId, const QString& course);
    QList<QMap<QString, QVariant>> getCourseComparison(const QString& className);

    // 获取唯一值列表
    QStringList getAllClasses();
    QStringList getAllCourses();
    QStringList getAllStudents();

    // 批量导入
    bool importFromCSV(const QString& filePath);
    bool importFromExcel(const QString& filePath);
    bool exportToCSV(const QString& filePath);

    // 测试数据库连接
    bool isDatabaseConnected() const;
    QString getDatabasePath() const;

private:
    explicit DatabaseManager(QObject *parent = nullptr);
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    static DatabaseManager* m_instance;
    QSqlDatabase m_database;
    bool createTables();
    bool insertSampleData();
};

#endif // DATABASEMANAGER_H
