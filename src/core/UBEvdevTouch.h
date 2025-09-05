#pragma once

#include <QPointF>

#if defined(Q_OS_LINUX) && defined(HAVE_LIBEVDEV)
#include <QObject>
#include <QHash>
#include <QMutex>
#include <QThread>

struct libevdev;

class UBEvdevTouch : public QThread
{
    Q_OBJECT
public:
    static UBEvdevTouch* instance();
    int majorNear(const QPointF& pos, int radius);
protected:
    void run() override;
private:
    UBEvdevTouch();
    ~UBEvdevTouch() override;
    void initDevice();
    struct SlotInfo {
        int x = 0;
        int y = 0;
        int major = 0;
        int tracking = -1;
    };
    int m_fd;
    libevdev* m_dev;
    int m_currentSlot;
    double m_scaleX;
    double m_scaleY;
    QHash<int, SlotInfo> m_slots;
    QMutex m_mutex;
};

#else

class UBEvdevTouch
{
public:
    static UBEvdevTouch* instance() { static UBEvdevTouch inst; return &inst; }
    int majorNear(const QPointF&, int) { return 0; }
private:
    UBEvdevTouch() = default;
};

#endif
