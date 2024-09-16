#include "undo-proxy.hh"
#include "abstract-gui-listener.hh"
#include "gui.hh"

namespace clap {
   UndoProxy::UndoProxy(Gui &client) : _client(client) {}

   void UndoProxy::setCanUndo(bool can) {
      if (_canUndo == can)
         return;
      _canUndo = can;
      canUndoChanged();
   }

   void UndoProxy::setCanRedo(bool can) {
      if (_canRedo == can)
         return;
      _canRedo = can;
      canRedoChanged();
   }

   void UndoProxy::setUndoName(QString name) {
      if (_undoName == name)
         return;
      _undoName = std::move(name);
      undoNameChanged();
   }

   void UndoProxy::setRedoName(QString name) {
      if (_redoName == name)
         return;
      _redoName = std::move(name);
      redoNameChanged();
   }

   void UndoProxy::undo() {
      if (_canUndo)
         _client.guiListener().onGuiUndo();
   }

   void UndoProxy::redo() {
      if (_canRedo)
         _client.guiListener().onGuiRedo();
   }
} // namespace clap