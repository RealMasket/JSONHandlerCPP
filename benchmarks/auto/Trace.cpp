#include "AutoReservation.hpp"
#include "../reservation/Data.hpp"
#include <iostream>
#include <locale>
int main(int argc,char** argv) {
    try {
        if (argc<2) throw std::invalid_argument("Usage: AutoTrace INPUT...");
        std::cout.imbue(std::locale::classic());
        std::cout<<std::setprecision(12)<<"dataset,stage,event,reason,position,tokens,old_capacity,request,actual_capacity,lower,upper,moved_tokens,stable_streak,global_density,recent_density,prediction,clamped,floored,status,error_fingerprint\n";
        for (int i=1;i<argc;++i) {
            const std::filesystem::path path(argv[i]);
            const auto source=MatrixData::read(path);
            for (bool lex : {false,true}) {
                AutoReserve::Journal journal;
                std::string status="accepted";
                MatrixData::Fingerprint error;
                {
                    AutoReserve::JournalScope scope(journal);
                    try {
                        if (lex) { const auto tokens=MatrixData::tokenize(source); (void)tokens; }
                        else { const auto value=MatrixData::parse(source); (void)value; }
                    } catch (const std::exception& ex) { status="rejected"; error.text(ex.what()); }
                }
                if (journal.dropped || !journal.size) throw std::runtime_error("Auto journal incomplete");
                for (size_t event=0;event<journal.size;++event) {
                    const auto& d=journal.entries[event];
                    std::cout<<MatrixData::csv(path.filename().string())<<','<<(lex ? "lex_only" : "parse")<<','<<event<<','<<AutoReserve::name(d.reason)<<','
                        <<d.position<<','<<d.tokens<<','<<d.oldCapacity<<','<<d.request<<','<<d.actualCapacity<<','<<d.lower<<','<<d.upper<<','
                        <<d.moved<<','<<d.streak<<','<<d.globalDensity<<','<<d.recentDensity<<','<<d.prediction<<','<<d.clamped<<','<<d.floored<<','<<status<<','<<error.hex()<<'\n';
                }
            }
        }
    } catch (const std::exception& ex) { std::cerr<<ex.what()<<'\n'; return 1; }
}
