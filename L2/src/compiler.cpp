#include <string>
#include <vector>
#include <utility>
#include <algorithm>
#include <set>
#include <iterator>
#include <iostream>
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <stdint.h>
#include <unistd.h>
#include <iostream>
#include <assert.h>

#include <fstream>
#include <sstream>

#include <parser.h>
#include <liveness_analysis.h>

std::string read_file(const char *path) {
  std::ifstream in(path);
  std::stringstream buffer;
  buffer << in.rdbuf();
  return buffer.str();
}

void print_help (char *progName){
  std::cerr << "Usage: " << progName << " [-v] [-g 0|1] [-O 0|1|2] SOURCE" << std::endl;
  return ;
}

int main(
  int argc, 
  char **argv
  ){
  auto enable_code_generator = false;
  auto liveness_analysis = false; 
  int32_t optLevel = 0;
  bool verbose;

  /* 
   * Check the compiler arguments.
   */
  if( argc < 2 ) {
    print_help(argv[0]);
    return 1;
  }
  int32_t opt;
  while ((opt = getopt(argc, argv, "vlg:O:")) != -1) {
    switch (opt){
      case 'O':
        optLevel = strtoul(optarg, NULL, 0);
        break ;

      case 'g':
        enable_code_generator = (strtoul(optarg, NULL, 0) == 0) ? false : true ;
        break ;

      case 'v':
        verbose = true;
        break ;
      
      case 'l':
        liveness_analysis = true;
        break ;

      default:
        print_help(argv[0]);
        return 1;
    }
  }


  /*
   * Parse the input file.
   */
  std::string src = read_file(argv[optind]);
  src = "(@go\n" + src + ")";

  const char *tmp = "/tmp/l2_tmp.L2";
  std::ofstream out(tmp);
  out << src;
  out.close();

  auto p = L2::parse_file((char*)tmp);

  /*
   * Perform liveness analysis 
   */

  L2::analyze_liveness(p); 

  return 0;
}
