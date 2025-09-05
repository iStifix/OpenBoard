// UBEvdevTouch.h
#pragma once

#include <QThread>
#include <QMutex>
#include <QPoint>
#include <QVector>

class UBEvdevTouch : public QThread
{
    Q_OBJECT
public:
    static UBEvdevTouch* instance();

    // Ищем ближайший активный слот вокруг глобальной точки (пиксели экрана)
    // Возвращаем "диаметр" (TOUCH_MAJOR) в пикселях экрана. 0, если ничего близко нет.
    int majorNearGlobal(const QPoint& globalPos, int radiusPx);

protected:
    void run() override;

private:
    UBEvdevTouch();
    ~UBEvdevTouch() override;
    void initDevice();

    struct SlotInfo {
        int x = 0;       // в девайс-единицах ABS_MT_POSITION_X
        int y = 0;       // в девайс-единицах ABS_MT_POSITION_Y
        int major = 0;   // ABS_MT_TOUCH_MAJOR (девайс-единицы)
        int tracking = -1; // -1 = пустой слот, иначе активен
    };
    QVector<SlotInfo> m_slots;

    struct AbsRange {
        int min = 0;
        int max = 0;
        inline int span() const { return max - min; }
    };

    // /dev/input
    int m_fd = -1;

#ifdef Q_OS_LINUX
#  ifdef HAVE_LIBEVDEV
    struct libevdev* m_dev = nullptr;
#  else
    void* m_dev = nullptr;
#  endif
#else
    void* m_dev = nullptr;
#endif

    int m_currentSlot = 0;
    AbsRange m_ax;  // ABS_MT_POSITION_X range
    AbsRange m_ay;  // ABS_MT_POSITION_Y range

    QVector<SlotInfo> m_slots;
    QMutex m_mutex;
};

