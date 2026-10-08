#include <string>
#include <vector>
#include <iostream>
#include <cstdlib>
#include <stdint.h>
#include <unistd.h>

#include <parser.h>
#include <codegen.h>

void print_help (char *progName){
  std::cerr << "Usage: " << progName << " [-v] [-g 0|1] [-O 0|1|2] SOURCE" << std::endl;
  return ;
}

int main(
  int argc, 
  char **argv
  ){
  int32_t optLevel = 0;
  bool verbose = false;

  if( argc < 2 ) {
    print_help(argv[0]);
    return 1;
  }
  int32_t opt;
  while ((opt = getopt(argc, argv, "vg:O:")) != -1) {
    switch (opt){
      case 'O':
        optLevel = strtoul(optarg, NULL, 0);
        break ;

      case 'g':
        break ;

      case 'v':
        verbose = true;
        break ;

      default:
        print_help(argv[0]);
        return 1;
    }
  }

  auto p = LB::parse_file(argv[optind]);

  try {
    LB::generate_code(p);
  } catch (const std::exception &e) {
    std::cerr << argv[optind] << ": " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
