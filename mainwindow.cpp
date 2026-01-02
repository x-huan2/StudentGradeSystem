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
#include <QDateTime>
#include <QStandardItem>
#include <QSplitter>
#include <QBrush>
#include <QColor>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_scoreModel(new ScoreModel(this))
    , m_rankingModel(new QStandardItemModel(this))  // 初始化排名模型
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

    // 设置排名表格模型
    ui->tableViewRanking->setModel(m_rankingModel);
    ui->tableViewRanking->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableViewRanking->setSelectionBehavior(QAbstractItemView::SelectRows);
    setupRankingModel();

    // 设置排名表格列宽
    ui->tableViewRanking->setColumnWidth(0, 60);   // 排名
    ui->tableViewRanking->setColumnWidth(1, 100);  // 学号
    ui->tableViewRanking->setColumnWidth(2, 80);   // 姓名
    ui->tableViewRanking->setColumnWidth(3, 100);  // 班级
    ui->tableViewRanking->setColumnWidth(4, 80);   // 总分
    ui->tableViewRanking->setColumnWidth(5, 80);   // 平均分
    ui->tableViewRanking->setColumnWidth(6, 80);   // 科目数

    // 初始隐藏排名分析相关控件
    ui->labelRankingType->setVisible(false);
    ui->comboRankingType->setVisible(false);
    ui->labelRankingCourse->setVisible(false);
    ui->comboRankingCourse->setVisible(false);
    ui->btnShowRanking->setVisible(false);

    // 连接信号槽
    connect(ui->tableView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &MainWindow::loadSelectedScoreToForm);

    // 连接标签页切换信号
    connect(ui->tabWidgetCharts, &QTabWidget::currentChanged,
            this, &MainWindow::on_tabWidgetCharts_currentChanged);

    // 初始刷新
    refreshFilterCombos();

    // 显示初始记录数
    ui->labelRecordCount->setText(QString("总记录数: %1").arg(m_scoreModel->rowCount()));
}

void MainWindow::setupDatabase()
{
    // 直接连接到指定数据库文件
    QString dbPath = "C:/Users/bill/Desktop/student_scores.db";

    // 检查数据库文件是否存在
    QFileInfo dbFile(dbPath);
    if (!dbFile.exists()) {
        qDebug() << "数据库文件不存在，将创建新数据库";
        QMessageBox::information(this, "提示",
                                 QString("数据库文件不存在，将创建新数据库文件:\n%1\n\n程序将自动创建数据表。")
                                     .arg(dbPath));
    }

    // 初始化数据库
    if (DatabaseManager::instance()->initializeDatabase()) {
        QString actualDbPath = DatabaseManager::instance()->getDatabasePath();
        updateStatusBar(QString("数据库连接成功 - 路径: %1").arg(actualDbPath));

        // 显示数据库信息
        qDebug() << "数据库连接成功，路径:" << actualDbPath;

        // 刷新数据模型
        m_scoreModel->refreshData();
        ui->labelRecordCount->setText(QString("总记录数: %1").arg(m_scoreModel->rowCount()));

        qDebug() << "数据库初始化完成，加载了" << m_scoreModel->rowCount() << "条记录";

        // 如果数据库为空，显示提示
        if (m_scoreModel->rowCount() == 0) {
            QMessageBox::information(this, "提示",
                                     "数据库中没有学生成绩记录。\n\n请通过以下方式添加数据：\n"
                                     "1. 使用上方的表单手动添加记录\n"
                                     "2. 使用'导入CSV'功能批量导入数据\n"
                                     "3. 使用文件菜单中的导入功能");
        }
    } else {
        updateStatusBar("数据库连接失败");
        QString errorMsg = QString("无法连接数据库，请检查：\n"
                                   "1. 数据库文件路径: %1\n"
                                   "2. 是否有写入权限\n"
                                   "3. 数据库是否被其他程序占用")
                               .arg(dbPath);
        QMessageBox::critical(this, "错误", errorMsg);
    }
}

void MainWindow::setupCharts()
{
    // 初始化图表视图 - 只初始化前三个图表视图
    ui->chartViewHistogram->setRenderHint(QPainter::Antialiasing);
    ui->chartViewTrend->setRenderHint(QPainter::Antialiasing);
    ui->chartViewComparison->setRenderHint(QPainter::Antialiasing);

    // 注意：这里移除了对 chartViewRanking 和 splitter 的引用，因为它们已经在UI中被删除

    // 设置默认图表
    showDefaultCharts();
}

void MainWindow::showDefaultCharts()
{
    // 检查是否有数据
    if (m_scoreModel->rowCount() == 0) {
        // 显示空图表提示
        QChart *emptyChart = new QChart();
        emptyChart->setTitle("暂无数据，请先添加成绩记录");
        ui->chartViewHistogram->setChart(emptyChart);

        QChart *emptyTrendChart = new QChart();
        emptyTrendChart->setTitle("暂无数据，请先添加成绩记录");
        ui->chartViewTrend->setChart(emptyTrendChart);

        QChart *emptyComparisonChart = new QChart();
        emptyComparisonChart->setTitle("暂无数据，请先添加成绩记录");
        ui->chartViewComparison->setChart(emptyComparisonChart);
        return;
    }

    // 显示默认的柱状图
    QChart *histogramChart = new QChart();
    histogramChart->setTitle("成绩分布");
    QBarSeries *series = new QBarSeries();
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

// 标签页切换槽函数
void MainWindow::on_tabWidgetCharts_currentChanged(int index)
{
    // 当切换到排名分析标签页（索引3）时，显示排名分析相关控件
    bool showRankingControls = (index == 3);

    // 显示/隐藏排名分析控件
    ui->labelRankingType->setVisible(showRankingControls);
    ui->comboRankingType->setVisible(showRankingControls);

    // 只有当选择单科排名时才显示课程下拉框
    bool showCourseCombo = showRankingControls &&
                           (ui->comboRankingType->currentText() == "单科成绩排名");
    ui->labelRankingCourse->setVisible(showCourseCombo);
    ui->comboRankingCourse->setVisible(showCourseCombo);

    ui->btnShowRanking->setVisible(showRankingControls);

    // 如果是排名分析标签页，自动显示排名
    if (index == 3) {
        // 自动显示排名
        on_btnShowRanking_clicked();
    }
}

// 设置排名模型
void MainWindow::setupRankingModel()
{
    m_rankingModel->clear();
    QStringList headers;
    headers << "排名" << "学号" << "姓名" << "班级" << "总分" << "平均分" << "科目数";
    m_rankingModel->setHorizontalHeaderLabels(headers);
}

// 更新排名表格
void MainWindow::updateRankingTable(const QList<StudentRank>& rankingList)
{
    m_rankingModel->setRowCount(rankingList.size());

    for (int i = 0; i < rankingList.size(); i++) {
        const StudentRank& rankInfo = rankingList[i];

        // 排名
        QStandardItem* rankItem = new QStandardItem(QString::number(rankInfo.rank));
        rankItem->setTextAlignment(Qt::AlignCenter);
        m_rankingModel->setItem(i, 0, rankItem);

        // 学号
        QStandardItem* idItem = new QStandardItem(rankInfo.studentId);
        m_rankingModel->setItem(i, 1, idItem);

        // 姓名
        QStandardItem* nameItem = new QStandardItem(rankInfo.studentName);
        m_rankingModel->setItem(i, 2, nameItem);

        // 班级
        QStandardItem* classItem = new QStandardItem(rankInfo.className);
        m_rankingModel->setItem(i, 3, classItem);

        // 总分
        QStandardItem* totalItem = new QStandardItem(QString::number(rankInfo.score, 'f', 2));
        totalItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_rankingModel->setItem(i, 4, totalItem);

        // 平均分
        QStandardItem* avgItem = new QStandardItem(QString::number(rankInfo.avgScore, 'f', 2));
        avgItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_rankingModel->setItem(i, 5, avgItem);

        // 科目数
        QStandardItem* countItem = new QStandardItem(QString::number(rankInfo.courseCount));
        countItem->setTextAlignment(Qt::AlignCenter);
        m_rankingModel->setItem(i, 6, countItem);

        // 根据排名设置背景色
        QBrush backgroundBrush;
        if (rankInfo.rank == 1) {
            backgroundBrush = QBrush(QColor(255, 255, 200)); // 第一名：金色
        } else if (rankInfo.rank == 2) {
            backgroundBrush = QBrush(QColor(230, 230, 230)); // 第二名：银色
        } else if (rankInfo.rank == 3) {
            backgroundBrush = QBrush(QColor(255, 200, 150)); // 第三名：铜色
        } else if (rankInfo.rank <= 10) {
            backgroundBrush = QBrush(QColor(220, 240, 255)); // 前10名：浅蓝色
        }

        if (backgroundBrush.style() != Qt::NoBrush) {
            for (int col = 0; col < m_rankingModel->columnCount(); col++) {
                QStandardItem* item = m_rankingModel->item(i, col);
                if (item) {
                    item->setBackground(backgroundBrush);
                }
            }
        }
    }

    // 更新状态栏
    if (!rankingList.isEmpty()) {
        updateStatusBar(QString("已显示排名，共 %1 名学生").arg(rankingList.size()));
    }
}

// 显示排名表格（删除图表，只显示表格）
void MainWindow::showRankingTable(const QString &className, const QString &course,
                                  const QString &examDate)
{
    // 检查是否有数据
    if (m_scoreModel->rowCount() == 0) {
        // 清空排名表格
        m_rankingModel->setRowCount(0);
        // 在表格中显示提示信息
        m_rankingModel->setRowCount(1);
        QStandardItem* tipItem = new QStandardItem("暂无数据，请先添加成绩记录");
        tipItem->setTextAlignment(Qt::AlignCenter);
        m_rankingModel->setItem(0, 0, tipItem);
        m_rankingModel->setItem(0, 1, new QStandardItem(""));
        m_rankingModel->setItem(0, 2, new QStandardItem(""));
        m_rankingModel->setItem(0, 3, new QStandardItem(""));
        m_rankingModel->setItem(0, 4, new QStandardItem(""));
        m_rankingModel->setItem(0, 5, new QStandardItem(""));
        m_rankingModel->setItem(0, 6, new QStandardItem(""));
        return;
    }

    // 检查是否选择了考试日期
    if (examDate == "所有日期" || examDate.isEmpty()) {
        // 清空排名表格并显示提示
        m_rankingModel->setRowCount(1);
        QStandardItem* tipItem = new QStandardItem("请选择考试日期");
        tipItem->setTextAlignment(Qt::AlignCenter);
        m_rankingModel->setItem(0, 0, tipItem);
        m_rankingModel->setItem(0, 1, new QStandardItem(""));
        m_rankingModel->setItem(0, 2, new QStandardItem(""));
        m_rankingModel->setItem(0, 3, new QStandardItem(""));
        m_rankingModel->setItem(0, 4, new QStandardItem(""));
        m_rankingModel->setItem(0, 5, new QStandardItem(""));
        m_rankingModel->setItem(0, 6, new QStandardItem(""));
        return;
    }

    // 根据排名类型获取数据
    QString rankingType = ui->comboRankingType->currentText();
    QList<StudentRank> rankingList;

    if (rankingType == "单科成绩排名") {
        // 检查是否选择了课程
        if (course == "所有课程" || course.isEmpty()) {
            // 清空排名表格并显示提示
            m_rankingModel->setRowCount(1);
            QStandardItem* tipItem = new QStandardItem("请选择具体课程");
            tipItem->setTextAlignment(Qt::AlignCenter);
            m_rankingModel->setItem(0, 0, tipItem);
            m_rankingModel->setItem(0, 1, new QStandardItem(""));
            m_rankingModel->setItem(0, 2, new QStandardItem(""));
            m_rankingModel->setItem(0, 3, new QStandardItem(""));
            m_rankingModel->setItem(0, 4, new QStandardItem(""));
            m_rankingModel->setItem(0, 5, new QStandardItem(""));
            m_rankingModel->setItem(0, 6, new QStandardItem(""));
            return;
        }

        rankingList = DatabaseManager::instance()->getSingleCourseRanking(
            className == "所有班级" ? "" : className,
            course,
            examDate
            );
    } else if (rankingType == "总分成绩排名") {
        rankingList = DatabaseManager::instance()->getTotalScoreRanking(
            className == "所有班级" ? "" : className,
            examDate
            );
    } else {
        // 清空表格
        m_rankingModel->setRowCount(0);
        return;
    }

    // 如果没有数据，显示提示
    if (rankingList.isEmpty()) {
        m_rankingModel->setRowCount(1);
        QStandardItem* tipItem = new QStandardItem("暂无排名数据");
        tipItem->setTextAlignment(Qt::AlignCenter);
        m_rankingModel->setItem(0, 0, tipItem);
        m_rankingModel->setItem(0, 1, new QStandardItem(""));
        m_rankingModel->setItem(0, 2, new QStandardItem(""));
        m_rankingModel->setItem(0, 3, new QStandardItem(""));
        m_rankingModel->setItem(0, 4, new QStandardItem(""));
        m_rankingModel->setItem(0, 5, new QStandardItem(""));
        m_rankingModel->setItem(0, 6, new QStandardItem(""));
        return;
    }

    // 更新排名表格
    updateRankingTable(rankingList);
}

// 排名类型选择变化槽函数
void MainWindow::on_comboRankingType_currentIndexChanged(int index)
{
    Q_UNUSED(index);

    // 当选择单科排名时，启用课程下拉框；选择总分排名时，禁用课程下拉框
    QString rankingType = ui->comboRankingType->currentText();
    ui->comboRankingCourse->setEnabled(rankingType == "单科成绩排名");

    // 更新显示状态
    bool showCourseCombo = (ui->tabWidgetCharts->currentIndex() == 3) &&
                           (rankingType == "单科成绩排名");
    ui->labelRankingCourse->setVisible(showCourseCombo);
    ui->comboRankingCourse->setVisible(showCourseCombo);

    // 自动更新排名
    if (ui->tabWidgetCharts->currentIndex() == 3) { // 排名分析标签页
        on_btnShowRanking_clicked();
    }
}

// 显示排名按钮点击槽函数
void MainWindow::on_btnShowRanking_clicked()
{
    QString className = ui->comboStatsClass->currentText();
    QString course = ui->comboRankingCourse->currentText();
    QString examDate = ui->comboStatsDate->currentText();

    showRankingTable(className, course, examDate);
}

// 菜单动作槽函数实现

void MainWindow::on_actionImport_triggered()
{
    // 调用导入CSV功能
    on_btnImportCSV_clicked();
}

void MainWindow::on_actionExport_triggered()
{
    // 调用导出报表功能
    on_btnExport_clicked();
}

void MainWindow::on_actionExit_triggered()
{
    // 退出程序
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "退出",
                                  "确定要退出程序吗？",
                                  QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        QApplication::quit();
    }
}

void MainWindow::on_actionAddRecord_triggered()
{
    // 调用添加记录功能
    on_btnAdd_clicked();
}

void MainWindow::on_actionEditRecord_triggered()
{
    // 检查是否有选中的行
    QModelIndexList selected = ui->tableView->selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, "警告", "请先选择要编辑的记录");
        return;
    }

    // 加载选中记录到表单
    loadSelectedScoreToForm();

    // 切换到数据管理标签页
    ui->tabWidget->setCurrentIndex(0);
}

void MainWindow::on_actionDeleteRecord_triggered()
{
    // 调用删除记录功能
    on_btnDelete_clicked();
}

void MainWindow::on_actionRefresh_triggered()
{
    // 调用刷新数据功能
    on_btnRefresh_clicked();
}

void MainWindow::on_actionStatistics_triggered()
{
    // 切换到统计分析标签页
    ui->tabWidget->setCurrentIndex(1);
    updateStatusBar("已切换到统计分析页面");
}

void MainWindow::on_actionCharts_triggered()
{
    // 切换到统计分析标签页中的图表页面
    ui->tabWidget->setCurrentIndex(1);
    updateStatusBar("已切换到图表分析页面");
}

void MainWindow::on_actionReports_triggered()
{
    // 调用生成报告功能
    on_btnGenerateReport_clicked();
}

void MainWindow::on_actionAbout_triggered()
{
    // 显示关于对话框
    QMessageBox::about(this, "关于学生成绩与分析系统",
                       "<h2>学生成绩与分析系统</h2>"
                       "<p>版本: 1.0.0</p>"
                       "<p>数据库路径: C:/Users/bill/Desktop/student_scores.db</p>"
                       "<p>开发: 慕容显欢 2023414290427</p>");
}

// ==================== 原有的按钮槽函数 ====================

void MainWindow::on_btnAdd_clicked()
{
    if (ui->editStudentId->text().isEmpty() || ui->editStudentName->text().isEmpty()) {
        QMessageBox::warning(this, "警告", "请填写学号和姓名");
        return;
    }

    if (ui->comboClass->currentText().isEmpty()) {
        QMessageBox::warning(this, "警告", "请选择班级");
        return;
    }

    if (ui->comboCourse->currentText().isEmpty()) {
        QMessageBox::warning(this, "警告", "请选择课程");
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
        refreshFilterCombos(); // 刷新筛选下拉框
        updateStatusBar("添加成绩成功");
        ui->labelRecordCount->setText(QString("总记录数: %1").arg(m_scoreModel->rowCount()));

        // 如果这是第一条记录，更新图表
        if (m_scoreModel->rowCount() == 1) {
            setupCharts();
        }
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
        refreshFilterCombos(); // 刷新筛选下拉框
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
            refreshFilterCombos(); // 刷新筛选下拉框
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
    refreshFilterCombos(); // 刷新筛选下拉框
    updateStatusBar("数据已刷新");
    ui->labelRecordCount->setText(QString("总记录数: %1").arg(m_scoreModel->rowCount()));
}

void MainWindow::on_btnImportCSV_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, "选择CSV文件", "", "CSV文件 (*.csv);;所有文件 (*.*)");
    if (filePath.isEmpty()) return;

    // 确认导入
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "确认导入",
                                  QString("确定要从文件导入数据吗？\n文件路径: %1").arg(filePath),
                                  QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes) return;

    if (DatabaseManager::instance()->importFromCSV(filePath)) {
        m_scoreModel->refreshData();
        refreshFilterCombos(); // 刷新筛选下拉框
        updateStatusBar("CSV导入成功");
        ui->labelRecordCount->setText(QString("总记录数: %1").arg(m_scoreModel->rowCount()));

        // 更新图表
        setupCharts();
    } else {
        QMessageBox::warning(this, "错误", "CSV导入失败");
    }
}

void MainWindow::on_btnExport_clicked()
{
    if (m_scoreModel->rowCount() == 0) {
        QMessageBox::warning(this, "警告", "没有数据可以导出");
        return;
    }

    QString defaultFileName = QString("学生成绩_%1.csv")
                                  .arg(QDate::currentDate().toString("yyyyMMdd"));
    QString filePath = QFileDialog::getSaveFileName(this, "导出报表", defaultFileName, "CSV文件 (*.csv);;所有文件 (*.*)");
    if (filePath.isEmpty()) return;

    if (!filePath.endsWith(".csv", Qt::CaseInsensitive)) {
        filePath += ".csv";
    }

    if (DatabaseManager::instance()->exportToCSV(filePath)) {
        updateStatusBar(QString("报表已导出到: %1").arg(filePath));
        QMessageBox::information(this, "导出成功",
                                 QString("成功导出 %1 条记录到:\n%2")
                                     .arg(m_scoreModel->rowCount())
                                     .arg(filePath));
    } else {
        QMessageBox::warning(this, "错误", "报表导出失败");
    }
}

void MainWindow::on_btnCalculateStats_clicked()
{
    if (m_scoreModel->rowCount() == 0) {
        QMessageBox::warning(this, "警告", "没有数据可以统计");
        return;
    }

    QString className = ui->comboStatsClass->currentText();
    QString course = ui->comboStatsCourse->currentText();
    QString examDate = ui->comboStatsDate->currentText();

    QMap<QString, QVariant> stats = DatabaseManager::instance()->calculateStatistics(
        className == "所有班级" ? "" : className,
        course == "所有课程" ? "" : course,
        examDate == "所有日期" ? "" : examDate
        );

    // 更新统计结果标签
    ui->labelAvgValue->setText(QString::number(stats["avg"].toDouble(), 'f', 2));
    ui->labelMaxValue->setText(QString::number(stats["max"].toDouble(), 'f', 2));
    ui->labelMinValue->setText(QString::number(stats["min"].toDouble(), 'f', 2));
    ui->labelStdDevValue->setText(QString::number(stats["std_dev"].toDouble(), 'f', 2));
    ui->labelPassRateValue->setText(QString::number(stats["pass_rate"].toDouble(), 'f', 2) + "%");
    ui->labelCountValue->setText(QString::number(stats["count"].toInt()));

    // 更新图表
    showHistogramChart(className, course, examDate);
    showTrendChart(className, course, examDate);
    showComparisonChart(className, examDate);

    // 新增：如果当前在排名标签页，更新排名
    if (ui->tabWidgetCharts->currentIndex() == 3) { // 排名分析标签页
        on_btnShowRanking_clicked();
    }

    updateStatusBar("统计计算完成");
}

// 修改：简化图表显示函数，使用具体日期点
void MainWindow::showHistogramChart(const QString &className, const QString &course,
                                    const QString &examDate)
{
    // 从数据库获取实际的成绩分布数据
    QList<QMap<QString, QVariant>> distribution = DatabaseManager::instance()->getScoreDistribution(
        className == "所有班级" ? "" : className,
        course == "所有课程" ? "" : course,
        examDate == "所有日期" ? "" : examDate,
        5  // 使用5个区间
        );

    if (distribution.isEmpty()) {
        // 如果没有数据，显示空图表
        QChart *emptyChart = new QChart();
        emptyChart->setTitle("暂无数据");
        ui->chartViewHistogram->setChart(emptyChart);
        return;
    }

    // 创建柱状图
    QChart *chart = new QChart();

    // 构建标题
    QString title = "成绩分布";
    if (className != "所有班级") title += " - " + className;
    if (course != "所有课程") title += " " + course;
    if (examDate != "所有日期" && !examDate.isEmpty()) {
        title += QString(" (%1)").arg(examDate);
    }
    chart->setTitle(title);

    QBarSeries *series = new QBarSeries();
    QBarSet *set = new QBarSet("人数分布");

    // 提取区间标签和人数
    QStringList categories;
    QVector<int> counts;

    for (const auto &bin : distribution) {
        categories << bin["range"].toString();
        counts << bin["count"].toInt();
    }

    // 添加数据到柱状图
    for (int count : counts) {
        *set << count;
    }

    series->append(set);
    chart->addSeries(series);

    // 设置X轴
    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(categories);
    axisX->setTitleText("成绩区间");
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    // 设置Y轴
    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText("人数");
    axisY->setLabelFormat("%d");

    // 设置Y轴范围，让最大值稍大一些以便显示
    int maxCount = 0;
    for (int count : counts) {
        if (count > maxCount) maxCount = count;
    }
    axisY->setRange(0, maxCount + 1);

    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    // 显示图例
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);

    ui->chartViewHistogram->setChart(chart);
}

void MainWindow::showTrendChart(const QString &className, const QString &course,
                                const QString &examDate)
{
    // 检查是否选择了课程
    if (course == "所有课程" || course.isEmpty()) {
        // 如果没有选择具体课程，显示提示信息
        QChart *emptyChart = new QChart();
        emptyChart->setTitle("请选择具体课程查看成绩趋势");
        ui->chartViewTrend->setChart(emptyChart);
        return;
    }

    // 从数据库获取课程平均成绩趋势数据
    QList<QMap<QString, QVariant>> trendData = DatabaseManager::instance()->getCourseTrendData(
        className == "所有班级" ? "" : className,
        course,
        examDate == "所有日期" ? "" : examDate
        );

    if (trendData.isEmpty()) {
        // 如果没有数据，显示空图表
        QChart *emptyChart = new QChart();
        emptyChart->setTitle(QString("暂无 %1 的成绩趋势数据").arg(course));
        ui->chartViewTrend->setChart(emptyChart);
        return;
    }

    qDebug() << "趋势数据点数量:" << trendData.size();

    // 创建图表
    QChart *chart = new QChart();

    // 构建标题
    QString title = QString("%1 成绩趋势").arg(course);
    if (className != "所有班级") title += " - " + className;
    if (examDate != "所有日期" && !examDate.isEmpty()) {
        title += QString(" (%1)").arg(examDate);
    }
    chart->setTitle(title);

    // 创建折线系列
    QLineSeries *series = new QLineSeries();
    series->setName(course);
    series->setPointsVisible(true); // 显示数据点
    series->setPointLabelsVisible(true); // 显示数据点标签
    series->setPointLabelsFormat("@yPoint"); // 标签格式为Y值

    // 使用QBarCategoryAxis来显示日期作为分类
    QStringList dateCategories;
    QVector<double> scores;

    // 添加数据点到系列，同时收集日期和分数
    for (const auto &dataPoint : trendData) {
        QDate examDateObj = dataPoint["date_obj"].toDate();
        double avgScore = dataPoint["score"].toDouble();
        int count = dataPoint["count"].toInt();

        // 添加日期到分类列表
        dateCategories << examDateObj.toString("MM-dd");

        // 添加分数
        scores << avgScore;

        // 添加数据点到系列 - 使用索引作为X值
        int index = dateCategories.size() - 1;
        series->append(index, avgScore);

        qDebug() << "趋势数据点[" << index << "]:" << examDateObj.toString("yyyy-MM-dd")
                 << "平均分:" << avgScore << "人数:" << count;
    }

    chart->addSeries(series);

    // 创建分类轴作为X轴
    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(dateCategories);
    axisX->setTitleText("考试日期");

    // 设置X轴范围
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    // 创建Y轴
    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText("平均成绩");
    axisY->setLabelFormat("%.1f");

    // 设置Y轴范围
    double minScore = 100, maxScore = 0;
    for (double score : scores) {
        if (score < minScore) minScore = score;
        if (score > maxScore) maxScore = score;
    }

    // 添加一些边距
    double yMin = std::max(0.0, minScore - 5);
    double yMax = std::min(100.0, maxScore + 5);

    // 如果所有分数相同，调整范围
    if (qFuzzyCompare(minScore, maxScore)) {
        yMin = std::max(0.0, minScore - 10);
        yMax = std::min(100.0, maxScore + 10);
    }

    axisY->setRange(yMin, yMax);

    qDebug() << "Y轴范围:" << yMin << "到" << yMax;

    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    // 设置图表动画
    chart->setAnimationOptions(QChart::SeriesAnimations);

    // 显示图例
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);

    // 设置图表主题
    chart->setTheme(QChart::ChartThemeLight);

    ui->chartViewTrend->setChart(chart);
}

void MainWindow::showComparisonChart(const QString &className,
                                     const QString &examDate)
{
    // 从数据库获取实际课程对比数据
    QList<QMap<QString, QVariant>> comparisonData = DatabaseManager::instance()->getCourseComparison(
        className == "所有班级" ? "" : className,
        examDate == "所有日期" ? "" : examDate
        );

    if (comparisonData.isEmpty()) {
        // 如果没有数据，显示空图表
        QChart *emptyChart = new QChart();
        emptyChart->setTitle("暂无数据");
        ui->chartViewComparison->setChart(emptyChart);
        return;
    }

    QChart *chart = new QChart();

    // 构建标题
    QString title = "课程平均分对比";
    if (className != "所有班级") title += " - " + className;
    if (examDate != "所有日期" && !examDate.isEmpty()) {
        title += QString(" (%1)").arg(examDate);
    }
    chart->setTitle(title);

    // 使用柱状图显示课程对比
    QBarSeries *series = new QBarSeries();

    // 创建柱状图数据
    QBarSet *set = new QBarSet("平均分");

    QStringList categories;
    for (const auto &courseData : comparisonData) {
        QString course = courseData["course"].toString();
        double avgScore = courseData["avg_score"].toDouble();

        categories << course;
        *set << avgScore;
    }

    series->append(set);
    chart->addSeries(series);

    // 设置X轴
    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(categories);
    axisX->setTitleText("课程");
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    // 设置Y轴
    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText("平均分");
    axisY->setRange(0, 150); // 成绩范围0-150
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    // 显示图例
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);

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

// 修改：更新文本搜索函数
void MainWindow::on_editSearch_textChanged(const QString &text)
{
    Q_UNUSED(text);
    updateDataFilter();
}

void MainWindow::on_comboFilterClass_currentTextChanged(const QString &text)
{
    Q_UNUSED(text);
    updateDataFilter();
}

void MainWindow::on_comboFilterCourse_currentTextChanged(const QString &text)
{
    Q_UNUSED(text);
    updateDataFilter();
}

void MainWindow::on_comboStatsClass_currentTextChanged(const QString &text)
{
    Q_UNUSED(text);
}

void MainWindow::on_comboStatsCourse_currentTextChanged(const QString &text)
{
    Q_UNUSED(text);
}

void MainWindow::on_comboFilterDate_currentTextChanged(const QString &text)
{
    Q_UNUSED(text);
    updateDataFilter();
}

void MainWindow::on_comboStatsDate_currentTextChanged(const QString &text)
{
    Q_UNUSED(text);
    updateStatisticsFilter();
}

void MainWindow::updateDataFilter()
{
    QString className = ui->comboFilterClass->currentText();
    QString course = ui->comboFilterCourse->currentText();
    QString examDate = ui->comboFilterDate->currentText();
    QString keyword = ui->editSearch->text();

    m_scoreModel->filterData(
        className == "所有班级" ? "" : className,
        course == "所有课程" ? "" : course,
        examDate == "所有日期" ? "" : examDate,
        keyword
        );

    int recordCount = m_scoreModel->rowCount();
    ui->labelRecordCount->setText(QString("筛选记录数: %1").arg(recordCount));

    // 更新状态栏
    if (examDate != "所有日期" && !examDate.isEmpty()) {
        updateStatusBar(QString("已筛选考试日期: %1，共%2条记录").arg(examDate).arg(recordCount));
    } else {
        updateStatusBar(QString("已筛选，共%1条记录").arg(recordCount));
    }
}

void MainWindow::updateStatisticsFilter()
{
    // 统计筛选无需特殊处理，只更新UI
}

void MainWindow::refreshFilterCombos()
{
    QStringList classes = DatabaseManager::instance()->getAllClasses();
    QStringList courses = DatabaseManager::instance()->getAllCourses();
    QStringList examDates = DatabaseManager::instance()->getAllExamDates();

    ui->comboFilterClass->clear();
    ui->comboFilterClass->addItems(classes);

    ui->comboFilterCourse->clear();
    ui->comboFilterCourse->addItems(courses);

    ui->comboFilterDate->clear();
    ui->comboFilterDate->addItems(examDates);

    ui->comboStatsClass->clear();
    ui->comboStatsClass->addItems(classes);

    ui->comboStatsCourse->clear();
    ui->comboStatsCourse->addItems(courses);

    // 新增：排名课程下拉框
    ui->comboRankingCourse->clear();
    ui->comboRankingCourse->addItems(courses);

    ui->comboStatsDate->clear();
    ui->comboStatsDate->addItems(examDates);

    // 如果班级下拉框为空，添加默认选项
    if (ui->comboClass->count() == 0) {
        ui->comboClass->addItems(QStringList() << "2023级1班" << "2023级2班" << "2023级3班"
                                               << "2024级1班" << "2024级2班" << "2024级3班");
    }

    // 如果课程下拉框为空，添加默认选项 - 增加生物、历史、政治、地理
    if (ui->comboCourse->count() == 0) {
        ui->comboCourse->addItems(QStringList()
                                  << "数学" << "语文" << "英语" << "物理" << "化学"
                                  << "生物" << "历史" << "政治" << "地理");
    }
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
    if (m_scoreModel->rowCount() == 0) {
        QMessageBox::warning(this, "警告", "没有数据可以生成报告");
        return;
    }

    QString className = ui->comboStatsClass->currentText();
    QString course = ui->comboStatsCourse->currentText();
    QString examDate = ui->comboStatsDate->currentText();

    QMap<QString, QVariant> stats = DatabaseManager::instance()->calculateStatistics(
        className == "所有班级" ? "" : className,
        course == "所有课程" ? "" : course,
        examDate == "所有日期" ? "" : examDate
        );

    QString report = QString(
                         "========== 学生成绩分析报告 ==========\n\n"
                         "班级: %1\n"
                         "课程: %2\n"
                         "考试日期: %3\n\n"
                         "========== 统计结果 ==========\n"
                         "平均分: %4\n"
                         "最高分: %5\n"
                         "最低分: %6\n"
                         "标准差: %7\n"
                         "及格率: %8%%\n"
                         "学生人数: %9\n\n"
                         "生成时间: %10\n"
                         "数据库路径: %11\n"
                         "================================="
                         ).arg(className == "所有班级" ? "全部班级" : className)
                         .arg(course == "所有课程" ? "全部课程" : course)
                         .arg(examDate == "所有日期" ? "全部日期" : examDate)
                         .arg(QString::number(stats["avg"].toDouble(), 'f', 2))
                         .arg(QString::number(stats["max"].toDouble(), 'f', 2))
                         .arg(QString::number(stats["min"].toDouble(), 'f', 2))
                         .arg(QString::number(stats["std_dev"].toDouble(), 'f', 2))
                         .arg(QString::number(stats["pass_rate"].toDouble(), 'f', 2))
                         .arg(QString::number(stats["count"].toInt()))
                         .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"))
                         .arg(DatabaseManager::instance()->getDatabasePath());

    QMessageBox::information(this, "学生成绩分析报告", report);
}
