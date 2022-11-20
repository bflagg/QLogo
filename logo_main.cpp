#include "qlogocontroller.h"
#include <QCoreApplication>
#include <QCommandLineParser>


// Some global options.
// Use 'extern' to access them.
bool hasGUI = false;

void processOptions(QCoreApplication *a)
{
  QCommandLineParser parser;

  QCoreApplication::setApplicationName("logo");
  QCoreApplication::setApplicationVersion(LOGOVERSION);

  parser.setApplicationDescription("UCBLOGO-compatable Logo language Interpreter.");
  parser.addHelpOption();
  parser.addVersionOption();

  parser.addOptions({
                      {"QLogoGUI",
                       QCoreApplication::translate("main",
                       "DO NOT USE! Set the input and output to the format used by "
                       "the QLogo GUI Application. Useless elsewhere.")},
                    });

  parser.process(*a);

  if (parser.isSet("QLogoGUI")) {
      hasGUI = true;
    }
}


int main(int argc, char **argv)
{
  QCoreApplication application(argc, argv);

  processOptions(&application);

  LogoController *mainController;
  if (hasGUI) {
      mainController = new QLogoController;
  } else {
      mainController = new LogoController;
  }
  int retval = mainController->run();
  delete mainController;
  return retval;
}
