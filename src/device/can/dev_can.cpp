#include "dev_can.hpp"

#include <stdbool.h>
#include <string.h>

#include "comp_utils.hpp"
#include "om.h"

using namespace Device;

CAN::CAN() { bsp_can_init(); }
