#ifndef SCOREMODEL_H
#define SCOREMODEL_H

#include <QAbstractTableModel>
#include <QList>
#include "databasemanager.h"

class ScoreModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit ScoreModel(QObject *parent = nullptr);

    // 重写虚函数
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // 自定义方法
    void refreshData();
    void filterData(const QString& className, const QString& course, const QString& keyword = "");
    StudentScore getScoreAt(int row) const;

private:
    QList<StudentScore> m_scores;
    QStringList m_headers;
};

#endif // SCOREMODEL_H
