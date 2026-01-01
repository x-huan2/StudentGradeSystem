#include "databasemanager.h"
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QMessageBox>
#include <QApplication>
#include <QDir>

DatabaseManager* DatabaseManager::m_instance = nullptr;

DatabaseManager::DatabaseManager(QObject *parent) : QObject(parent)
{
    // 设置数据库文件路径
    QString dbPath = "C:/Users/bill/Desktop/student_scores/student_scores.db";

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

    QSqlQuery query(m_database);
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

    QSqlQuery query(m_database);
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

            QSqlQuery varianceQuery(m_database);
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
    // 这里需要安装额外的库来处理Excel文件
    // 暂时先返回false，或调用CSV导入
    QMessageBox::information(nullptr, "提示", "Excel导入功能需要额外安装库支持，请使用CSV格式导入");
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

// 这些函数暂时返回空列表，以后可以扩展
QList<QMap<QString, QVariant>> DatabaseManager::getScoreDistribution(const QString& className, const QString& course, int bins)
{
    Q_UNUSED(className);
    Q_UNUSED(course);
    Q_UNUSED(bins);
    return QList<QMap<QString, QVariant>>();
}

QList<QMap<QString, QVariant>> DatabaseManager::getTrendData(const QString& studentId, const QString& course)
{
    Q_UNUSED(studentId);
    Q_UNUSED(course);
    return QList<QMap<QString, QVariant>>();
}

QList<QMap<QString, QVariant>> DatabaseManager::getCourseComparison(const QString& className)
{
    Q_UNUSED(className);
    return QList<QMap<QString, QVariant>>();
}
