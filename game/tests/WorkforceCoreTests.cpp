#include "hh/game/Workforce.h"
#include <iostream>
#include <stdexcept>
using namespace hh::game;
static void require(bool v,const char *m){if(!v)throw std::runtime_error(m);}
int main(){try{
  const auto a=Workforce::applicantPool(0x12345678ULL,7);
  const auto b=Workforce::applicantPool(0x12345678ULL,7);
  require(a==b,"applicant pool is not deterministic");
  require(a.size()==6,"applicant pool size changed");
  require(Workforce::applicantPool(1,-1).empty(),"negative day should have no applicants");
  for(const auto &x:a){require(x.id!=0,"applicant id must be non-zero");require(x.skill>=45&&x.skill<=90,"skill outside contract");require(x.reliability>=70&&x.reliability<=100,"reliability outside contract");require(x.wageExpectationCents>0,"wage must be positive");}
  require(!Workforce::absentForShift(99,123,456,100.0),"100% reliable employee was absent");
  require(Workforce::absentForShift(99,123,456,0.0),"0% reliable employee was present");
  require(Workforce::absentForShift(99,123,456,85.0)==Workforce::absentForShift(99,123,456,85.0),"absence decision is not deterministic");
}catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}std::cout<<"Workforce core tests passed\n";return 0;}
