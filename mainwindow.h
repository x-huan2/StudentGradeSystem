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

    void showDefaultCharts();  // 添加这个声明
    void showHistogramChart(const QString& className, const QString& course);
    void showTrendChart(const QString& className, const QString& course);
    void showComparisonChart(const QString& className);

    void generateReport();
};

#endif // MAINWINDOW_H
