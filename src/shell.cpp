#include "diana_powertrain/powertrain_manager.hpp"
#include "diana_powertrain/pci7841_card.hpp"
#include "diana_powertrain/command_line.hpp"
#include "diana_powertrain/utils.hpp"
#include "diana_powertrain/consts.hpp"
#include "diana_powertrain/CppReadline/Console.hpp"

#include <team_diana_lib/logging/logging.h>
#include <team_diana_lib/strings/strings.h>


#include <boost/program_options.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/algorithm/string.hpp>

#include <iostream>
#include <string>
#include <chrono>
#include <vector>

INITIALIZE_EASYLOGGINGPP

using namespace hlcanopen;
using namespace std;
using namespace Td;

namespace cr = CppReadline;
using ret = cr::Console::ReturnCode;

bool parseCommandLine(int argc, char** argv, int& motorId) {
  using namespace boost::program_options;
  options_description desc("Options");
  desc.add_options()
    ("help,h", "Print help messages");
  prepareParseMotorId(desc);
  variables_map varsMap;

  try {
      store(parse_command_line(argc, argv, desc), varsMap);

      if(varsMap.count("help")) {
        cout << desc << endl;
        return false;
      }

      if(!parseMotorId(varsMap, motorId)) {
        return false;
      }

  } catch (error& e) {
      ros_error(toString("Unable to parse description: ",  e.what()));
      return false;
  }

  return true;
}


unsigned help(const std::vector<std::string> &) {
    std::cout << "Press q then enter to exit.\n"
              << "Available commands are listed in the SimplIQ command reference manual.\n"
              << "Example: Commands for run a wheel\n"
              << "\tMO=1\n\tJV=10000\n\tBG\n\tST\n";
    return ret::Ok;
}

int main(int argc, char** argv) {
  Pci7841Card card(0, 0);
  hlcanopen::CanOpenManager<Pci7841Card> canOpenManager(card, std::chrono::milliseconds(150));
  canOpenManager.setupLogging();

  if(!card.open()) {
    ros_error("Unable to open p7841 card");
    return -1;
  } else {
    ros_info("card opened");
  }

  int motorId = 0;
  if(!parseCommandLine(argc, argv, motorId)) {
    return -1;
  }

  canOpenManager.initNode(motorId, hlcanopen::NodeManagerType::CLIENT);

  ros_info("starting manager thread");
  auto managerThread = thread([&](){
    canOpenManager.run();
  });
  ros_info("manager thread started");

  canOpenManager.startRemoteNode(motorId);
  mssleep(4000);
  auto res = canOpenManager.writeSdoRemote<uint32_t>(motorId, OS_COMMAND_MODE, 0);

  if(!res.wait().hasValue()) {
    ros_error("Unable to set command mode");
    return -1;
  }

  mssleep(1000);


  cr::Console c("--> ");
  c.registerCommand("help", help);

  auto defaultAction = [&](const std::vector<std::string>& command){
    auto res = canOpenManager.writeSdoRemote(motorId, OS_COMMAND_PROMPT_WRITE, command[0]);
    if(res.wait().hasValue()) {
      ros_info("sent new command");
    } else {
      ros_error("error while sending new command");
    }
    return 0;
  };

  // Add default commands just for easier typing
  std::vector<const char*> defaultCommands{"BG", "BG", "MO=0", "MO=1", "JV="};
  std::for_each(defaultCommands.begin(), defaultCommands.end(), [&](const char* com){
    std::string str(com);
    c.registerCommand(str, defaultAction);
    boost::algorithm::to_lower(str);
    c.registerCommand(str, defaultAction);
  });

  c.registerDefaultCommand(defaultAction);

  int retCode;
  do {
      retCode = c.readLine();
  }
  while ( retCode != ret::Quit );

  ros_info("stopping manager thread");
  canOpenManager.stop();
  managerThread.join();
  ros_info("done");
}

