#pragma once

#include "Guijo/Guijo.hpp"
#include "Midijo/Midijo.hpp"
#include "Audijo/Audijo.hpp"

using namespace Midijo;
using namespace Guijo;
using namespace Audijo;

#define db2lin(db) std::powf(10.0f, 0.05 * (db))
#define lin2db(lin) (20.0f * std::log10(std::max((double)(lin), 0.000000000001)))
