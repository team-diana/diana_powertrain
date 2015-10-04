#ifndef DIANA_POWERTRAIN_CONSTS_HPP
#define DIANA_POWERTRAIN_CONSTS_HPP

#include <hlcanopen/types.hpp>

const hlcanopen::SDOIndex ERROR_REGISTER(0x1001, 0);
const hlcanopen::SDOIndex MANUFACTURER_DEVICE_NAME(0x1008, 0);
const hlcanopen::SDOIndex MANUFACTURER_HARDWARE_VERSION(0x1009, 0);
const hlcanopen::SDOIndex MANUFACTURER_SOFTWARE_VERSION(0x100A, 0);
const hlcanopen::SDOIndex ERROR_BEHAVIOUR_READ(0x1029, 0);
const hlcanopen::SDOIndex ERROR_BEHAVIOUR_WRITE(0x1029, 1);
const hlcanopen::SDOIndex OS_COMMAND_MODE(0x1024, 0);
const hlcanopen::SDOIndex OS_COMMAND_PROMPT_WRITE = hlcanopen::SDOIndex(0x1023, 1);
const hlcanopen::SDOIndex OS_COMMAND_PROMPT_STATUS = hlcanopen::SDOIndex(0x1023, 2);
const hlcanopen::SDOIndex OS_COMMAND_PROMPT_READ = hlcanopen::SDOIndex(0x1023, 3);
const hlcanopen::SDOIndex CONTROL_WORD = hlcanopen::SDOIndex(0x6040, 0);
const hlcanopen::SDOIndex STATUS_WORD = hlcanopen::SDOIndex(0x6041, 0);
const hlcanopen::SDOIndex QUICK_STOP_OPTION_CODE = hlcanopen::SDOIndex(0x605A, 0);
const hlcanopen::SDOIndex SHUT_DOWN_OPTION_CODE = hlcanopen::SDOIndex(0x605B, 0);
const hlcanopen::SDOIndex MODE_OF_OPERATION = hlcanopen::SDOIndex(0x6060, 0);
const hlcanopen::SDOIndex MODE_OF_OPERATION_DISPLAY = hlcanopen::SDOIndex(0x6061, 0);
const hlcanopen::SDOIndex VELOCITY_SENSOR_ACTUAL_VALUE = hlcanopen::SDOIndex(0x6069, 0);
const hlcanopen::SDOIndex VELOCITY_ACTUAL_VALUE = hlcanopen::SDOIndex(0x606C, 0);
const unsigned int VELOCITY_ACTUAL_COD_ID = 0x180;
const hlcanopen::SDOIndex TARGET_VELOCITY = hlcanopen::SDOIndex(0x60FF, 0);
const unsigned int TARGET_VELOCITY_COB_ID = 0x200;
// hlcanopen::SDOIndex = hlcanopen::SDOIndex(0x, 0);

enum  {
  // Maximum speed limit in JV
  SPEED_JV_LIMIT = 55000, // 100 RPM
  // JV / Revolution Per Second ratio.
  JV_RPS_FACTOR = 33000 // 60 RPM
};

enum class ElmoStatus {
  NOT_READY_TO_SWITCH_ON,
  SWITCH_ON_DISABLED,
  READY_TO_SWITCH_ON,
  SWITCH_ON,
  OPERATION_ENABLED,
  QUICK_STOP_ACTIVE,
  FAULT_REACTION_ACTIVE,
  FAULT,
  UNKNOWN
};

std::ostream& operator<< (std::ostream & os, ElmoStatus val);

enum class ControlWordCommand {
  SHUTDOWN,
  SWITCH_ON,
  DISABLE_VOLTAGE,
  QUICK_STOP,
  DISABLE_OPERATION,
  ENABLE_OPERATION,
  FAULT_RESET
};

std::uint16_t getControlWordCommandBits(ControlWordCommand command);

bool getControlWordCommandFromString(const std::string s, ControlWordCommand& command);

enum ModeOfOperation : int8_t {
  NO_MODE = -1,
  PROFILE_POSITION = 1,
  PROFILED_VELOCITY = 3,
  TORQUE_PROFILED = 4,
  HOMING = 6,
  INTERPOLATION_POSITION = 7
};

bool getModeOfOperationFromString(const std::string s, ModeOfOperation& mode);

std::ostream& operator<< (std::ostream & os, ModeOfOperation val);


#endif
