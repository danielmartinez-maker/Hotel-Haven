#include "hh/game/StaffOptimization.h"
#include <iostream>
#include <stdexcept>
using namespace hh::game;
static void require(bool v,const char *m){if(!v)throw std::runtime_error(m);}
static OptimizerSnapshot makeSnapshot(){OptimizerSnapshot s;s.capturedSecond=100;s.horizonEndSecond=1000;s.employees={{1,StaffRole::Housekeeper,true,false,{{100,500}},{}},{2,StaffRole::Housekeeper,false,true,{{100,500}},{{200,260}}},{3,StaffRole::Maintenance,false,true,{{100,500}},{}}};s.tasks={{11,StaffRole::Housekeeper,true,100,60},{12,StaffRole::Housekeeper,false,100,45}};return s;}
int main(){try{
  const auto s=makeSnapshot();AssignmentPlan p;p.assignments={{11,1,100,160}};require(!validateAssignmentPlan(s,p),"accepted absent employee");
  p.assignments={{11,3,100,160}};require(!validateAssignmentPlan(s,p),"accepted wrong role");
  p.assignments={{11,2,210,270}};require(!validateAssignmentPlan(s,p),"accepted unavailable overlap");
  p.assignments={{11,2,100,160}};require(validateAssignmentPlan(s,p).ok,"rejected valid assignment");
  auto f=s;f.employees[0].absent=false;f.employees[0].availableNow=true;
  const auto a=buildDeterministicFallbackPlan(f),b=buildDeterministicFallbackPlan(f);
  require(a==b,"fallback scheduler is not deterministic");require(a.assignments.size()==2,"did not assign feasible tasks");require(a.assignments.front().taskId==11,"critical task not prioritized");require(validateAssignmentPlan(f,a).ok,"fallback plan invalid");
}catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}std::cout<<"Staff optimization core tests passed\n";return 0;}
