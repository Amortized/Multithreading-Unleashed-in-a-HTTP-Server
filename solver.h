#ifndef solver_h
#define solver_h

#include "formula_impl.h"
#include <vector>

namespace Sat {

using namespace std;

class Solver : {
  stack<int> propositionalVariables;
  public:
  Solver();
  ~Solver();

  bool solve(FormulaImpl);
  
  

};

}  // end of namespace 

#endif

