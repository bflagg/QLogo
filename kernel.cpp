
//===-- qlogo/kernel.cpp - Kernel class implementation -------*- C++ -*-===//
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
/// This file contains the implementation of the Kernel class, which is the
/// executor proper of the QLogo language.
///
//===----------------------------------------------------------------------===//

#include "kernel.h"
#include "parser.h"
//#include <math.h>
#include <QColor>
#include <QFont>
#include <QImage>

#include "error.h"
#include "library.h"
#include "turtle.h"

#include CONTROLLER_HEADER

ProcedureScope::ProcedureScope(Kernel *exec, DatumP procname) {
  procedureHistory = exec->callingProcedure;
  exec->callingProcedure = exec->currentProcedure;
  exec->currentProcedure = procname;
  lineHistory = exec->callingLine;
  exec->callingLine = exec->currentLine;
  kernel = exec;
}

ProcedureScope::~ProcedureScope() {
  kernel->currentProcedure = kernel->callingProcedure;
  kernel->callingProcedure = procedureHistory;
  kernel->currentLine = kernel->callingLine;
  kernel->callingLine = lineHistory;
}

StreamRedirect::StreamRedirect(Kernel *srcExec, QTextStream *newReadStream,
                               QTextStream *newWriteStream) {
  exec = srcExec;
  originalWriteStream = srcExec->writeStream;
  originalSystemWriteStream = srcExec->systemWriteStream;
  originalReadStream = srcExec->readStream;
  originalSystemReadStream = srcExec->systemReadStream;

  exec->writeStream = newWriteStream;
  exec->systemWriteStream = newWriteStream;
  exec->readStream = newReadStream;
  exec->systemReadStream = newReadStream;
}

StreamRedirect::~StreamRedirect() {
  exec->writeStream = originalWriteStream;
  exec->readStream = originalReadStream;
  exec->systemWriteStream = originalSystemWriteStream;
  exec->systemReadStream = originalSystemReadStream;
}

bool Kernel::isInputRedirected() { return readStream != NULL; }

bool Kernel::numbersFromList(QVector<double> &retval, DatumP l) {
  ListIterator iter = l.listValue()->newIterator();

  retval.clear();
  while (iter.elementExists()) {
    DatumP n = iter.element();
    if (!n.isWord())
      return false;
    double v = n.wordValue()->numberValue();
    if (!n.wordValue()->didNumberConversionSucceed())
      return false;
    retval.push_back(v);
  }
  return true;
}

bool Kernel::colorFromDatumP(QColor &retval, DatumP colorP) {
  if (colorP.isWord()) {
    double colorNum = colorP.wordValue()->numberValue();
    if (colorP.wordValue()->didNumberConversionSucceed()) {
      if ((colorNum != round(colorNum)) || (colorNum < 0) ||
          (colorNum >= palette.size()))
        return false;
      retval = palette[colorNum];
      if (!retval.isValid())
        retval = palette[0];
      return true;
    }
    retval = QColor(colorP.wordValue()->printValue().toLower());
    return retval.isValid();
  } else if (colorP.isList()) {
    QVector<double> v;
    if (!numbersFromList(v, colorP.listValue()))
      return false;
    if (v.size() != 3)
      return false;
    for (int i = 0; i < 3; ++i) {
      if ((v[i] < 0) || (v[i] > 100))
        return false;
      v[i] *= 255.0 / 100;
    }
    retval = QColor(v[0], v[1], v[2], 255);
    return true;
  }
  return false;
}

bool Kernel::getLineAndRunIt(bool shouldHandleError) {
  QString prompt;
  if (currentProcedure.isASTNode()) {
    prompt =
        currentProcedure.astnodeValue()->nodeName.wordValue()->printValue();
  }
  prompt += "? ";
  ProcedureScope ps(this, nothing);

  try {
    DatumP line = readlistWithPrompt(prompt, true, systemReadStream);
    if (line == nothing)
      return false; // EOF
    if (line.listValue()->size() == 0)
      return true;

    DatumP result = runList(line);
    if (result != nothing)
      Error::dontSay(result);
  } catch (Error *e) {
    if (shouldHandleError) {
      if (e->tag.isWord() && (e->tag.wordValue()->keyValue() == "TOPLEVEL"))
        return true;
      sysPrint(e->errorText.printValue());
      if (e->procedure != nothing)
        sysPrint(QString(" in ") +
                 e->procedure.astnodeValue()->nodeName.printValue());
      sysPrint("\n");
      if (e->instructionLine != nothing) {
        sysPrint(parser->unreadDatum(e->instructionLine, true));
        sysPrint("\n");
      }
      registerError(nothing);
    } else {
      throw e;
    }
  }
  return true;
}

DatumP Kernel::registerError(DatumP anError, bool allowErract,
                             bool allowRecovery) {
  const QString erract = "ERRACT";
  currentError = anError;
  ProcedureHelper::setIsErroring(anError != nothing);
  if (anError != nothing) {
    Error *e = currentError.errorValue();
    if (e->code == 35) {
      e->procedure = callingProcedure;
      e->instructionLine = callingLine;
    } else {
      e->procedure = currentProcedure;
      e->instructionLine = currentLine;
    }
    DatumP erractP = variables.datumForName(erract);
    bool shouldPause = (erractP != nothing) && (erractP.datumValue()->size() > 0);

    if (allowErract && shouldPause) {
      sysPrint(e->errorText.printValue());
      sysPrint("\n");
      ProcedureHelper::setIsErroring(false);
      currentError = nothing;

      DatumP retval = pause();

      if (retval == nothing)
        Error::throwError(DatumP(new Word("TOPLEVEL")), nothing);
      if (allowRecovery) {
        return retval;
      }
      sysPrint(
          QString("You don't say what to do with %1").arg(retval.printValue()));
      return nothing;
    } else {
      throw anError.errorValue();
    }
  }
  return nothing;
}

void Kernel::initPalette() {
  const int paletteSize = 101;
  palette.clear();
  palette.reserve(paletteSize);
  palette.push_back(QColor(QStringLiteral("black")));       // 0
  palette.push_back(QColor(QStringLiteral("blue")));        // 1
  palette.push_back(QColor(QStringLiteral("green")));       // 2
  palette.push_back(QColor(QStringLiteral("cyan")));        // 3
  palette.push_back(QColor(QStringLiteral("red")));         // 4
  palette.push_back(QColor(QStringLiteral("magenta")));     // 5
  palette.push_back(QColor(QStringLiteral("yellow")));      // 6
  palette.push_back(QColor(QStringLiteral("white")));       // 7
  palette.push_back(QColor(QStringLiteral("brown")));       // 8
  palette.push_back(QColor(QStringLiteral("tan")));         // 9
  palette.push_back(QColor(QStringLiteral("forestgreen"))); // 10
  palette.push_back(QColor(QStringLiteral("aqua")));        // 11
  palette.push_back(QColor(QStringLiteral("salmon")));      // 12
  palette.push_back(QColor(QStringLiteral("purple")));      // 13
  palette.push_back(QColor(QStringLiteral("orange")));      // 14
  palette.push_back(QColor(QStringLiteral("grey")));        // 15
  palette.resize(paletteSize);
  turtle->setPenColor(palette[7]);
}

void Kernel::initLibrary() { executeText(libraryStr); }

Kernel::Kernel() {
  readStream = NULL;
  systemReadStream = NULL;
  writeStream = NULL;
  systemWriteStream = NULL;

  turtle = new Turtle;
  parser = new Parser(this);
  ProcedureHelper::setParser(parser);
  Error::setKernel(this);

  initPalette();

  filePrefix = nothing;

  const QString logoPlatform = "LOGOPLATFORM";
  const QString logoVersion = "LOGOVERSION";

  DatumP platform(new Word(LOGOPLATFORM));
  DatumP version(new Word(LOGOVERSION));
  variables.setDatumForName(platform, logoPlatform);
  variables.setDatumForName(version, logoVersion);
  variables.bury(logoPlatform);
  variables.bury(logoVersion);
}

Kernel::~Kernel() {
  closeAll();
  delete parser;
  delete turtle;
}

// https://stackoverflow.com/questions/2509679/how-to-generate-a-random-number-from-within-a-range
long Kernel::randomFromRange(long start, long end) {
  long max = end - start;

  max = (max < RAND_MAX) ? max : RAND_MAX;

  unsigned long num_bins = (unsigned long)max + 1;
  unsigned long num_rand = (unsigned long)RAND_MAX + 1;
  unsigned long bin_size = num_rand / num_bins;
  unsigned long defect = num_rand % num_bins;

  long x;
  do {
    x = rand();
  } while (num_rand - defect <= (unsigned long)x);

  return x / bin_size + start;
}

DatumP Kernel::readRawLineWithPrompt(const QString prompt,
                                     QTextStream *stream) {
  forever {
    DatumP retval = parser->readrawlineWithPrompt(prompt, stream);
    if (retval == toplevelTokenP)
      Error::throwError(DatumP(new Word("TOPLEVEL")), nothing);
    if (retval == pauseTokenP) {
      pause();
      continue;
    }
    return retval;
  }
}

DatumP Kernel::readChar() {
  if (readStream == NULL) {
    forever {
      DatumP retval = mainController()->readchar();
      if (retval == toplevelTokenP)
        Error::throwError(DatumP(new Word("TOPLEVEL")), nothing);
      if (retval == pauseTokenP) {
        pause();
        continue;
      }
      return retval;
    }
  }

  if (readStream->atEnd())
    return DatumP(new List);
  QString line = readStream->read(1);
  if (readStream->status() != QTextStream::Ok)
    Error::fileSystem();
  return DatumP(new Word(line));
}

DatumP Kernel::readlistWithPrompt(const QString &prompt,
                                  bool shouldRemoveComments,
                                  QTextStream *stream) {
  forever {
    DatumP retval =
        parser->readlistWithPrompt(prompt, shouldRemoveComments, stream);
    if (retval == toplevelTokenP)
      Error::throwError(DatumP(new Word("TOPLEVEL")), nothing);
    if (retval == pauseTokenP) {
      pause();
      continue;
    }
    return retval;
  }
}

DatumP Kernel::readWordWithPrompt(const QString prompt, QTextStream *stream) {
  forever {
    DatumP retval = parser->readwordWithPrompt(prompt, stream);
    if (retval == toplevelTokenP)
      Error::throwError(DatumP(new Word("TOPLEVEL")), nothing);
    if (retval == pauseTokenP) {
      pause();
      continue;
    }
    return retval;
  }
}

void Kernel::makeVarLocal(const QString &varname) {
  if (variables.currentScope() <= 1)
    return;
  if (variables.isStepped(varname)) {
    QString line = varname + " shadowed by local in procedure call";
    if (currentProcedure != nothing) {
      line +=
          " in " +
          currentProcedure.astnodeValue()->nodeName.wordValue()->printValue();
    }
    sysPrint(line + "\n");
  }
  variables.setVarAsLocal(varname);
}

DatumP Kernel::executeProcedureCore(DatumP node) {
  ProcedureHelper h(this, node);
  // The first child is the body of the procedure
  DatumP proc = h.datumAtIndex(0);

  // The remaining children are the parameters
  int childIndex = 1;

  // first assign the REQUIRED params
  QList<QString> &requiredInputs = proc.procedureValue()->requiredInputs;
  for (auto &name : requiredInputs) {
    DatumP value = h.datumAtIndex(childIndex);
    ++childIndex;
    makeVarLocal(name);
    variables.setDatumForName(value, name);
  }

  // then assign the OPTIONAL params
  QList<QString> &optionalInputs = proc.procedureValue()->optionalInputs;
  QList<DatumP> &optionalDefaults = proc.procedureValue()->optionalDefaults;

  auto defaultIter = optionalDefaults.begin();
  for (auto &name : optionalInputs) {
    DatumP value;
    if (childIndex < h.countOfChildren()) {
      value = h.datumAtIndex(childIndex);
      ++childIndex;
    } else {
      value = runList(*defaultIter);
    }
    makeVarLocal(name);
    variables.setDatumForName(value, name);
    ++defaultIter;
  }

  // Finally, take in the remainder (if any) as a list.
  if (proc.procedureValue()->restInput != "") {
    const QString &name = proc.procedureValue()->restInput;
    DatumP remainderList = new List;
    while (childIndex < h.countOfChildren()) {
      DatumP value = h.datumAtIndex(childIndex);
      remainderList.listValue()->append(value);
      ++childIndex;
    }
    makeVarLocal(name);
    variables.setDatumForName(remainderList, name);
  }

  // Execute the commands in the procedure.

  DatumP retval;
  {
    ProcedureScope ps(this, node);
    ListIterator iter =
        proc.procedureValue()->instructionList.listValue()->newIterator();
    bool isStepped = parser->isStepped(
        node.astnodeValue()->nodeName.wordValue()->keyValue());
    while (iter.elementExists() && (retval == nothing)) {
      currentLine = iter.element();
      if (isStepped) {
        QString line = h.indent() + parser->unreadDatum(currentLine, true);
        sysPrint(line);
        readRawLineWithPrompt(" >>>", systemReadStream);
      }
      retval = runList(currentLine);
      if (retval.isASTNode()) {
        ASTNode *a = retval.astnodeValue();
        if (a->kernel == &Kernel::excGotoCore) {
          QString tag = a->childAtIndex(0).wordValue()->keyValue();
          DatumP startingLine = proc.procedureValue()->tagToLine[tag];
          iter =
              proc.procedureValue()->instructionList.listValue()->newIterator();
          while (iter.elementExists() && (currentLine != startingLine)) {
            currentLine = iter.element();
          }
          retval = runList(currentLine, tag);
        }
      }
    }
  }

  if ((retval != nothing) && !retval.isASTNode())
    Error::dontSay(retval);

  if (h.isTraced && retval.isASTNode()) {
    KernelMethod method = retval.astnodeValue()->kernel;
    retval = (this->*method)(retval);
  }
  return h.ret(retval);
}

DatumP Kernel::executeProcedure(DatumP node) {
  Scope s(&variables);

  DatumP retval = executeProcedureCore(node);
  while (retval.isASTNode()) {
    KernelMethod method = retval.astnodeValue()->kernel;
    if (method == &Kernel::executeProcedure) {
      retval = executeProcedureCore(retval);
    } else if (method == &Kernel::excStop) {
      retval = nothing;
    } else {
      retval = (this->*method)(retval);
    }
  }

  return retval;
}

DatumP Kernel::executeMacro(DatumP node) {
  DatumP retval = executeProcedure(node);
  if (!retval.isList())
    return Error::macroReturned(retval);
  return runList(retval);
}

ASTNode *Kernel::astnodeValue(DatumP caller, DatumP value) {
  if (!value.isASTNode())
    Error::doesntLike(caller.astnodeValue()->nodeName, value);
  return value.astnodeValue();
}

DatumP Kernel::executeLiteral(DatumP node) {
  return node.astnodeValue()->childAtIndex(0);
}

DatumP Kernel::executeValueOf(DatumP node) {
  DatumP varnameP = node.astnodeValue()->childAtIndex(0);
  QString varName = varnameP.wordValue()->keyValue();
  DatumP retval = variables.datumForName(varName);
  if (retval == nothing)
    return (Error::noValueRecoverable(varnameP));
  return retval;
}

DatumP Kernel::runList(DatumP listP, const QString startTag) {
  bool shouldSearchForTag = (startTag != "");
  DatumP retval;

  if (listP.isWord())
    listP = parser->runparse(listP);

  if (!listP.isList()) {
    Error::noHow(listP);
  }

  bool tagHasBeenFound = !shouldSearchForTag;

  QList<DatumP> *parsedList = parser->astFromList(listP.listValue());
  for (auto &statement : *parsedList) {
    if (retval != nothing) {
      if (retval.isASTNode()) {
        return retval;
      }
      Error::dontSay(retval);
    }
    KernelMethod method = statement.astnodeValue()->kernel;
    if (tagHasBeenFound) {
      retval = (this->*method)(statement);
    } else {
      if (method == &Kernel::excTag) {
        ASTNode *child =
            statement.astnodeValue()->childAtIndex(0).astnodeValue();
        if (child->kernel == &Kernel::executeLiteral) {
          DatumP v = child->childAtIndex(0);
          if (v.isWord()) {
            QString tag = v.wordValue()->keyValue();
            tagHasBeenFound = (startTag == tag);
          }
        }
      }
    }
  }

  while (!mainController()->eventQueueIsEmpty()) {
    char event = mainController()->nextQueueEvent();
    DatumP action;
    switch (event) {
    case mouseEvent: {
      action = varBUTTONACT();
      break;
    }
    case characterEvent: {
      action = varKEYACT();
      break;
    }
    case toplevelEvent: {
      Error::throwError(DatumP(new Word("TOPLEVEL")), nothing);
      break;
    }
    case pauseEvent: {
      pause();
      break;
    }
    }
    if (action != nothing) {
      DatumP localRetval = runList(action);
      if (localRetval != nothing)
        Error::dontSay(localRetval);
    }
  }

  return retval;
}

DatumP Kernel::excWait(DatumP node) {
  ProcedureHelper h(this, node);
  double value = h.validatedNumberAtIndex(
      0, [](double candidate) { return candidate >= 0; });
  mainController()->mwait((1000.0 / 60) * value);
  return nothing;
}

DatumP Kernel::excNoop(DatumP node) {
  ProcedureHelper h(this, node);
  return h.ret();
}

DatumP Kernel::pause() {
  ProcedureScope procScope(this, nothing);
  PauseScope levelScope(&pauseLevel);
  StreamRedirect streamScope(this, NULL, NULL);

  sysPrint("Pausing...\n");

  forever {
    try {
      bool shouldContinue = true;
      while (shouldContinue) {
        shouldContinue = getLineAndRunIt(false);
      }
    } catch (Error *e) {
      if ((e->code == 14) && (e->tag.wordValue()->keyValue() == "PAUSE")) {
        DatumP retval = e->output;
        registerError(nothing);
        return retval;
      }
      if ((e->code == 14) && (e->tag.wordValue()->keyValue() == "TOPLEVEL")) {
        throw e;
      }
      sysPrint(e->errorText.printValue());
      sysPrint("\n");
      registerError(nothing);
    }
  }
  return nothing;
}
