#ifndef INPUTQUEUETHREAD_H
#define INPUTQUEUETHREAD_H

#include <QThread>
#include <QByteArrayList>
#include <QMutexLocker>

class InputQueueThread : public QThread
{
    bool dataIsAvailable = false;
    QByteArrayList list;
    QMutex mutex;

    void run() override;
public:
    explicit InputQueueThread(QObject *parent = nullptr);

    /// Get a message.
    /// If no message is available this will simply return an empty QByteArray
    QByteArray getMessage();

    /// Clear the Queue.
    /// Necessary after interrupt.
    void clearQueue();

    /// No mutex for efficiency.
    /// TRUE: Data is probably available.
    /// FALSE: Data is probably not available.
    bool queueHasData() { return dataIsAvailable; }


};

#endif // INPUTQUEUETHREAD_H
