#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "databasemanager.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QStandardItemModel>
#include <QChartView>
#include <QBarSeries>
#include <QBarSet>
#include <QLineSeries>
#include <QBarCategoryAxis>
#include <QValueAxis>
#include <QPieSeries>
#include <QPrinter>
#include <QPainter>
#include <QTextDocument>
#include <QDateTimeAxis>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_scoreModel(new ScoreModel(this))
{
    ui->setupUi(this);

    // 先设置UI
    setupUI();

    // 然后初始化数据库
    setupDatabase();

    // 最后设置图表
    setupCharts();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupUI()
{
    // 设置表格模型
    ui->tableView->setModel(m_scoreModel);
    ui->tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);

    // 设置列宽
    ui->tableView->setColumnWidth(0, 50);   // ID
    ui->tableView->setColumnWidth(1, 100);  // 学号
    ui->tableView->setColumnWidth(2, 80);   // 姓名
    ui->tableView->setColumnWidth(3, 100);  // 班级
    ui->tableView->setColumnWidth(4, 80);   // 课程
    ui->tableView->setColumnWidth(5, 80);   // 成绩
    ui->tableView->setColumnWidth(6, 120);  // 考试日期

    // 隐藏ID列
    ui->tableView->hideColumn(0);

    // 设置默认日期为今天
    ui->dateExam->setDate(QDate::currentDate());

    // 连接信号槽
    connect(ui->tableView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &MainWindow::loadSelectedScoreToForm);

    // 初始刷新
    refreshFilterCombos();

    // 显示初始记录数
    ui->labelRecordCount->setText(QString("总记录数: %1").arg(m_scoreModel->rowCount()));
}

void MainWindow::setupDatabase()
{
    if (DatabaseManager::instance()->initializeDatabase()) {
        QString dbPath = DatabaseManager::instance()->getDatabasePath();
        updateStatusBar(QString("数据库连接成功 - 路径: %1").arg(dbPath));

        // 刷新数据模型
        m_scoreModel->refreshData();
        ui->labelRecordCount->setText(QString("总记录数: %1").arg(m_scoreModel->rowCount()));

        qDebug() << "Database initialized and data loaded successfully";
    } else {
        updateStatusBar("数据库连接失败");
        QMessageBox::critical(this, "错误", "无法连接数据库，请检查数据库配置");
    }
}

void MainWindow::setupCharts()
{
    // 初始化图表视图
    ui->chartViewHistogram->setRenderHint(QPainter::Antialiasing);
    ui->chartViewTrend->setRenderHint(QPainter::Antialiasing);
    ui->chartViewComparison->setRenderHint(QPainter::Antialiasing);

    // 设置默认图表
    showDefaultCharts();
}

void MainWindow::showDefaultCharts()
{
    // 显示默认的柱状图
    QChart *histogramChart = new QChart();
    histogramChart->setTitle("成绩分布 (示例)");
    QBarSeries *series = new QBarSeries();
    QBarSet *set = new QBarSet("示例数据");
    *set << 5 << 10 << 15 << 8 << 12;
    series->append(set);
    histogramChart->addSeries(series);

    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append({"0-59", "60-69", "70-79", "80-89", "90-100"});
    histogramChart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText("人数");
    histogramChart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    ui->chartViewHistogram->setChart(histogramChart);
}

void MainWindow::on_btnAdd_clicked()
{
    if (ui->editStudentId->text().isEmpty() || ui->editStudentName->text().isEmpty()) {
        QMessageBox::warning(this, "警告", "请填写学号和姓名");
        return;
    }

    StudentScore score;
    score.studentId = ui->editStudentId->text();
    score.studentName = ui->editStudentName->text();
    score.className = ui->comboClass->currentText();
    score.course = ui->comboCourse->currentText();
    score.score = ui->spinScore->value();
    score.examDate = ui->dateExam->date();

    if (DatabaseManager::instance()->addScore(score)) {
        // 刷新数据
        m_scoreModel->refreshData();
        clearForm();
        refreshFilterCombos();
        updateStatusBar("添加成绩成功");
        ui->labelRecordCount->setText(QString("总记录数: %1").arg(m_scoreModel->rowCount()));
    } else {
        QMessageBox::warning(this, "错误", "添加成绩失败");
    }
}

void MainWindow::on_btnUpdate_clicked()
{
    QModelIndexList selected = ui->tableView->selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, "警告", "请选择要更新的记录");
        return;
    }

    int row = selected.first().row();
    StudentScore oldScore = m_scoreModel->getScoreAt(row);

    if (oldScore.id == -1) {
        QMessageBox::warning(this, "错误", "无效的记录");
        return;
    }

    StudentScore newScore;
    newScore.studentId = ui->editStudentId->text();
    newScore.studentName = ui->editStudentName->text();
    newScore.className = ui->comboClass->currentText();
    newScore.course = ui->comboCourse->currentText();
    newScore.score = ui->spinScore->value();
    newScore.examDate = ui->dateExam->date();

    if (DatabaseManager::instance()->updateScore(oldScore.id, newScore)) {
        m_scoreModel->refreshData();
        clearForm();
        updateStatusBar("更新成绩成功");
        ui->labelRecordCount->setText(QString("总记录数: %1").arg(m_scoreModel->rowCount()));
    } else {
        QMessageBox::warning(this, "错误", "更新成绩失败");
    }
}

void MainWindow::on_btnDelete_clicked()
{
    QModelIndexList selected = ui->tableView->selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, "警告", "请选择要删除的记录");
        return;
    }

    int row = selected.first().row();
    StudentScore score = m_scoreModel->getScoreAt(row);

    if (score.id == -1) {
        QMessageBox::warning(this, "错误", "无效的记录");
        return;
    }

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "确认删除",
                                  QString("确定要删除 %1 的 %2 成绩吗？").arg(score.studentName).arg(score.course),
                                  QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        if (DatabaseManager::instance()->deleteScore(score.id)) {
            m_scoreModel->refreshData();
            clearForm();
            refreshFilterCombos();
            updateStatusBar("删除成绩成功");
            ui->labelRecordCount->setText(QString("总记录数: %1").arg(m_scoreModel->rowCount()));
        } else {
            QMessageBox::warning(this, "错误", "删除成绩失败");
        }
    }
}

void MainWindow::on_btnRefresh_clicked()
{
    m_scoreModel->refreshData();
    refreshFilterCombos();
    updateStatusBar("数据已刷新");
    ui->labelRecordCount->setText(QString("总记录数: %1").arg(m_scoreModel->rowCount()));
}

void MainWindow::on_btnImportCSV_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, "选择CSV文件", "", "CSV文件 (*.csv)");
    if (filePath.isEmpty()) return;

    if (DatabaseManager::instance()->importFromCSV(filePath)) {
        m_scoreModel->refreshData();
        refreshFilterCombos();
        updateStatusBar("CSV导入成功");
        ui->labelRecordCount->setText(QString("总记录数: %1").arg(m_scoreModel->rowCount()));
    } else {
        QMessageBox::warning(this, "错误", "CSV导入失败");
    }
}

void MainWindow::on_btnExport_clicked()
{
    QString filePath = QFileDialog::getSaveFileName(this, "导出报表", "", "CSV文件 (*.csv)");
    if (filePath.isEmpty()) return;

    if (!filePath.endsWith(".csv", Qt::CaseInsensitive)) {
        filePath += ".csv";
    }

    if (DatabaseManager::instance()->exportToCSV(filePath)) {
        updateStatusBar(QString("报表已导出到: %1").arg(filePath));
    } else {
        QMessageBox::warning(this, "错误", "报表导出失败");
    }
}

void MainWindow::on_btnCalculateStats_clicked()
{
    QString className = ui->comboStatsClass->currentText();
    QString course = ui->comboStatsCourse->currentText();

    QMap<QString, QVariant> stats = DatabaseManager::instance()->calculateStatistics(
        className == "所有班级" ? "" : className,
        course == "所有课程" ? "" : course
        );

    // 更新统计结果标签
    ui->labelAvgValue->setText(QString::number(stats["avg"].toDouble(), 'f', 2));
    ui->labelMaxValue->setText(QString::number(stats["max"].toDouble(), 'f', 2));
    ui->labelMinValue->setText(QString::number(stats["min"].toDouble(), 'f', 2));
    ui->labelStdDevValue->setText(QString::number(stats["std_dev"].toDouble(), 'f', 2));
    ui->labelPassRateValue->setText(QString::number(stats["pass_rate"].toDouble(), 'f', 2) + "%");
    ui->labelCountValue->setText(QString::number(stats["count"].toInt()));

    // 更新图表
    showHistogramChart(className, course);
    showTrendChart(className, course);
    showComparisonChart(className);

    updateStatusBar("统计计算完成");
}

void MainWindow::showHistogramChart(const QString &className, const QString &course)
{
    // 创建柱状图
    QChart *chart = new QChart();
    chart->setTitle(QString("成绩分布 - %1 %2").arg(className).arg(course));

    QBarSeries *series = new QBarSeries();

    // 模拟数据
    QStringList categories;
    categories << "0-59" << "60-69" << "70-79" << "80-89" << "90-100";

    QBarSet *set = new QBarSet("人数分布");

    // 根据统计结果生成模拟数据
    QMap<QString, QVariant> stats = DatabaseManager::instance()->calculateStatistics(
        className == "所有班级" ? "" : className,
        course == "所有课程" ? "" : course
        );

    int count = stats["count"].toInt();
    if (count > 0) {
        double avg = stats["avg"].toDouble();
        // 基于平均分生成模拟分布
        if (avg >= 85) {
            *set << 2 << 3 << 5 << 8 << 12;  // 优秀分布
        } else if (avg >= 75) {
            *set << 3 << 5 << 8 << 6 << 3;   // 良好分布
        } else if (avg >= 60) {
            *set << 5 << 8 << 6 << 3 << 1;   // 中等分布
        } else {
            *set << 10 << 5 << 3 << 1 << 0;  // 较差分布
        }
    } else {
        *set << 0 << 0 << 0 << 0 << 0;
    }

    series->append(set);
    chart->addSeries(series);

    // 设置X轴
    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(categories);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    // 设置Y轴
    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText("人数");
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);

    ui->chartViewHistogram->setChart(chart);
}

void MainWindow::showTrendChart(const QString &className, const QString &course)
{
    QChart *chart = new QChart();
    chart->setTitle(QString("成绩趋势 - %1 %2").arg(className).arg(course));

    QLineSeries *series = new QLineSeries();
    series->setName("平均分趋势");

    // 模拟趋势数据
    series->append(1, 75);
    series->append(2, 78);
    series->append(3, 82);
    series->append(4, 80);
    series->append(5, 85);

    chart->addSeries(series);

    QValueAxis *axisX = new QValueAxis();
    axisX->setTitleText("考试次数");
    axisX->setLabelFormat("%d");
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText("平均成绩");
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    ui->chartViewTrend->setChart(chart);
}

void MainWindow::showComparisonChart(const QString &className)
{
    QChart *chart = new QChart();
    chart->setTitle(QString("课程对比 - %1").arg(className));

    QPieSeries *series = new QPieSeries();

    // 获取所有课程
    QStringList courses = DatabaseManager::instance()->getAllCourses();
    courses.removeAll("所有课程");

    // 为每个课程计算平均分
    for (const QString &course : courses) {
        QMap<QString, QVariant> stats = DatabaseManager::instance()->calculateStatistics(
            className == "所有班级" ? "" : className,
            course
            );

        double avg = stats["avg"].toDouble();
        if (avg > 0) {
            series->append(course, avg);
        }
    }

    // 设置扇区标签
    for (QPieSlice *slice : series->slices()) {
        slice->setLabelVisible();
        slice->setLabel(QString("%1\n%2分").arg(slice->label()).arg(slice->value()));
    }

    chart->addSeries(series);
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignRight);

    ui->chartViewComparison->setChart(chart);
}

void MainWindow::on_tableView_doubleClicked(const QModelIndex &index)
{
    loadSelectedScoreToForm();
}

void MainWindow::loadSelectedScoreToForm()
{
    QModelIndexList selected = ui->tableView->selectionModel()->selectedRows();
    if (selected.isEmpty()) return;

    int row = selected.first().row();
    StudentScore score = m_scoreModel->getScoreAt(row);

    if (score.id == -1) return;

    ui->editStudentId->setText(score.studentId);
    ui->editStudentName->setText(score.studentName);
    ui->comboClass->setCurrentText(score.className);
    ui->comboCourse->setCurrentText(score.course);
    ui->spinScore->setValue(score.score);
    ui->dateExam->setDate(score.examDate);
}

void MainWindow::clearForm()
{
    ui->editStudentId->clear();
    ui->editStudentName->clear();
    ui->spinScore->setValue(0);
    ui->dateExam->setDate(QDate::currentDate());
    ui->tableView->clearSelection();
}

void MainWindow::refreshFilterCombos()
{
    QStringList classes = DatabaseManager::instance()->getAllClasses();
    QStringList courses = DatabaseManager::instance()->getAllCourses();

    ui->comboFilterClass->clear();
    ui->comboFilterClass->addItems(classes);

    ui->comboFilterCourse->clear();
    ui->comboFilterCourse->addItems(courses);

    ui->comboStatsClass->clear();
    ui->comboStatsClass->addItems(classes);

    ui->comboStatsCourse->clear();
    ui->comboStatsCourse->addItems(courses);
}

void MainWindow::on_editSearch_textChanged(const QString &text)
{
    QString className = ui->comboFilterClass->currentText();
    QString course = ui->comboFilterCourse->currentText();

    m_scoreModel->filterData(
        className == "所有班级" ? "" : className,
        course == "所有课程" ? "" : course,
        text
        );
    ui->labelRecordCount->setText(QString("筛选记录数: %1").arg(m_scoreModel->rowCount()));
}

void MainWindow::on_comboFilterClass_currentTextChanged(const QString &text)
{
    Q_UNUSED(text);
    on_editSearch_textChanged(ui->editSearch->text());
}

void MainWindow::on_comboFilterCourse_currentTextChanged(const QString &text)
{
    Q_UNUSED(text);
    on_editSearch_textChanged(ui->editSearch->text());
}

void MainWindow::on_comboStatsClass_currentTextChanged(const QString &text)
{
    Q_UNUSED(text);
    // 可以在这里添加动态更新统计图表的逻辑
}

void MainWindow::on_comboStatsCourse_currentTextChanged(const QString &text)
{
    Q_UNUSED(text);
    // 可以在这里添加动态更新统计图表的逻辑
}

void MainWindow::updateStatusBar(const QString &message)
{
    ui->statusbar->showMessage(message, 5000);
}

void MainWindow::on_btnGenerateReport_clicked()
{
    generateReport();
}

void MainWindow::generateReport()
{
    QString className = ui->comboStatsClass->currentText();
    QString course = ui->comboStatsCourse->currentText();

    QMap<QString, QVariant> stats = DatabaseManager::instance()->calculateStatistics(
        className == "所有班级" ? "" : className,
        course == "所有课程" ? "" : course
        );

    QString report = QString(
                         "========== 成绩分析报告 ==========\n\n"
                         "班级: %1\n"
                         "课程: %2\n\n"
                         "========== 统计结果 ==========\n"
                         "平均分: %3\n"
                         "最高分: %4\n"
                         "最低分: %5\n"
                         "标准差: %6\n"
                         "及格率: %7\n"
                         "学生人数: %8\n\n"
                         "生成时间: %9\n"
                         "================================="
                         ).arg(className == "所有班级" ? "全部班级" : className)
                         .arg(course == "所有课程" ? "全部课程" : course)
                         .arg(QString::number(stats["avg"].toDouble(), 'f', 2))
                         .arg(QString::number(stats["max"].toDouble(), 'f', 2))
                         .arg(QString::number(stats["min"].toDouble(), 'f', 2))
                         .arg(QString::number(stats["std_dev"].toDouble(), 'f', 2))
                         .arg(QString::number(stats["pass_rate"].toDouble(), 'f', 2))
                         .arg(QString::number(stats["count"].toInt()))
                         .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));

    QMessageBox::information(this, "成绩分析报告", report);
}
