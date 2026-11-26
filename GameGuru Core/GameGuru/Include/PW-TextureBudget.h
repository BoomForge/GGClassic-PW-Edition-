#pragma once
#include <cstdint>
#include <vector>
#include <algorithm>
#include <limits>
namespace PWTextureBudget {
using Bytes = std::uint64_t;
constexpr Bytes MiB = 1024ull * 1024ull;
inline Bytes Add(Bytes a, Bytes b) {
    const Bytes maxValue=(std::numeric_limits<Bytes>::max)();
    return b>maxValue-a?maxValue:a+b;
}
inline Bytes Excess(Bytes a, Bytes b){return a>b?a-b:0;}
inline bool ParseMiB(const char* s, Bytes& bytes){
    bytes=0;
    if(!s||!*s)return true;
    Bytes n=0;
    for(;*s;++s){
        if(*s<'0'||*s>'9')return false;
        n=n*10+static_cast<Bytes>(*s-'0');
        if(n>1048576ull)return false;
    }
    bytes=n*MiB;return true;
}
inline Bytes Percent(Bytes value,unsigned p){return (value/100)*p+((value%100)*p)/100;}
struct Input {
    Bytes registeredGpu=0,configured=0,localUsage=0,localBudget=0,dedicated=0;
    bool localValid=false;
};
struct Result {
    Bytes target=0,logicalExcess=0,adapterExcess=0;
    bool targetKnown=false,adapterPressure=false;
};
inline Result Evaluate(const Input& in){
    Result out;
    const Bytes capacity=in.localValid&&in.localBudget?in.localBudget:in.dedicated;
    const Bytes automatic=Percent(capacity,70);
    out.target=in.configured?in.configured:automatic;
    if(automatic&&out.target>automatic)out.target=automatic;
    out.targetKnown=out.target!=0;
    if(out.targetKnown)out.logicalExcess=Excess(in.registeredGpu,out.target);
    if(in.localValid&&in.localBudget){
        out.adapterExcess=Excess(in.localUsage,Percent(in.localBudget,90));
        out.adapterPressure=out.adapterExcess!=0;
    }
    return out;
}
enum class Priority { Background,World,Important,Essential };
struct Candidate {
    int id=0;
    std::uint64_t generation=0,lastUsed=0;
    Bytes reclaimable=0;
    Priority priority=Priority::Essential;
    bool pinned=true,exclusiveOwnership=false,reloadable=false;
};
struct Plan { std::vector<Candidate> selected; Bytes planned=0,remaining=0; };
// Advisory only: unique allocation records; revalidate generation and bindings before any future swap.
inline Plan PlanEviction(std::vector<Candidate> candidates,Bytes deficit){
    Plan out;out.remaining=deficit;
    if(!deficit)return out;
    std::sort(candidates.begin(),candidates.end(),[](const Candidate& a,const Candidate& b){return a.id<b.id;});
    std::vector<Candidate> eligible;
    for(std::size_t i=0;i<candidates.size();){
        std::size_t end=i+1;
        while(end<candidates.size()&&candidates[end].id==candidates[i].id)++end;
        const auto& c=candidates[i];
        if(end==i+1&&c.id!=0&&c.generation&&c.reclaimable&&!c.pinned&&c.exclusiveOwnership&&c.reloadable&&c.priority!=Priority::Essential)eligible.push_back(c);
        i=end;
    }
    std::sort(eligible.begin(),eligible.end(),[](const Candidate& a,const Candidate& b){
        if(a.priority!=b.priority)return a.priority<b.priority;
        if(a.lastUsed!=b.lastUsed)return a.lastUsed<b.lastUsed;
        return a.id<b.id;
    });
    for(const auto& c:eligible){
        out.selected.push_back(c);out.planned=Add(out.planned,c.reclaimable);
        out.remaining=Excess(deficit,out.planned);
        if(!out.remaining)break;
    }
    return out;
}
}
