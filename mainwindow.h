#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>
#include "scoremodel.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // 按钮槽函数
    void on_btnAdd_clicked();
    void on_btnUpdate_clicked();
    void on_btnDelete_clicked();
    void on_btnRefresh_clicked();
    void on_btnImportCSV_clicked();
    void on_btnExport_clicked();
    void on_btnCalculateStats_clicked();
    void on_btnGenerateReport_clicked();

    void on_tableView_doubleClicked(const QModelIndex &index);
    void on_editSearch_textChanged(const QString &text);
    void on_comboFilterClass_currentTextChanged(const QString &text);
    void on_comboFilterCourse_currentTextChanged(const QString &text);
    void on_comboStatsClass_currentTextChanged(const QString &text);
    void on_comboStatsCourse_currentTextChanged(const QString &text);

    // 菜单动作槽函数 - 新增
    void on_actionImport_triggered();
    void on_actionExport_triggered();
    void on_actionExit_triggered();
    void on_actionAddRecord_triggered();
    void on_actionEditRecord_triggered();
    void on_actionDeleteRecord_triggered();
    void on_actionRefresh_triggered();
    void on_actionStatistics_triggered();
    void on_actionCharts_triggered();
    void on_actionReports_triggered();
    void on_actionAbout_triggered();


private:
    Ui::MainWindow *ui;
    ScoreModel *m_scoreModel;

    void setupUI();
    void setupDatabase();
    void setupCharts();
    void refreshFilterCombos();
    void loadSelectedScoreToForm();
    void clearForm();
    void updateStatusBar(const QString &message);

    void showDefaultCharts();
    void showHistogramChart(const QString& className, const QString& course);
    void showTrendChart(const QString& className, const QString& course);
    void showComparisonChart(const QString& className);

    void generateReport();
};

#endif // MAINWINDOW_H
