#include "databasemanager.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QFileInfo>
#include <QStandardPaths>
#include <cmath>  // 添加cmath头文件

DatabaseManager* DatabaseManager::m_instance = nullptr;

DatabaseManager::DatabaseManager(QObject *parent) : QObject(parent)
{
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(appDataPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QString dbPath = appDataPath + "/scores.db";
    m_database = QSqlDatabase::addDatabase("QSQLITE");
    m_database.setDatabaseName(dbPath);
}

DatabaseManager* DatabaseManager::instance()
{
    if (!m_instance) {
        m_instance = new DatabaseManager();
    }
    return m_instance;
}

bool DatabaseManager::initializeDatabase()
{
    if (!m_database.open()) {
        qDebug() << "Database error:" << m_database.lastError().text();
        return false;
    }

    QSqlQuery query;

    // 创建学生成绩表
    query.exec("CREATE TABLE IF NOT EXISTS scores ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "student_id TEXT NOT NULL,"
               "student_name TEXT NOT NULL,"
               "class_name TEXT NOT NULL,"
               "course TEXT NOT NULL,"
               "score REAL NOT NULL,"
               "exam_date DATE NOT NULL,"
               "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
               ")");

    // 创建索引以提高查询性能
    query.exec("CREATE INDEX IF NOT EXISTS idx_student_id ON scores(student_id)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_class_course ON scores(class_name, course)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_exam_date ON scores(exam_date)");

    return true;
}

bool DatabaseManager::addScore(const StudentScore &score)
{
    QSqlQuery query;
    query.prepare("INSERT INTO scores (student_id, student_name, class_name, course, score, exam_date) "
                  "VALUES (:student_id, :student_name, :class_name, :course, :score, :exam_date)");
    query.bindValue(":student_id", score.studentId);
    query.bindValue(":student_name", score.studentName);
    query.bindValue(":class_name", score.className);
    query.bindValue(":course", score.course);
    query.bindValue(":score", score.score);
    query.bindValue(":exam_date", score.examDate.toString("yyyy-MM-dd"));

    return query.exec();
}

bool DatabaseManager::updateScore(int id, const StudentScore &score)
{
    QSqlQuery query;
    query.prepare("UPDATE scores SET "
                  "student_id = :student_id, "
                  "student_name = :student_name, "
                  "class_name = :class_name, "
                  "course = :course, "
                  "score = :score, "
                  "exam_date = :exam_date "
                  "WHERE id = :id");
    query.bindValue(":student_id", score.studentId);
    query.bindValue(":student_name", score.studentName);
    query.bindValue(":class_name", score.className);
    query.bindValue(":course", score.course);
    query.bindValue(":score", score.score);
    query.bindValue(":exam_date", score.examDate.toString("yyyy-MM-dd"));
    query.bindValue(":id", id);

    return query.exec();
}

bool DatabaseManager::deleteScore(int id)
{
    QSqlQuery query;
    query.prepare("DELETE FROM scores WHERE id = :id");
    query.bindValue(":id", id);
    return query.exec();
}

QList<StudentScore> DatabaseManager::getAllScores()
{
    QList<StudentScore> scores;
    QSqlQuery query("SELECT id, student_id, student_name, class_name, course, score, exam_date FROM scores ORDER BY exam_date DESC");

    while (query.next()) {
        StudentScore score;
        score.id = query.value(0).toInt();
        score.studentId = query.value(1).toString();
        score.studentName = query.value(2).toString();
        score.className = query.value(3).toString();
        score.course = query.value(4).toString();
        score.score = query.value(5).toDouble();
        score.examDate = QDate::fromString(query.value(6).toString(), "yyyy-MM-dd");
        scores.append(score);
    }

    return scores;
}

QList<StudentScore> DatabaseManager::getScoresByFilter(const QString &className, const QString &course, const QString &keyword)
{
    QList<StudentScore> scores;
    QString sql = "SELECT id, student_id, student_name, class_name, course, score, exam_date FROM scores WHERE 1=1";

    if (!className.isEmpty() && className != "所有班级") {
        sql += " AND class_name = :class_name";
    }
    if (!course.isEmpty() && course != "所有课程") {
        sql += " AND course = :course";
    }
    if (!keyword.isEmpty()) {
        sql += " AND (student_id LIKE :keyword OR student_name LIKE :keyword)";
    }
    sql += " ORDER BY exam_date DESC";

    QSqlQuery query;
    query.prepare(sql);

    if (!className.isEmpty() && className != "所有班级") {
        query.bindValue(":class_name", className);
    }
    if (!course.isEmpty() && course != "所有课程") {
        query.bindValue(":course", course);
    }
    if (!keyword.isEmpty()) {
        query.bindValue(":keyword", "%" + keyword + "%");
    }

    if (!query.exec()) {
        qDebug() << "Query error:" << query.lastError().text();
        return scores;
    }

    while (query.next()) {
        StudentScore score;
        score.id = query.value(0).toInt();
        score.studentId = query.value(1).toString();
        score.studentName = query.value(2).toString();
        score.className = query.value(3).toString();
        score.course = query.value(4).toString();
        score.score = query.value(5).toDouble();
        score.examDate = QDate::fromString(query.value(6).toString(), "yyyy-MM-dd");
        scores.append(score);
    }

    return scores;
}

QMap<QString, QVariant> DatabaseManager::calculateStatistics(const QString &className, const QString &course)
{
    QMap<QString, QVariant> stats;
    QString sql = "SELECT "
                  "COUNT(*) as count, "
                  "AVG(score) as avg_score, "
                  "MAX(score) as max_score, "
                  "MIN(score) as min_score, "
                  "SUM(CASE WHEN score >= 60 THEN 1 ELSE 0 END) as pass_count "
                  "FROM scores WHERE 1=1";

    if (!className.isEmpty() && className != "所有班级") {
        sql += " AND class_name = :class_name";
    }
    if (!course.isEmpty() && course != "所有课程") {
        sql += " AND course = :course";
    }

    QSqlQuery query;
    query.prepare(sql);

    if (!className.isEmpty() && className != "所有班级") {
        query.bindValue(":class_name", className);
    }
    if (!course.isEmpty() && course != "所有课程") {
        query.bindValue(":course", course);
    }

    if (query.exec() && query.next()) {
        int count = query.value("count").toInt();
        double avg = query.value("avg_score").toDouble();
        double max = query.value("max_score").toDouble();
        double min = query.value("min_score").toDouble();
        int passCount = query.value("pass_count").toInt();

        stats["count"] = count;
        stats["avg"] = avg;
        stats["max"] = max;
        stats["min"] = min;
        stats["pass_rate"] = count > 0 ? (double(passCount) / count) * 100 : 0;

        // 计算标准差
        if (count > 0) {
            QString varianceSql = "SELECT SUM((score - :avg) * (score - :avg)) / COUNT(*) as variance FROM scores WHERE 1=1";
            if (!className.isEmpty() && className != "所有班级") {
                varianceSql += " AND class_name = :class_name";
            }
            if (!course.isEmpty() && course != "所有课程") {
                varianceSql += " AND course = :course";
            }

            QSqlQuery varianceQuery;
            varianceQuery.prepare(varianceSql);
            varianceQuery.bindValue(":avg", avg);
            if (!className.isEmpty() && className != "所有班级") {
                varianceQuery.bindValue(":class_name", className);
            }
            if (!course.isEmpty() && course != "所有课程") {
                varianceQuery.bindValue(":course", course);
            }

            if (varianceQuery.exec() && varianceQuery.next()) {
                double variance = varianceQuery.value("variance").toDouble();
                stats["std_dev"] = sqrt(variance);
            } else {
                stats["std_dev"] = 0.0;
            }
        } else {
            stats["std_dev"] = 0.0;
        }
    }

    return stats;
}

QStringList DatabaseManager::getAllClasses()
{
    QStringList classes;
    QSqlQuery query("SELECT DISTINCT class_name FROM scores ORDER BY class_name");
    classes << "所有班级";
    while (query.next()) {
        classes << query.value(0).toString();
    }
    return classes;
}

QStringList DatabaseManager::getAllCourses()
{
    QStringList courses;
    QSqlQuery query("SELECT DISTINCT course FROM scores ORDER BY course");
    courses << "所有课程";
    while (query.next()) {
        courses << query.value(0).toString();
    }
    return courses;
}

bool DatabaseManager::importFromCSV(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream in(&file);
    // 跳过标题行
    in.readLine();

    int successCount = 0;
    int errorCount = 0;

    while (!in.atEnd()) {
        QString line = in.readLine();
        QStringList fields = line.split(",");

        if (fields.size() >= 6) {
            StudentScore score;
            score.studentId = fields[0].trimmed();
            score.studentName = fields[1].trimmed();
            score.className = fields[2].trimmed();
            score.course = fields[3].trimmed();
            score.score = fields[4].trimmed().toDouble();
            score.examDate = QDate::fromString(fields[5].trimmed(), "yyyy-MM-dd");

            if (addScore(score)) {
                successCount++;
            } else {
                errorCount++;
            }
        }
    }

    file.close();
    return successCount > 0;
}

bool DatabaseManager::exportToCSV(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out << "学号,姓名,班级,课程,成绩,考试日期\n";

    QList<StudentScore> scores = getAllScores();
    foreach (const StudentScore& score, scores) {
        out << score.studentId << ","
            << score.studentName << ","
            << score.className << ","
            << score.course << ","
            << QString::number(score.score, 'f', 2) << ","
            << score.examDate.toString("yyyy-MM-dd") << "\n";
    }

    file.close();
    return true;
}
