#include "databasemanager.h"
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QMessageBox>
#include <QApplication>
#include <QDir>
#include <QSet>  // 添加缺少的头文件

DatabaseManager* DatabaseManager::m_instance = nullptr;

DatabaseManager::DatabaseManager(QObject *parent) : QObject(parent)
{
    // 设置数据库文件路径
    QString dbPath = "C:/Users/bill/Desktop/student_scores.db";

    // 确保桌面目录存在
    QDir desktopDir("C:/Users/bill/Desktop");
    if (!desktopDir.exists()) {
        qDebug() << "桌面目录不存在，尝试创建...";
        if (!desktopDir.mkpath(".")) {
            qDebug() << "无法创建桌面目录";
            return;
        }
    }

    qDebug() << "数据库路径:" << dbPath;

    // 连接数据库
    m_database = QSqlDatabase::addDatabase("QSQLITE", "StudentScoresConnection");
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
        qDebug() << "数据库错误:" << m_database.lastError().text();
        return false;
    }

    qDebug() << "数据库成功打开";

    // 创建表
    if (!createTables()) {
        qDebug() << "创建表失败";
        return false;
    }

    // 检查数据库是否已有数据
    QSqlQuery query(m_database);
    query.prepare("SELECT COUNT(*) FROM scores");
    if (query.exec() && query.next()) {
        int count = query.value(0).toInt();
        qDebug() << "数据库中有" << count << "条记录";
    }

    return true;
}

bool DatabaseManager::createTables()
{
    QSqlQuery query(m_database);

    // 创建学生成绩表
    bool success = query.exec(
        "CREATE TABLE IF NOT EXISTS scores ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "student_id TEXT NOT NULL,"
        "student_name TEXT NOT NULL,"
        "class_name TEXT NOT NULL,"
        "course TEXT NOT NULL,"
        "score REAL NOT NULL,"
        "exam_date DATE NOT NULL,"
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
        ")"
        );

    if (!success) {
        qDebug() << "创建表错误:" << query.lastError().text();
        return false;
    }

    // 创建索引以提高查询性能
    query.exec("CREATE INDEX IF NOT EXISTS idx_student_id ON scores(student_id)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_class_course ON scores(class_name, course)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_exam_date ON scores(exam_date)");

    return true;
}

bool DatabaseManager::addScore(const StudentScore &score)
{
    QSqlQuery query(m_database);
    query.prepare(
        "INSERT INTO scores (student_id, student_name, class_name, course, score, exam_date) "
        "VALUES (:student_id, :student_name, :class_name, :course, :score, :exam_date)"
        );
    query.bindValue(":student_id", score.studentId);
    query.bindValue(":student_name", score.studentName);
    query.bindValue(":class_name", score.className);
    query.bindValue(":course", score.course);
    query.bindValue(":score", score.score);
    query.bindValue(":exam_date", score.examDate.toString("yyyy-MM-dd"));

    bool success = query.exec();
    if (!success) {
        qDebug() << "添加成绩错误:" << query.lastError().text();
    } else {
        qDebug() << "成功添加成绩:" << score.studentName << score.course << score.score;
    }

    return success;
}

bool DatabaseManager::updateScore(int id, const StudentScore &score)
{
    QSqlQuery query(m_database);
    query.prepare(
        "UPDATE scores SET "
        "student_id = :student_id, "
        "student_name = :student_name, "
        "class_name = :class_name, "
        "course = :course, "
        "score = :score, "
        "exam_date = :exam_date "
        "WHERE id = :id"
        );
    query.bindValue(":student_id", score.studentId);
    query.bindValue(":student_name", score.studentName);
    query.bindValue(":class_name", score.className);
    query.bindValue(":course", score.course);
    query.bindValue(":score", score.score);
    query.bindValue(":exam_date", score.examDate.toString("yyyy-MM-dd"));
    query.bindValue(":id", id);

    bool success = query.exec();
    if (!success) {
        qDebug() << "更新成绩错误:" << query.lastError().text();
    }

    return success;
}

bool DatabaseManager::deleteScore(int id)
{
    QSqlQuery query(m_database);
    query.prepare("DELETE FROM scores WHERE id = :id");
    query.bindValue(":id", id);

    bool success = query.exec();
    if (!success) {
        qDebug() << "删除成绩错误:" << query.lastError().text();
    }

    return success;
}

QList<StudentScore> DatabaseManager::getAllScores()
{
    QList<StudentScore> scores;
    QSqlQuery query(m_database);
    query.prepare("SELECT id, student_id, student_name, class_name, course, score, exam_date FROM scores ORDER BY exam_date DESC");

    if (!query.exec()) {
        qDebug() << "获取所有成绩错误:" << query.lastError().text();
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

    qDebug() << "获取到" << scores.size() << "条成绩记录";
    return scores;
}

// 修改：简化筛选功能，只保留日期参数（具体日期点）
QList<StudentScore> DatabaseManager::getScoresByFilter(const QString &className, const QString &course,
                                                       const QString &examDate, const QString &keyword)
{
    QList<StudentScore> scores;
    QString sql = "SELECT id, student_id, student_name, class_name, course, score, exam_date FROM scores WHERE 1=1";

    if (!className.isEmpty() && className != "所有班级") {
        sql += " AND class_name = :class_name";
    }
    if (!course.isEmpty() && course != "所有课程") {
        sql += " AND course = :course";
    }

    // 修改：添加具体日期筛选
    if (!examDate.isEmpty() && examDate != "所有日期") {
        sql += " AND exam_date = :exam_date";
    }

    if (!keyword.isEmpty()) {
        sql += " AND (student_id LIKE :keyword OR student_name LIKE :keyword)";
    }
    sql += " ORDER BY exam_date DESC";

    QSqlQuery query(m_database);
    query.prepare(sql);

    if (!className.isEmpty() && className != "所有班级") {
        query.bindValue(":class_name", className);
    }
    if (!course.isEmpty() && course != "所有课程") {
        query.bindValue(":course", course);
    }

    // 绑定日期参数
    if (!examDate.isEmpty() && examDate != "所有日期") {
        query.bindValue(":exam_date", examDate);
    }

    if (!keyword.isEmpty()) {
        query.bindValue(":keyword", "%" + keyword + "%");
    }

    if (!query.exec()) {
        qDebug() << "查询错误:" << query.lastError().text();
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

    qDebug() << "筛选查询结果:" << scores.size() << "条记录";
    return scores;
}

// 修改：简化统计功能，只保留日期参数（具体日期点）
QMap<QString, QVariant> DatabaseManager::calculateStatistics(const QString &className, const QString &course,
                                                             const QString &examDate)
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

    // 修改：添加具体日期筛选
    if (!examDate.isEmpty() && examDate != "所有日期") {
        sql += " AND exam_date = :exam_date";
    }

    QSqlQuery query(m_database);
    query.prepare(sql);

    if (!className.isEmpty() && className != "所有班级") {
        query.bindValue(":class_name", className);
    }
    if (!course.isEmpty() && course != "所有课程") {
        query.bindValue(":course", course);
    }

    // 绑定日期参数
    if (!examDate.isEmpty() && examDate != "所有日期") {
        query.bindValue(":exam_date", examDate);
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

            // 修改：添加具体日期筛选
            if (!examDate.isEmpty() && examDate != "所有日期") {
                varianceSql += " AND exam_date = :exam_date";
            }

            QSqlQuery varianceQuery(m_database);
            varianceQuery.prepare(varianceSql);
            varianceQuery.bindValue(":avg", avg);
            if (!className.isEmpty() && className != "所有班级") {
                varianceQuery.bindValue(":class_name", className);
            }
            if (!course.isEmpty() && course != "所有课程") {
                varianceQuery.bindValue(":course", course);
            }

            // 绑定日期参数
            if (!examDate.isEmpty() && examDate != "所有日期") {
                varianceQuery.bindValue(":exam_date", examDate);
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

// 修改：简化成绩分布功能，只保留日期参数（具体日期点）
QList<QMap<QString, QVariant>> DatabaseManager::getScoreDistribution(const QString &className, const QString &course,
                                                                     const QString &examDate, int bins)
{
    QList<QMap<QString, QVariant>> distribution;

    if (bins <= 0) bins = 5;

    // 定义分数段
    QVector<QPair<double, double>> scoreRanges;
    QVector<QString> rangeLabels;

    if (bins == 5) {
        // 使用5个固定区间：0-59, 60-69, 70-79, 80-89, 90-100
        scoreRanges = {
            {0, 59.99}, {60, 69.99}, {70, 79.99}, {80, 89.99}, {90, 100}
        };
        rangeLabels = {"0-59", "60-69", "70-79", "80-89", "90-100"};
    } else {
        // 使用动态区间
        double rangeSize = 100.0 / bins;
        for (int i = 0; i < bins; i++) {
            double lower = i * rangeSize;
            double upper = (i + 1) * rangeSize;
            if (i == bins - 1) upper = 100.0; // 最后一个区间包含100
            scoreRanges.append({lower, upper});
            rangeLabels.append(QString("%1-%2").arg(lower, 0, 'f', 1).arg(upper, 0, 'f', 1));
        }
    }

    // 构建SQL查询
    QString sql = "SELECT score FROM scores WHERE 1=1";

    if (!className.isEmpty() && className != "所有班级") {
        sql += " AND class_name = :class_name";
    }
    if (!course.isEmpty() && course != "所有课程") {
        sql += " AND course = :course";
    }

    // 修改：添加具体日期筛选
    if (!examDate.isEmpty() && examDate != "所有日期") {
        sql += " AND exam_date = :exam_date";
    }

    QSqlQuery query(m_database);
    query.prepare(sql);

    if (!className.isEmpty() && className != "所有班级") {
        query.bindValue(":class_name", className);
    }
    if (!course.isEmpty() && course != "所有课程") {
        query.bindValue(":course", course);
    }

    // 绑定日期参数
    if (!examDate.isEmpty() && examDate != "所有日期") {
        query.bindValue(":exam_date", examDate);
    }

    // 初始化统计数组
    QVector<int> counts(scoreRanges.size(), 0);
    int total = 0;

    if (query.exec()) {
        while (query.next()) {
            double score = query.value(0).toDouble();
            total++;

            // 查找分数所在的区间
            for (int i = 0; i < scoreRanges.size(); i++) {
                if (score >= scoreRanges[i].first && score <= scoreRanges[i].second) {
                    counts[i]++;
                    break;
                }
            }
        }

        // 构建返回结果
        for (int i = 0; i < scoreRanges.size(); i++) {
            QMap<QString, QVariant> binData;
            binData["range"] = rangeLabels[i];
            binData["count"] = counts[i];
            binData["percentage"] = total > 0 ? (counts[i] * 100.0 / total) : 0.0;
            binData["lower"] = scoreRanges[i].first;
            binData["upper"] = scoreRanges[i].second;
            distribution.append(binData);
        }
    } else {
        qDebug() << "获取成绩分布错误:" << query.lastError().text();
    }

    qDebug() << "成绩分布统计完成，共" << total << "条记录，" << bins << "个区间";
    return distribution;
}

// 修改：简化趋势数据功能，只保留日期参数（具体日期点）
QList<QMap<QString, QVariant>> DatabaseManager::getCourseTrendData(const QString &className, const QString &course,
                                                                   const QString &examDate)
{
    QList<QMap<QString, QVariant>> trendData;

    // 修改：如果指定了具体日期，则只返回该日期的数据
    QString sql;
    if (!examDate.isEmpty() && examDate != "所有日期") {
        // 只查询指定日期的数据
        sql = "SELECT exam_date, AVG(score) as avg_score, COUNT(*) as count "
              "FROM scores WHERE 1=1";

        if (!className.isEmpty() && className != "所有班级") {
            sql += " AND class_name = :class_name";
        }
        if (!course.isEmpty() && course != "所有课程") {
            sql += " AND course = :course";
        }
        sql += " AND exam_date = :exam_date "
               "GROUP BY exam_date ORDER BY exam_date ASC";
    } else {
        // 查询所有日期的趋势数据
        sql = "SELECT exam_date, AVG(score) as avg_score, COUNT(*) as count "
              "FROM scores WHERE 1=1";

        if (!className.isEmpty() && className != "所有班级") {
            sql += " AND class_name = :class_name";
        }
        if (!course.isEmpty() && course != "所有课程") {
            sql += " AND course = :course";
        }
        sql += " GROUP BY exam_date ORDER BY exam_date ASC";
    }

    QSqlQuery query(m_database);
    query.prepare(sql);

    if (!className.isEmpty() && className != "所有班级") {
        query.bindValue(":class_name", className);
    }
    if (!course.isEmpty() && course != "所有课程") {
        query.bindValue(":course", course);
    }

    // 绑定日期参数
    if (!examDate.isEmpty() && examDate != "所有日期") {
        query.bindValue(":exam_date", examDate);
    }

    if (query.exec()) {
        QSet<QString> uniqueDates;
        while (query.next()) {
            QDate examDate = QDate::fromString(query.value(0).toString(), "yyyy-MM-dd");
            double avgScore = query.value(1).toDouble();
            int count = query.value(2).toInt();

            QString dateStr = examDate.toString("yyyy-MM-dd");

            if (!uniqueDates.contains(dateStr) && avgScore > 0) {
                uniqueDates.insert(dateStr);

                QMap<QString, QVariant> dataPoint;
                dataPoint["date"] = dateStr;
                dataPoint["score"] = avgScore;
                dataPoint["date_obj"] = examDate;
                dataPoint["count"] = count;

                trendData.append(dataPoint);
            }
        }
        qDebug() << "获取课程趋势数据成功，共" << trendData.size() << "条记录";
    } else {
        qDebug() << "获取课程趋势数据错误:" << query.lastError().text();
    }

    return trendData;
}

// 修改：简化课程对比功能，只保留日期参数（具体日期点）
QList<QMap<QString, QVariant>> DatabaseManager::getCourseComparison(const QString &className,
                                                                    const QString &examDate)
{
    QList<QMap<QString, QVariant>> comparisonData;

    QString sql = "SELECT course, AVG(score) as avg_score, COUNT(*) as count FROM scores WHERE 1=1";

    if (!className.isEmpty() && className != "所有班级") {
        sql += " AND class_name = :class_name";
    }

    // 修改：添加具体日期筛选
    if (!examDate.isEmpty() && examDate != "所有日期") {
        sql += " AND exam_date = :exam_date";
    }

    sql += " GROUP BY course ORDER BY avg_score DESC";

    QSqlQuery query(m_database);
    query.prepare(sql);

    if (!className.isEmpty() && className != "所有班级") {
        query.bindValue(":class_name", className);
    }

    // 绑定日期参数
    if (!examDate.isEmpty() && examDate != "所有日期") {
        query.bindValue(":exam_date", examDate);
    }

    if (query.exec()) {
        while (query.next()) {
            QMap<QString, QVariant> courseData;
            QString course = query.value(0).toString();
            double avgScore = query.value(1).toDouble();
            int count = query.value(2).toInt();

            courseData["course"] = course;
            courseData["avg_score"] = avgScore;
            courseData["count"] = count;

            comparisonData.append(courseData);
        }
    } else {
        qDebug() << "获取课程对比数据错误:" << query.lastError().text();
    }

    return comparisonData;
}

QStringList DatabaseManager::getAllClasses()
{
    QStringList classes;
    QSqlQuery query(m_database);
    query.prepare("SELECT DISTINCT class_name FROM scores ORDER BY class_name");

    if (query.exec()) {
        classes << "所有班级";
        while (query.next()) {
            classes << query.value(0).toString();
        }
    }

    return classes;
}

QStringList DatabaseManager::getAllCourses()
{
    QStringList courses;
    QSqlQuery query(m_database);
    query.prepare("SELECT DISTINCT course FROM scores ORDER BY course");

    if (query.exec()) {
        courses << "所有课程";
        while (query.next()) {
            courses << query.value(0).toString();
        }
    }

    return courses;
}

// 修改：新增获取所有考试日期函数
QStringList DatabaseManager::getAllExamDates()
{
    QStringList examDates;
    QSqlQuery query(m_database);
    query.prepare("SELECT DISTINCT exam_date FROM scores ORDER BY exam_date DESC");

    if (query.exec()) {
        examDates << "所有日期";
        while (query.next()) {
            QString dateStr = query.value(0).toString();
            examDates << dateStr;
        }
    }

    qDebug() << "获取到" << examDates.size() << "个考试日期";
    return examDates;
}

QStringList DatabaseManager::getAllStudents()
{
    QStringList students;
    QSqlQuery query(m_database);
    query.prepare("SELECT DISTINCT student_id, student_name FROM scores ORDER BY student_id");

    if (query.exec()) {
        while (query.next()) {
            QString studentId = query.value(0).toString();
            QString studentName = query.value(1).toString();
            students << QString("%1 - %2").arg(studentId).arg(studentName);
        }
    }

    return students;
}

// 导入导出函数（注意：这些函数在databasemanager.h中已声明）
bool DatabaseManager::importFromCSV(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "无法打开文件:" << filePath;
        return false;
    }

    QTextStream in(&file);
    // 跳过标题行
    QString headerLine = in.readLine();
    qDebug() << "CSV标题行:" << headerLine;

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
        } else {
            qDebug() << "CSV行格式错误:" << line;
            errorCount++;
        }
    }

    file.close();
    qDebug() << "CSV导入结果: 成功 =" << successCount << ", 失败 =" << errorCount;
    return successCount > 0;
}

bool DatabaseManager::importFromExcel(const QString &filePath)
{
    // Excel导入需要额外库支持，这里只显示提示
    qDebug() << "Excel导入功能需要额外安装库支持，请使用CSV格式导入";

// 在GUI应用程序中显示消息框
#ifndef QT_NO_MESSAGEBOX
    QMessageBox::information(nullptr, "提示", "Excel导入功能需要额外安装库支持，请使用CSV格式导入");
#endif

    return false;
}

bool DatabaseManager::exportToCSV(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "无法创建文件:" << filePath;
        return false;
    }

    QTextStream out(&file);
    out << "学号,姓名,班级,课程,成绩,考试日期\n";

    QList<StudentScore> scores = getAllScores();
    for (const StudentScore& score : scores) {
        out << score.studentId << ","
            << score.studentName << ","
            << score.className << ","
            << score.course << ","
            << QString::number(score.score, 'f', 2) << ","
            << score.examDate.toString("yyyy-MM-dd") << "\n";
    }

    file.close();
    qDebug() << "导出完成，共" << scores.size() << "条记录";
    return true;
}

bool DatabaseManager::isDatabaseConnected() const
{
    return m_database.isOpen();
}

QString DatabaseManager::getDatabasePath() const
{
    return m_database.databaseName();
}
