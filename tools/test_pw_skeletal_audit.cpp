#include "PW-SkeletalAudit.h"
#include <cassert>
#include <iostream>
struct Bone{const char* szName;};
struct Mesh{unsigned int dwBoneCount;Bone* pBones;};
struct Frame{const char* szName;};
struct Anim{unsigned int dwNumPositionKeys,dwNumRotateKeys,dwNumScaleKeys,dwNumMatrixKeys;Anim* pNext;};
struct Set{Anim* pAnimation;Set* pNext;};
struct Obj{int iMeshCount,iFrameCount;Mesh** ppMeshList;Frame** ppFrameList;Set* pAnimationSet;};
int main(){
 assert(PWSkeletalAudit::Eq(PWSkeletalAudit::MapMixamo("mixamorig:Hips"),"Bip01_Pelvis"));
 assert(PWSkeletalAudit::Eq(PWSkeletalAudit::MapMixamo("mixamorig_LeftUpLeg"),"Bip01_L_Thigh"));
 assert(PWSkeletalAudit::MapMixamo("mixamorigMystery")==nullptr);
 Bone bones[]={{"mixamorig:Hips"},{"mixamorig:Spine"},{"mixamorig:Mystery"}};
 Mesh mesh={3,bones};Mesh* meshes[]={&mesh};
 Frame fs[]={{"mixamorig:Hips"},{"mixamorig:Spine"},{"mixamorig:Mystery"}};
 Frame* frames[]={&fs[0],&fs[1],&fs[2]};
 Anim a={2,4,0,0,nullptr};Set set={&a,nullptr};
 Obj o={1,3,meshes,frames,&set};
 auto snapshot=PWSkeletalAudit::Inspect(&o);
 assert(snapshot.skinnedMeshes==1&&snapshot.boneRefs==3);
 assert(snapshot.animSets==1&&snapshot.animTracks==1&&snapshot.animKeys==6);
 assert(snapshot.hasMixamo&&snapshot.mappedMixamo==2&&snapshot.unmapped.size()==1);
 assert(PWSkeletalAudit::Compare(snapshot,snapshot).empty());
 Obj lost={1,3,meshes,frames,nullptr};
 assert(!PWSkeletalAudit::Compare(snapshot,PWSkeletalAudit::Inspect(&lost)).empty());
 Mesh empty={0,nullptr};Mesh* stripped[]={&empty};Obj lostWeights={1,3,stripped,frames,&set};
 assert(!PWSkeletalAudit::Compare(snapshot,PWSkeletalAudit::Inspect(&lostWeights)).empty());
 Bone duplicates[]={{"mixamorig:Hips"},{"mixamorig_Hips"}};
 Mesh dupMesh={2,duplicates};Mesh* dupMeshes[]={&dupMesh};
 Obj dupObj={1,0,dupMeshes,nullptr,nullptr};
 assert(!PWSkeletalAudit::Inspect(&dupObj).collisions.empty());
 std::cout<<"PW Skeletal Audit PASS\n";
}
