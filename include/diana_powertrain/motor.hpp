#ifndef MOTOR_HPP
#define MOTOR_HPP

#include <string>

#include "diana_powertrain/consts.hpp"
#include "diana_powertrain/elmo_structs.hpp"

#include <hlcanopen/can_open_manager.hpp>
#include <hlcanopen/types.hpp>

#include <team_diana_lib/logging/logging.h>
#include <team_diana_lib/strings/strings.h>
#include <team_diana_lib/enum/enum.h>

#include <folly/futures/Future.h>

#include "utils.hpp"

#include <functional>

template <class T> class Motor {

public:
  Motor(hlcanopen::CanOpenManager<T>& canOpenManager, hlcanopen::NodeId id) :
    manager(canOpenManager),
    nodeId(id) {

  }

  Motor(const Motor<T>& oth) :
    manager(oth.manager),
    nodeId(oth.nodeId) {
  }

  ~Motor() {}

  folly::Future<folly::Unit> enable() {
    return send_msg_async("MO=1");
  }
  folly::Future<folly::Unit> disable() {
    return send_msg_async("MO=0");
  }

  folly::Future<folly::Unit> start() {
    return send_msg_async("BG");
  }

  folly::Future<folly::Unit> stop() {
    return send_msg_async("ST");
  }

  folly::Future<folly::Unit> setControlWord(ControlWordCommand command) {
    uint32_t controlWord = 0;
    controlWord |= getControlWordCommandBits(command);
    return manager.template writeSdoRemote(nodeId, CONTROL_WORD, controlWord, 2000);
  }

  folly::Future<StatusWord> getStatusWord() {
    Td::ros_info("requestig status word");
    folly::Future<uint32_t> res = manager.template readSdoRemote<uint32_t>(nodeId, STATUS_WORD, 1000);
    return res.then([](uint32_t statusWordValue) {
        StatusWord statusWord(statusWordValue);
        return statusWord;
    });
  }

  folly::Future<folly::Unit> setOperationMode(ModeOfOperation mode) {
    uint32_t value = Td::to_int(mode);
    return manager.template writeSdoRemote(nodeId, MODE_OF_OPERATION, value, 1000);
  }

  folly::Future<ModeOfOperation> getOperationMode() {
    Td::ros_info("requestig mode of operation");
    folly::Future<uint32_t> res = manager.template readSdoRemote<uint32_t>(nodeId, MODE_OF_OPERATION_DISPLAY, 1000);
    return res.then([](uint32_t modeOfOperationValue) {
        ModeOfOperation modeOfOperation = (ModeOfOperation) modeOfOperationValue;
        return modeOfOperation;
    });
  }

//   std::future<MotorAsyncResult> setCommandMode() {
  bool setCommandMode() {
    manager.startRemoteNode(nodeId);
    mssleep(4000);
    return manager.template writeSdoRemote<uint32_t>(nodeId, OS_COMMAND_MODE, 0).wait().hasValue();
  }

  void send_msg_sync(const std::string& msg, const std::string& desc) {
    Td::ros_info(Td::toString("Sending msg to shell ", nodeId, ": ", msg));
    auto response = manager.writeSdoRemote(nodeId, writeIndex, msg);
    if(response.get().get() == false) {
      Td::ros_info(desc +" failed");
    }
  }

  folly::Future<folly::Unit> send_msg_async(const std::string& msg) {
    Td::ros_info(Td::toString("Sending msg to shell ", nodeId, ": ", msg));
    return manager.writeSdoRemote(nodeId, OS_COMMAND_PROMPT_WRITE, msg, 1500);
  }

  folly::Future<folly::Unit> setVelocity(float MetersPerSecond) {
//     return setJVVelocity(MetersPerSecond*MPS_JV_FACTOR);

    // TODO: this is still in testing:
    return setVelocitySdoOnly(MetersPerSecond*MPS_JV_FACTOR);
  }

  folly::Future<folly::Unit> setVelocitySdoOnly(int value) {
      return manager.template writeSdoRemote(nodeId, TARGET_VELOCITY, value, 1500);
  }

  void setVelocityPdo(float MetersPerSecond) {
    manager.template writeSdoLocal(nodeId, TARGET_VELOCITY, MetersPerSecond*MPS_JV_FACTOR);
    manager.writeRPDO(nodeId, TARGET_VELOCITY_COB_ID);
  }

  folly::Future<float> getVelocity() {
    auto res = manager.template readSdoRemote<int32_t>(nodeId, VELOCITY_ACTUAL_VALUE, 1500);
    return res.then([](int32_t velValue) {
      return (float)velValue;
    });
  }

  float getVelocityPdo() {
    return manager.readSdoLocal(nodeId, VELOCITY_ACTUAL_VALUE);
  }

  hlcanopen::NodeId getId() const {
    return nodeId;
  }

private:

  int clampJVToSafe(int speed) {
    bool clamped = false;

    if(speed > SPEED_JV_LIMIT) {
      return SPEED_JV_LIMIT;
      clamped = true;
    } else if(speed < -SPEED_JV_LIMIT) {
      return -SPEED_JV_LIMIT;
      clamped = true;
    }

    if(clamped) {
      Td::ros_warn(Td::toString("The requested speed was clamped to: ", speed ,"JV in order to maintain safety margins"));
    }

    return speed;
  }

  folly::Future<folly::Unit> setJVVelocity(int velocity) {
    velocity = clampJVToSafe(velocity);

    folly::Future<folly::Unit> res;
    res = send_msg_async(Td::toString("JV=", velocity));

    return res.then([&, velocity]() {
      if(velocity == 0) {
        return stop().get();
      } else {
        return start().get();
      }
    });
  }

  int getJVVelocity() {
    auto response = manager.writeSdoRemote(nodeId, writeIndex, "JV");
    if(response.get().get() == false) {
      Td::ros_error("getSpeed() failed: unable to fetch value");
      return -1; // Return error properly
    }

    while(true) {
      auto response = manager.template readSdoRemote<uint8_t>(nodeId, OS_COMMAND_PROMPT_STATUS);
      if(response.get().get() == 0x1)
        break;
      else if (response.get().get() == 0x3) {
        Td::ros_error("getSpeed() failed: command rejected");
        return -1; // Return error properly
      }
      else if (response.get().get() != 0xFF) {
        Td::ros_error("getSpeed() failed: unexpected value in 0x1023.2");
        return -1; // Return error properly
      }
      mssleep(500); // 500 ms
    }

    auto result = manager.template readSdoRemote<std::string>(nodeId, OS_COMMAND_PROMPT_READ);
    return std::stoi(result.get().get());
  }


private:
  hlcanopen::CanOpenManager<T>& manager;
  hlcanopen::NodeId nodeId;

  static hlcanopen::SDOIndex writeIndex;
  static hlcanopen::SDOIndex statusIndex;
  static hlcanopen::SDOIndex readIndex;

};


#endif // MOTOR_HPP
