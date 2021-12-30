#include <QtPlugin>

#include "application.hh"

//Q_IMPORT_PLUGIN(qtquick2plugin)

int main(int argc, char **argv) {
   Application app(argc, argv);

   return app.exec();
}