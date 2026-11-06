#include "AutoReservationV2.hpp"
#include "Compare.hpp"
#include "../reservation/Data.hpp"
#include <iostream>
namespace {
void require(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
void arithmetic() {
    using namespace AutoReserveV2;
    const auto maximum=(std::numeric_limits<size_t>::max)();
    for(auto goal:{Goal::Speed,Goal::Memory})for(size_t limit:{size_t{1},size_t{1000},maximum/sizeof(Token),maximum})
        for(size_t cap:{size_t{0},limit/2,limit-1})for(size_t budget:{size_t{0},size_t{1},size_t{1048576},maximum})
            for(bool mature:{false,true})for(bool pressure:{false,true})
                for(double prediction:{0.0,100.0,static_cast<double>(limit),(std::numeric_limits<double>::infinity)()}) {
                    const auto h=choose(goal,cap,limit,mature,pressure,100,false,prediction,budget);
                    require(h.request>cap && h.request<=limit,"V2 progress/overflow failure");
                    require(h.lower<=h.effectiveUpper && h.effectiveUpper<=limit,"V2 invalid window");
                    if(!h.floorOverride)require(h.request<=h.byteUpper,"V2 byte ceiling exceeded silently");
                    else require(h.lower>h.byteUpper && h.request<=h.lower,"V2 floor override invalid");
                    if(h.tailApplied)require(h.request==h.tailBound,"V2 remaining-byte bound mismatch");
                    else require(h.request>=h.lower,"V2 lost geometric progress");
                }
    const auto tail=remainingBound(65535,2,false,maximum);
    require(tail.fits && tail.count==65538,"Dense near-end bound incorrect");
    require(!remainingBound(maximum,1,false,maximum).fits,"Tail bound overflow");
    const auto h=choose(Goal::Memory,65534,maximum,true,false,2,false,65538,524288);
    require(h.tailApplied && h.request==65538,"Near-end growth must fit all remaining tokens");
}
}
int main(int argc,char** argv){
    try {
        arithmetic();AutoCheck::selfTest();
        if(argc==2 && std::string_view(argv[1])=="--self-test"){std::cout<<"V2 budget/tail arithmetic passed\n";return 0;}
        if(argc<2)throw std::invalid_argument("Usage: AutoV2Verify INPUT... | --self-test");
        for(int i=1;i<argc;++i)require(AutoCheck::compare(MatrixData::read(argv[i])),"Expected valid V2 fixture");
        std::cout<<"V2 exact token/value/diagnostic comparison passed\n";
    }catch(const std::exception& ex){std::cerr<<ex.what()<<'\n';return 1;}
}
