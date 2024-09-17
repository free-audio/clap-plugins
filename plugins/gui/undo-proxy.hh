#pragma once

#include <QObject>
#include <QString>

namespace clap {

   class Gui;
   class UndoProxy : public QObject {
      using super = QObject;

      Q_OBJECT

      Q_PROPERTY(bool canUndo READ canUndo NOTIFY canUndoChanged)
      Q_PROPERTY(bool canRedo READ canRedo NOTIFY canRedoChanged)
      Q_PROPERTY(QString undoName READ undoName NOTIFY undoNameChanged)
      Q_PROPERTY(QString redoName READ redoName NOTIFY redoNameChanged)

   public:
      explicit UndoProxy(Gui &client);

      void setCanUndo(bool can);
      void setCanRedo(bool can);

      void setUndoName(QString name);
      void setRedoName(QString name);

      [[nodiscard]] const bool canUndo() const noexcept { return _canUndo; }
      [[nodiscard]] const bool canRedo() const noexcept { return _canRedo; }

      [[nodiscard]] const QString &undoName() const noexcept { return _undoName; }
      [[nodiscard]] const QString &redoName() const noexcept { return _redoName; }

      Q_INVOKABLE void undo();
      Q_INVOKABLE void redo();

   signals:
      void canUndoChanged();
      void canRedoChanged();
      void undoNameChanged();
      void redoNameChanged();

   private:
      Gui &_client;
      QString _undoName;
      QString _redoName;
      bool _canUndo{false};
      bool _canRedo{false};
   };

} // namespace clap