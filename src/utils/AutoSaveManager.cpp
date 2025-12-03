#include "AutoSaveManager.h"

AutoSaveManager& AutoSaveManager::instance()
{
    static AutoSaveManager inst;
    return inst;
}

AutoSaveManager::AutoSaveManager(QObject *parent)
    : QObject(parent)
    , m_intervalSeconds(180)
    , m_isDirty(false)
{
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &AutoSaveManager::onTimerTimeout);
}

AutoSaveManager::~AutoSaveManager()
{
    stop();
}

void AutoSaveManager::start(int intervalSeconds)
{
    m_intervalSeconds = intervalSeconds;
    m_timer->start(intervalSeconds * 1000);
}

void AutoSaveManager::stop()
{
    m_timer->stop();
}

void AutoSaveManager::setInterval(int seconds)
{
    m_intervalSeconds = seconds;
    if (m_timer->isActive()) {
        m_timer->setInterval(seconds * 1000);
    }
}

void AutoSaveManager::markDirty()
{
    m_isDirty = true;
}

void AutoSaveManager::markClean()
{
    m_isDirty = false;
}

void AutoSaveManager::saveNow()
{
    if (m_isDirty) {
        emit autoSaveTriggered();
        m_isDirty = false;
    }
}

void AutoSaveManager::onTimerTimeout()
{
    if (m_isDirty) {
        emit autoSaveTriggered();
        m_isDirty = false;
    }
}
