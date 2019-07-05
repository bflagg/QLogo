#ifndef MESSAGECATEGORIES_H
#define MESSAGECATEGORIES_H

#include <QChar>
#include <QDebug>

using message_t = quint8;

enum messageCategory : message_t {
    C_CONSOLE_PRINT_STRING, // Print text to the GUI
    C_CONSOLE_REQUEST_LINE, // Ask the GUI for a raw line.
    C_CONSOLE_RAWLINE_READ, // A line read returned from the GUI
};

const QChar escapeChar = 27;

#define dv(x) qDebug()<<#x<<'='<<x

#endif // MESSAGECATEGORIES_H
