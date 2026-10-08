#include <string>
#include <vector>
#include <iostream>
#include <cstdlib>
#include <stdint.h>
#include <unistd.h>

#include <parser.h>
#include <typecheck.h>
#include <codegen.h>

void print_help (char *progName){
  std::cerr << "Usage: " << progName << " [-v] [-c] SOURCE" << std::endl;
  return ;
}

int main(
  int argc, 
  char **argv
  ){
  bool check_only = false;

  if( argc < 2 ) {
    print_help(argv[0]);
    return 1;
  }
  int32_t opt;
  while ((opt = getopt(argc, argv, "vcg:O:")) != -1) {
    switch (opt){
      case 'c':
        check_only = true;
        break ;

      case 'O':
      case 'g':
      case 'v':
        break ;

      default:
        print_help(argv[0]);
        return 1;
    }
  }

  auto p = CM::parse_file(argv[optind]);

  if (!CM::check_program(p, argv[optind])) {
    return 1;
  }

  if (!check_only) {
    CM::generate_code(p);
  }

  return 0;
}
