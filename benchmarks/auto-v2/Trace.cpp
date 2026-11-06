#include "AutoReservationV2.hpp"
#include "../reservation/Data.hpp"
#include <iostream>
#include <locale>
int main(int argc,char** argv){
    try {
        if(argc<2)throw std::invalid_argument("Usage: AutoV2Trace INPUT...");
        std::cout.imbue(std::locale::classic());
        std::cout<<std::setprecision(12)<<"dataset,stage,event,reason,state,position,tokens,old_capacity,request,actual_capacity,lower,relative_upper,byte_upper,effective_upper,budget_bytes,token_size,spare_bytes,overlap_bytes,tail_bound,floor_override,byte_limited,tail_applied,moved_tokens,stable_streak,global_density,recent_density,prediction,status,error_fingerprint\n";
        for(int i=1;i<argc;++i){
            const std::filesystem::path path(argv[i]);const auto source=MatrixData::read(path);
            for(bool lex:{false,true}){
                AutoReserveV2::Journal journal;std::string status="accepted";MatrixData::Fingerprint error;
                {AutoReserveV2::JournalScope scope(journal);try{
                    if(lex){const auto tokens=MatrixData::tokenize(source);(void)tokens;}
                    else{const auto value=MatrixData::parse(source);(void)value;}
                }catch(const std::exception& ex){status="rejected";error.text(ex.what());}}
                if(journal.dropped || !journal.size)throw std::runtime_error("V2 journal incomplete");
                for(size_t index=0;index<journal.size;++index){
                    const auto& d=journal.entries[index];const auto& h=d.hint;
                    const auto maximum=(std::numeric_limits<size_t>::max)();
                    const bool growth=d.kind!=AutoReserveV2::Kind::Initial && d.kind!=AutoReserveV2::Kind::Finished && d.kind!=AutoReserveV2::Kind::Aborted;
                    const size_t needed=growth ? AutoReserve::add(d.tokens,1,maximum) : d.tokens;
                    const size_t spare=h.request>needed ? h.request-needed : 0;
                    const size_t overlap=growth || d.kind==AutoReserveV2::Kind::Initial
                        ? AutoReserve::multiply(AutoReserve::add(d.oldCapacity,h.request,maximum),sizeof(Token),maximum) : 0;
                    std::cout<<MatrixData::csv(path.filename().string())<<','<<(lex ? "lex_only" : "parse")<<','<<index<<','<<AutoReserveV2::name(d.kind,d.state)<<','<<AutoReserve::name(d.state)<<','
                        <<d.position<<','<<d.tokens<<','<<d.oldCapacity<<','<<h.request<<','<<d.actualCapacity<<','<<h.lower<<','<<h.relativeUpper<<','<<h.byteUpper<<','<<h.effectiveUpper<<','
                        <<h.budgetBytes<<','<<sizeof(Token)<<','<<AutoReserve::multiply(spare,sizeof(Token),maximum)<<','<<overlap<<','<<h.tailBound<<','<<h.floorOverride<<','<<h.byteLimited<<','<<h.tailApplied<<','
                        <<d.moved<<','<<d.streak<<','<<d.globalDensity<<','<<d.recentDensity<<','<<d.prediction<<','<<status<<','<<error.hex()<<'\n';
                }
            }
        }
    }catch(const std::exception& ex){std::cerr<<ex.what()<<'\n';return 1;}
}
