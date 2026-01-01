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

    // 修改：简化筛选功能，只保留日期参数（具体日期点）
    QList<StudentScore> getScoresByFilter(const QString& className, const QString& course,
                                          const QString& examDate = "",
                                          const QString& keyword = "");

    // 统计功能 - 简化，只保留日期参数（具体日期点）
    QMap<QString, QVariant> calculateStatistics(const QString& className, const QString& course,
                                                const QString& examDate = "");
    QList<QMap<QString, QVariant>> getScoreDistribution(const QString& className, const QString& course,
                                                        const QString& examDate = "",
                                                        int bins = 5);
    QList<QMap<QString, QVariant>> getCourseTrendData(const QString& className, const QString& course,
                                                      const QString& examDate = "");
    QList<QMap<QString, QVariant>> getCourseComparison(const QString& className,
                                                       const QString& examDate = "");

    // 获取唯一值列表
    QStringList getAllClasses();
    QStringList getAllCourses();
    QStringList getAllStudents();

    // 修改：获取考试日期列表
    QStringList getAllExamDates();

    // 新增：导入导出功能
    bool importFromCSV(const QString& filePath);
    bool importFromExcel(const QString& filePath);
    bool exportToCSV(const QString& filePath);

    // 新增：数据库状态检查
    bool isDatabaseConnected() const;
    QString getDatabasePath() const;

private:
    explicit DatabaseManager(QObject *parent = nullptr);
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    static DatabaseManager* m_instance;
    QSqlDatabase m_database;
    bool createTables();
};

#endif // DATABASEMANAGER_H
