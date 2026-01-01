#include "scoremodel.h"
#include <QBrush>
#include <QColor>

ScoreModel::ScoreModel(QObject *parent)
    : QAbstractTableModel(parent)
{
    m_headers << "ID" << "学号" << "姓名" << "班级" << "课程" << "成绩" << "考试日期";
    refreshData();
}

int ScoreModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_scores.size();
}

int ScoreModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_headers.size();
}

QVariant ScoreModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (index.row() >= m_scores.size() || index.row() < 0)
        return QVariant();

    const StudentScore& score = m_scores.at(index.row());

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case 0: return score.id;
        case 1: return score.studentId;
        case 2: return score.studentName;
        case 3: return score.className;
        case 4: return score.course;
        case 5: return QString::number(score.score, 'f', 2);
        case 6: return score.examDate.toString("yyyy-MM-dd");
        default: return QVariant();
        }
    } else if (role == Qt::TextAlignmentRole) {
        if (index.column() == 5) // 成绩列居右对齐
            return int(Qt::AlignRight | Qt::AlignVCenter);
        else
            return int(Qt::AlignLeft | Qt::AlignVCenter);
    } else if (role == Qt::BackgroundRole) {
        // 根据成绩设置背景色
        if (index.column() == 5) {
            if (score.score >= 90)
                return QBrush(QColor(200, 255, 200)); // 绿色
            else if (score.score >= 80)
                return QBrush(QColor(255, 255, 200)); // 黄色
            else if (score.score >= 60)
                return QBrush(QColor(255, 230, 200)); // 橙色
            else
                return QBrush(QColor(255, 200, 200)); // 红色
        }
    }

    return QVariant();
}

QVariant ScoreModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal) {
        if (section < m_headers.size())
            return m_headers.at(section);
    }

    return QVariant();
}

void ScoreModel::refreshData()
{
    beginResetModel();
    m_scores = DatabaseManager::instance()->getAllScores();
    endResetModel();
}

// 修改：简化筛选功能，只保留日期参数（具体日期点）
void ScoreModel::filterData(const QString &className, const QString &course,
                            const QString &examDate, const QString &keyword)
{
    beginResetModel();
    m_scores = DatabaseManager::instance()->getScoresByFilter(className, course, examDate, keyword);
    endResetModel();
}

StudentScore ScoreModel::getScoreAt(int row) const
{
    if (row >= 0 && row < m_scores.size())
        return m_scores.at(row);

    // 返回一个空结构体
    static StudentScore emptyScore;
    emptyScore.id = -1;
    emptyScore.score = 0.0;
    return emptyScore;
}
