#include "../util/Version.h"

namespace {
    static const std::string retval = "v0.4 [SVN 4635] Xcode 3";
}

const std::string& FreeOrionVersionString()
{ return retval; }
