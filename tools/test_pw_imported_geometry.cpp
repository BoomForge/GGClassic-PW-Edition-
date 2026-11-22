#include "PW-ImportedGeometry.h"
#include <cassert>
#include <iostream>
#include <limits>
struct Mesh { unsigned int dwVertexCount; };
struct Object { int iMeshCount; Mesh** ppMeshList; };
int main(){
 assert(!PWImportedGeometry::Inspect((Object*)nullptr).usable);
 Object none{0,nullptr};
 assert(!PWImportedGeometry::Inspect(&none).usable);
 Mesh zero{0};
 Mesh* onlyZero[]{&zero};
 Object empty{1,onlyZero};
 assert(!PWImportedGeometry::Inspect(&empty).usable);
 Mesh valid{3456};
 Mesh* withNull[]{nullptr,&zero,&valid};
 Object mixed{3,withNull};
 const auto report=PWImportedGeometry::Inspect(&mixed);
 assert(report.usable&&report.vertices==3456&&report.invalidMeshes==2);
 Object noBuffer{3,nullptr};
 assert(!PWImportedGeometry::Inspect(&noBuffer).usable);
 assert(PWImportedGeometry::SensibleExtent(2.0f,1.0f,0.0f));
 assert(!PWImportedGeometry::SensibleExtent(0.0f,0.0f,0.0f));
 assert(!PWImportedGeometry::SensibleExtent(
    std::numeric_limits<float>::infinity(),3,2));
 assert(!PWImportedGeometry::SensibleExtent(
    std::numeric_limits<float>::quiet_NaN(),3,2));
 std::cout<<"PW imported model geometry validation PASS\n";
}
