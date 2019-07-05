#ifndef MAINWINDOW_H
#define MAINWINDOW_H

//===-- qlogo/mainwindow.h - MainWindow class definition -------*- C++ -*-===//
//
// This file is part of QLogo.
//
// QLogo is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// QLogo is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with QLogo.  If not, see <http://www.gnu.org/licenses/>.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the MainWindow class, which is the
/// main window portion of the user interface.
///
//===----------------------------------------------------------------------===//

#include <QMainWindow>
#include <QtGui/QOpenGLFunctions>
#include <QProcess>
#include <QDataStream>

class Canvas;
class Console;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
  Q_OBJECT

    enum windowMode_t {
        windowMode_noWait,
        windowMode_waitForChar,
        windowMode_waitForRawline,
    };

public:
  explicit MainWindow(QWidget *parent = 0);
  ~MainWindow();
  void show();

private:
  Ui::MainWindow *ui;

  QProcess *logoProcess;
  QDataStream logoStream;

  windowMode_t windowMode;

  int startLogo();
  void beginReadRawline();

public slots:
  void readStandardOutput();
  void readStandardError();
  void processStarted();
  void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
  void errorOccurred(QProcess::ProcessError error);

  void sendRawlineSlot(const QString &line);
};

#endif // MAINWINDOW_H
