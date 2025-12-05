#include "PracticeStatsPanel.h"
#include "../core/ProgressManager.h"
#include <QPainter>
#include <QToolTip>
#include <QMouseEvent>
#include <QDateTime>
#include <QProgressBar>

// 热力图单元格组件
class HeatMapCell : public QWidget
{
public:
    HeatMapCell(const QDate &date, int count, QWidget *parent = nullptr)
        : QWidget(parent), m_date(date), m_count(count)
    {
        setFixedSize(12, 12);
        setToolTip(QString("%1\n%2 道题目")
            .arg(date.toString("yyyy-MM-dd"))
            .arg(count));
    }
    
protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        
        // 根据题目数量选择颜色（红色主题渐变）
        QColor color;
        if (m_count == 0) {
            color = QColor("#2d2d2d");  // 无活动 - 深灰
        } else if (m_count <= 2) {
            color = QColor("#440000");  // 1-2题 - 深红
        } else if (m_count <= 5) {
            color = QColor("#660000");  // 3-5题 - 中红
        } else if (m_count <= 10) {
            color = QColor("#880000");  // 6-10题 - 亮红
        } else {
            color = QColor("#aa0000");  // 10+题 - 鲜红
        }
        
        painter.setBrush(color);
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(rect(), 2, 2);
    }
    
private:
    QDate m_date;
    int m_count;
};

PracticeStatsPanel::PracticeStatsPanel(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    // 初始化时显示空数据
    updateStats(0, 0, 0, 0);
}

void PracticeStatsPanel::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    
    // 标题
    QLabel *titleLabel = new QLabel("📊 刷题统计", this);
    titleLabel->setStyleSheet("font-size: 14pt; font-weight: bold; color: #e8e8e8; margin-bottom: 4px;");
    mainLayout->addWidget(titleLabel);
    
    // 创建各个统计模块
    createStreakInfo();
    createHeatMap();
    createDifficultyChart();
    createRecentActivity();
    
    mainLayout->addWidget(m_streakWidget);
    mainLayout->addWidget(m_heatMapWidget);
    mainLayout->addWidget(m_difficultyWidget);
    mainLayout->addWidget(m_activityWidget);
    // 移除 addStretch()，让内容自然展开
}

void PracticeStatsPanel::createStreakInfo()
{
    m_streakWidget = new QWidget(this);
    QHBoxLayout *layout = new QHBoxLayout(m_streakWidget);
    layout->setSpacing(12);
    layout->setContentsMargins(0, 0, 0, 0);
    
    // 创建统计卡片
    auto createStatCard = [](const QString &title, const QString &value, const QString &icon) {
        QFrame *card = new QFrame();
        card->setStyleSheet(
            "QFrame {"
            "    background-color: #2d2d2d;"
            "    border-radius: 12px;"
            "    padding: 16px;"
            "}"
        );
        
        QVBoxLayout *cardLayout = new QVBoxLayout(card);
        cardLayout->setSpacing(8);
        
        QLabel *iconLabel = new QLabel(icon);
        iconLabel->setStyleSheet("font-size: 24pt;");
        iconLabel->setAlignment(Qt::AlignCenter);
        
        QLabel *valueLabel = new QLabel(value);
        valueLabel->setStyleSheet("font-size: 20pt; font-weight: bold; color: #e8e8e8;");
        valueLabel->setAlignment(Qt::AlignCenter);
        
        QLabel *titleLabel = new QLabel(title);
        titleLabel->setStyleSheet("color: #b0b0b0; font-size: 9pt;");
        titleLabel->setAlignment(Qt::AlignCenter);
        
        cardLayout->addWidget(iconLabel);
        cardLayout->addWidget(valueLabel);
        cardLayout->addWidget(titleLabel);
        
        return card;
    };
    
    // 创建可更新的统计卡片（无边框扁平样式）
    auto createUpdatableCard = [](const QString &title, const QString &icon, QLabel **valueLabel) {
        QWidget *card = new QWidget();
        card->setStyleSheet(
            "QWidget {"
            "    background-color: transparent;"
            "}"
        );
        
        QVBoxLayout *cardLayout = new QVBoxLayout(card);
        cardLayout->setSpacing(8);
        cardLayout->setContentsMargins(8, 8, 8, 8);
        
        QLabel *iconLabel = new QLabel(icon);
        iconLabel->setStyleSheet("font-size: 18pt; color: #e8e8e8;");
        iconLabel->setAlignment(Qt::AlignCenter);
        
        *valueLabel = new QLabel("0");
        (*valueLabel)->setStyleSheet("font-size: 24pt; font-weight: bold; color: #ffffff;");
        (*valueLabel)->setAlignment(Qt::AlignCenter);
        
        QLabel *titleLabel = new QLabel(title);
        titleLabel->setStyleSheet("font-size: 10pt; color: #aaa;");
        titleLabel->setAlignment(Qt::AlignCenter);
        
        cardLayout->addWidget(iconLabel);
        cardLayout->addWidget(*valueLabel);
        cardLayout->addWidget(titleLabel);
        
        return card;
    };
    
    layout->addWidget(createUpdatableCard("总完成", "✅", &m_totalSolvedLabel));
    layout->addWidget(createUpdatableCard("当前连续", "🔥", &m_currentStreakLabel));
    layout->addWidget(createUpdatableCard("最长连续", "⭐", &m_longestStreakLabel));
    layout->addWidget(createUpdatableCard("今日完成", "📅", &m_todayCountLabel));
}

void PracticeStatsPanel::createHeatMap()
{
    m_heatMapWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(m_heatMapWidget);
    layout->setSpacing(10);
    layout->setContentsMargins(0, 0, 0, 0);
    
    QLabel *titleLabel = new QLabel("📅 活动热力图（最近12周）", this);
    titleLabel->setStyleSheet("font-size: 11pt; font-weight: bold; color: #e8e8e8;");
    layout->addWidget(titleLabel);
    
    // 创建热力图容器（带星期标签）
    QWidget *heatMapContainer = new QWidget(this);
    QGridLayout *gridLayout = new QGridLayout(heatMapContainer);
    gridLayout->setSpacing(3);
    gridLayout->setContentsMargins(20, 0, 0, 0);  // 左侧留空间给星期标签
    
    // 添加星期标签（左侧）
    QStringList weekDays = {"", "一", "", "三", "", "五", ""};  // 只显示奇数行
    for (int i = 0; i < 7; i++) {
        if (!weekDays[i].isEmpty()) {
            QLabel *dayLabel = new QLabel(weekDays[i], heatMapContainer);
            dayLabel->setStyleSheet("font-size: 8pt; color: #666;");
            dayLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            dayLabel->setFixedWidth(15);
            gridLayout->addWidget(dayLabel, i, 0);
        }
    }
    
    // 获取活动数据
    QMap<QDate, int> activityData = getActivityData();
    
    // 显示最近12周（84天）
    QDate today = QDate::currentDate();
    QDate startDate = today.addDays(-83);  // 12周前
    
    // 调整开始日期到周一
    int daysToMonday = startDate.dayOfWeek() - 1;  // Qt::Monday = 1
    if (daysToMonday > 0) {
        startDate = startDate.addDays(-daysToMonday);
    }
    
    // 按周组织（每列是一周）
    int week = 1;  // 从第1列开始（第0列是星期标签）
    QDate currentDate = startDate;
    
    while (currentDate <= today) {
        // 一周7天
        for (int dayOfWeek = 0; dayOfWeek < 7; dayOfWeek++) {
            if (currentDate > today) break;
            
            int count = activityData.value(currentDate, 0);
            HeatMapCell *cell = new HeatMapCell(currentDate, count, heatMapContainer);
            
            gridLayout->addWidget(cell, dayOfWeek, week);
            
            currentDate = currentDate.addDays(1);
        }
        week++;
    }
    
    layout->addWidget(heatMapContainer);
    
    // 图例
    QHBoxLayout *legendLayout = new QHBoxLayout();
    legendLayout->setSpacing(6);
    legendLayout->addStretch();
    
    QLabel *lessLabel = new QLabel("少", this);
    lessLabel->setStyleSheet("font-size: 9pt; color: #999;");
    legendLayout->addWidget(lessLabel);
    
    for (int i = 0; i < 5; i++) {
        QWidget *colorBox = new QWidget(this);
        colorBox->setFixedSize(12, 12);
        QColor colors[] = {
            QColor("#2d2d2d"),  // 0题
            QColor("#440000"),  // 1-2题
            QColor("#660000"),  // 3-5题
            QColor("#880000"),  // 6-10题
            QColor("#aa0000")   // 10+题
        };
        colorBox->setStyleSheet(QString("background-color: %1; border-radius: 2px;")
            .arg(colors[i].name()));
        legendLayout->addWidget(colorBox);
    }
    
    QLabel *moreLabel = new QLabel("多", this);
    moreLabel->setStyleSheet("font-size: 9pt; color: #999;");
    legendLayout->addWidget(moreLabel);
    
    layout->addLayout(legendLayout);
}

void PracticeStatsPanel::createDifficultyChart()
{
    m_difficultyWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(m_difficultyWidget);
    layout->setSpacing(10);
    layout->setContentsMargins(0, 0, 0, 0);
    
    QLabel *titleLabel = new QLabel("📈 难度分布", this);
    titleLabel->setStyleSheet("font-size: 11pt; font-weight: bold; color: #e8e8e8;");
    layout->addWidget(titleLabel);
    
    // 获取难度统计
    QMap<QString, int> diffStats = getDifficultyStats();
    
    // 创建进度条
    auto createProgressBar = [](const QString &label, int value, int total, const QColor &color) {
        QWidget *widget = new QWidget();
        QHBoxLayout *layout = new QHBoxLayout(widget);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(16);
        
        QLabel *labelWidget = new QLabel(label);
        labelWidget->setFixedWidth(70);
        labelWidget->setStyleSheet("color: #e8e8e8; font-size: 10pt; padding: 6px 0px;");
        // 10pt * 1.5 = 15pt，需要约36px
        labelWidget->setMinimumHeight(36);
        
        QProgressBar *bar = new QProgressBar();
        bar->setMaximum(total > 0 ? total : 100);
        bar->setValue(value);
        bar->setTextVisible(true);
        bar->setFormat(QString("%1 / %2").arg(value).arg(total));
        // 9pt字体 * 1.5 = 13.5pt，进度条高度增加到45px
        bar->setFixedHeight(45);
        bar->setStyleSheet(QString(
            "QProgressBar {"
            "    border: 1px solid #4a4a4a;"
            "    border-radius: 22px;"
            "    background-color: #2d2d2d;"
            "    text-align: center;"
            "    color: #e8e8e8;"
            "    font-size: 9pt;"
            "    padding: 2px;"
            "}"
            "QProgressBar::chunk {"
            "    background-color: %1;"
            "    border-radius: 19px;"
            "    margin: 2px;"
            "}"
        ).arg(color.name()));
        
        layout->addWidget(labelWidget);
        layout->addWidget(bar);
        
        return widget;
    };
    
    int easyTotal = diffStats.value("easy_total", 0);
    int easyCompleted = diffStats.value("easy_completed", 0);
    int mediumTotal = diffStats.value("medium_total", 0);
    int mediumCompleted = diffStats.value("medium_completed", 0);
    int hardTotal = diffStats.value("hard_total", 0);
    int hardCompleted = diffStats.value("hard_completed", 0);
    
    layout->addWidget(createProgressBar("简单", easyCompleted, easyTotal, QColor("#00aa00")));
    layout->addWidget(createProgressBar("中等", mediumCompleted, mediumTotal, QColor("#ffaa00")));
    layout->addWidget(createProgressBar("困难", hardCompleted, hardTotal, QColor("#ff0000")));
}

void PracticeStatsPanel::createRecentActivity()
{
    m_activityWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(m_activityWidget);
    layout->setSpacing(10);
    layout->setContentsMargins(0, 0, 0, 0);
    
    QLabel *titleLabel = new QLabel("🕐 最近活动", this);
    titleLabel->setStyleSheet("font-size: 11pt; font-weight: bold; color: #e8e8e8;");
    layout->addWidget(titleLabel);
    
    // TODO: 显示最近完成的题目
    QLabel *placeholderLabel = new QLabel("暂无最近活动", this);
    placeholderLabel->setStyleSheet("color: #888; font-style: italic; font-size: 10pt; padding: 8px;");
    layout->addWidget(placeholderLabel);
}



QMap<QDate, int> PracticeStatsPanel::getActivityData() const
{
    QMap<QDate, int> activityData;
    
    // TODO: 从 ProgressManager 获取每日活动数据
    // 这需要在 ProgressManager 中添加按日期统计的功能
    
    return activityData;
}

int PracticeStatsPanel::getCurrentStreak() const
{
    // TODO: 计算当前连续刷题天数
    return 0;
}

int PracticeStatsPanel::getLongestStreak() const
{
    // TODO: 计算最长连续刷题天数
    return 0;
}

QMap<QString, int> PracticeStatsPanel::getDifficultyStats() const
{
    QMap<QString, int> stats;
    
    // TODO: 从 ProgressManager 获取难度统计
    
    return stats;
}

// 更新统计数据的实现

void PracticeStatsPanel::updateStats(int totalCompleted, int currentStreak, int longestStreak, int todayCompleted)
{
    if (m_totalSolvedLabel) {
        m_totalSolvedLabel->setText(QString::number(totalCompleted));
    }
    if (m_currentStreakLabel) {
        m_currentStreakLabel->setText(QString::number(currentStreak));
    }
    if (m_longestStreakLabel) {
        m_longestStreakLabel->setText(QString::number(longestStreak));
    }
    if (m_todayCountLabel) {
        m_todayCountLabel->setText(QString::number(todayCompleted));
    }
}

void PracticeStatsPanel::updateHeatMap(const QMap<QDate, int> &activityData)
{
    // 重新创建热力图
    if (m_heatMapWidget) {
        // 清除旧的热力图
        QLayout *oldLayout = m_heatMapWidget->layout();
        if (oldLayout) {
            QLayoutItem *item;
            while ((item = oldLayout->takeAt(0)) != nullptr) {
                delete item->widget();
                delete item;
            }
            delete oldLayout;
        }
        
        // 创建新的热力图布局
        QGridLayout *heatMapLayout = new QGridLayout(m_heatMapWidget);
        heatMapLayout->setSpacing(3);
        heatMapLayout->setContentsMargins(20, 0, 0, 0);  // 左侧留空间给星期标签
        
        // 添加星期标签（左侧）
        QStringList weekDays = {"", "一", "", "三", "", "五", ""};  // 只显示奇数行
        for (int i = 0; i < 7; i++) {
            if (!weekDays[i].isEmpty()) {
                QLabel *dayLabel = new QLabel(weekDays[i], m_heatMapWidget);
                dayLabel->setStyleSheet("font-size: 8pt; color: #666;");
                dayLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
                dayLabel->setFixedWidth(15);
                heatMapLayout->addWidget(dayLabel, i, 0);
            }
        }
        
        // 获取日期范围
        QDate today = QDate::currentDate();
        QDate startDate = today.addDays(-83);  // 12周前
        
        // 调整开始日期到周一
        int daysToMonday = startDate.dayOfWeek() - 1;  // Qt::Monday = 1
        if (daysToMonday > 0) {
            startDate = startDate.addDays(-daysToMonday);
        }
        
        // 按周组织（每列是一周）
        int week = 1;  // 从第1列开始（第0列是星期标签）
        QDate currentDate = startDate;
        
        while (currentDate <= today) {
            // 一周7天
            for (int dayOfWeek = 0; dayOfWeek < 7; dayOfWeek++) {
                if (currentDate > today) break;
                
                int count = activityData.value(currentDate, 0);
                
                // 创建热力图单元格
                QLabel *cell = new QLabel(m_heatMapWidget);
                cell->setFixedSize(12, 12);
                cell->setToolTip(QString("%1\n%2 道题目")
                    .arg(currentDate.toString("yyyy-MM-dd"))
                    .arg(count));
                
                // 根据数量设置颜色深度（红色主题渐变）
                QString color;
                if (count == 0) {
                    color = "#2d2d2d";  // 无活动
                } else if (count <= 2) {
                    color = "#440000";  // 1-2题
                } else if (count <= 5) {
                    color = "#660000";  // 3-5题
                } else if (count <= 10) {
                    color = "#880000";  // 6-10题
                } else {
                    color = "#aa0000";  // 10+题
                }
                
                cell->setStyleSheet(QString(
                    "QLabel {"
                    "    background-color: %1;"
                    "    border-radius: 2px;"
                    "}"
                ).arg(color));
                
                heatMapLayout->addWidget(cell, dayOfWeek, week);
                
                currentDate = currentDate.addDays(1);
            }
            week++;
        }
    }
}

void PracticeStatsPanel::updateDifficultyDistribution(int easyCompleted, int easyTotal,
                                                       int mediumCompleted, int mediumTotal,
                                                       int hardCompleted, int hardTotal)
{
    if (!m_difficultyWidget) return;
    
    // 查找进度条并更新
    QList<QProgressBar*> progressBars = m_difficultyWidget->findChildren<QProgressBar*>();
    QList<QLabel*> labels = m_difficultyWidget->findChildren<QLabel*>();
    
    if (progressBars.size() >= 3) {
        // 简单
        int easyPercent = easyTotal > 0 ? (easyCompleted * 100 / easyTotal) : 0;
        progressBars[0]->setValue(easyPercent);
        progressBars[0]->setFormat(QString("%1/%2 (%3%)").arg(easyCompleted).arg(easyTotal).arg(easyPercent));
        
        // 中等
        int mediumPercent = mediumTotal > 0 ? (mediumCompleted * 100 / mediumTotal) : 0;
        progressBars[1]->setValue(mediumPercent);
        progressBars[1]->setFormat(QString("%1/%2 (%3%)").arg(mediumCompleted).arg(mediumTotal).arg(mediumPercent));
        
        // 困难
        int hardPercent = hardTotal > 0 ? (hardCompleted * 100 / hardTotal) : 0;
        progressBars[2]->setValue(hardPercent);
        progressBars[2]->setFormat(QString("%1/%2 (%3%)").arg(hardCompleted).arg(hardTotal).arg(hardPercent));
    }
}
