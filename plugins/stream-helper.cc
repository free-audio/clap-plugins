#include "stream-helper.hh"

bool clap::readAll(const clap_istream *is, std::string &data)
{
   char buffer[4096];
   std::ostringstream os;

   while (true) {
      const auto nbytes = is->read(is, buffer, sizeof(buffer));
      if (nbytes < 0)
         return false;
      if (nbytes == 0) {
         data = os.str();
         return true;
      }
      os.write(buffer, nbytes);
   }
}
