#include "hh/game/Departments.h"
#include <iostream>
#include <stdexcept>
using namespace hh::game;
static void require(bool v,const char *m){if(!v)throw std::runtime_error(m);}
int main(){try{
  require(departmentRole(DepartmentId::FrontOffice)==StaffRole::Receptionist,"front office role mismatch");
  require(departmentRole(DepartmentId::Housekeeping)==StaffRole::Housekeeper,"housekeeping role mismatch");
  require(departmentRole(DepartmentId::Engineering)==StaffRole::Maintenance,"engineering role mismatch");
  require(std::string(departmentName(DepartmentId::FrontOffice))=="Front Office","front office name mismatch");
  const auto &d=allDepartments();require(d.size()==3,"department catalog size changed");
  require(d[0]==DepartmentId::FrontOffice&&d[1]==DepartmentId::Housekeeping&&d[2]==DepartmentId::Engineering,"department ordering changed");
}catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}std::cout<<"Department core tests passed\n";return 0;}
