#pragma once
#include <cstdint>
#include <string>
#include <array>
#include <cmath>
#include <algorithm>
namespace PWTextureBenchmark {
// Broader admission deliberately requires an owner-approved directory,
// an additional opt-in and conventional diffuse naming. Never infer material
// semantics from a filename throughout the entire asset bank.
inline bool ApprovedDiffusePath(std::string path,bool broader){
    if(!broader)return false;
    for(char& c:path){
        if(c=='/')c='\\';
        else if(c>='A'&&c<='Z')c=static_cast<char>(c-'A'+'a');
    }
    const std::string root="\\files\\entitybank\\user\\pwstreamingapproved\\";
    const auto at=path.find(root);
    if(at==std::string::npos||at<2||path.find(root,at+1)!=std::string::npos)return false;
    if(path[1]!=':'||path[2]!='\\'||path.find(':',2)!=std::string::npos||
        path.find("\\..\\")!=std::string::npos||path.find("\\.\\")!=std::string::npos)return false;
    const auto tail=path.substr(at+root.size());
    const auto slash=tail.find_last_of('\\');
    const auto name=tail.substr(slash==std::string::npos?0:slash+1);
    if(name.size()<=6||name.compare(name.size()-6,6,"_d.dds")!=0)return false;
    // Reject known special-purpose names anywhere in the approved subtree.
    const char* blocked[]={"terrain","grass","sky","lightmap","decal","particle","water","hud","sprite","mask","normal","alpha","opacity","thumbnail","preview","effect","foliage","vegetation","character","weapon"};
    for(auto token:blocked)if(tail.find(token)!=std::string::npos)return false;
    return true;
}
constexpr std::array<unsigned,11> Edges={{5,8,10,12,16,20,25,33,50,100,250}};
struct Window {
    std::array<std::uint64_t,12> bins{};
    std::uint64_t count=0;
    double sum=0,max=0;
    void Add(double ms){
        if(!std::isfinite(ms)||ms<0)return;
        unsigned i=0;
        while(i<Edges.size()&&ms>Edges[i])++i;
        ++bins[i];++count;sum+=ms;max=(std::max)(max,ms);
    }
};
}
