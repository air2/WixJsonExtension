#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
// Windows Header Files:
#include <windows.h>
#include <msiquery.h>
#include <strsafe.h>

// WiX Header Files:
#include <wcautil.h>
#include <wcawow64.h>
#include <strutil.h>
#include <iostream>
#include <fstream>
#include <comdef.h> 
#include <regex>
#include <cstdint>
#include <filesystem>
// TODO: reference additional headers your program requires here
#include "jsoncons/json.hpp"
#include "jsoncons_ext/jsonpath/jsonpath.hpp"
