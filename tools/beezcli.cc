#include "rocksdb/ldb_tool.h"
#include <iostream>
#include <getopt.h>
#include <signal.h>
#include <sstream>
#include <iterator>
#include <readline/readline.h>
#include <readline/history.h>

void SignalHandler(int sigint) {
    std::cout << std::endl << "Ciao" << std::endl;
    exit(0);
}
void ToArgv(std::string const &input, std::vector<std::string>& temp) {
    std::istringstream buffer(input);
    std::copy(std::istream_iterator<std::string>(buffer), 
              std::istream_iterator<std::string>(),
              std::back_inserter(temp));

}
int main(int argc, char** argv) {
  signal(SIGINT, &SignalHandler);
  ROCKSDB_NAMESPACE::LDBTool tool;
  std::string prompt = "beezcli> ";
  const char* const short_opts = "di\0";
  const option long_opts[] = {
          {"db", required_argument, 0 ,'d'},
          {"interactive", no_argument, nullptr, 'i'},
          {0, 0, 0, 0}
  };
  int opt;
  std::string dbpath = "";
  bool i = false;
  bool d = false;
  opterr = 0;
  opt = getopt_long(argc, argv, short_opts, long_opts, nullptr);
  while (opt != -1) {
    switch (opt) {
      case 'd':
          dbpath = std::string(optarg);
          std::cout << dbpath << std::endl;
          d = true;
          break;
      case 'i':
          i = true;
          break;
    }
    opt = getopt_long(argc, argv, short_opts, long_opts, nullptr);
  }
  char* line;
  if (i && !d) {
    std::cerr << "interactive flag provided without --db" << std::endl;
    return EINVAL;
  }
  while (i && d && (line=readline(prompt.c_str())) && line) {
        if (line[0] != '\0')
            add_history(line);
        std::string input(line);
        free(line);
        line = nullptr;
        if(input == "help") {
            char** help = new char*[2];
            help[0] = argv[0];
            help[1] = const_cast<char*>("--help");
            tool.Run(2, help);
            continue;
        }
        if (input == "quit" || input == "exit") {
            SignalHandler(0);
        }
        if (!input.empty()) {
            std::vector<std::string> vec;
            ToArgv(std::string(argv[0]) + " " + input + " --db=" + dbpath,vec);
            std::vector<char*> cstrings{};
            for(const auto& string : vec) {
                cstrings.push_back(const_cast<char *>(string.c_str()));
            }
            tool.Run(cstrings.size(), cstrings.data());
        }
  }
  if (line == NULL && i && d) {
    SignalHandler(0);
  }
  tool.Run(argc, argv);
  return 0;
}
