#include "PW-SkeletalIntegrity.h"
#include <cassert>
#include <cstring>
#include <iostream>
struct Matrix {float value[16];};
struct Vec3 {float x,y,z;};
struct Quat {float x,y,z,w;};
struct Bone {char szName[64];unsigned int dwNumInfluences;unsigned int* pVertices;float* pWeights;Matrix matTranslation;};
struct Mesh {unsigned int dwVertexCount,dwBoneCount;Bone* pBones;};
struct Frame {char szName[64];Frame* pParent;Matrix matOriginal;};
struct Position {unsigned int dwTime;Vec3 vecPos;};
struct Rotation {unsigned int dwTime;Quat Quaternion;};
struct Scale {unsigned int dwTime;Vec3 vecScale;};
struct MatrixKey {unsigned int dwTime;Matrix matMatrix;};
struct Track {
 char szName[64];int bLoop,bLinear;
 unsigned int dwNumPositionKeys,dwNumRotateKeys,dwNumScaleKeys,dwNumMatrixKeys;
 Position* pPositionKeys;Rotation* pRotateKeys;Scale* pScaleKeys;MatrixKey* pMatrixKeys;
 Track* pNext;
};
struct Clip {
 char szName[64];unsigned int ulLength;float fAnimSetStart,fAnimSetFinish;
 Track* pAnimation;Clip* pNext;
};
struct Object {int iMeshCount,iFrameCount;Mesh** ppMeshList;Frame** ppFrameList;Clip* pAnimationSet;};
static void identity(Matrix& m) {
 for (int i=0;i<16;++i)m.value[i]=i%5==0?1.0f:0.0f;
}
int main() {
 unsigned int verts[]={0,1};
 float weights[]={1.0f,1.0f};
 Bone bones[1]={};std::strcpy(bones[0].szName,"mixamorig:Hips");
 bones[0].dwNumInfluences=2;bones[0].pVertices=verts;bones[0].pWeights=weights;
 identity(bones[0].matTranslation);
 Mesh mesh={2,1,bones};Mesh* meshes[]={&mesh};
 Frame frame={};std::strcpy(frame.szName,"mixamorig:Hips");identity(frame.matOriginal);
 Frame* frames[]={&frame};
 Position positions[]={{0,{0,0,0}},{30,{0,1,0}}};
 Rotation rot[]={{0,{0,0,0,1}}};
 Track track={};std::strcpy(track.szName,"mixamorig:Hips");track.bLoop=1;
 track.dwNumPositionKeys=2;track.dwNumRotateKeys=1;
 track.pPositionKeys=positions;track.pRotateKeys=rot;
 Clip clip={};std::strcpy(clip.szName,"Walk");clip.ulLength=30;
 clip.fAnimSetStart=0;clip.fAnimSetFinish=30;clip.pAnimation=&track;
 Object object={1,1,meshes,frames,&clip};

 auto before=PWSkeletalIntegrity::Inspect(&object);
 assert(before.influences==2);
 assert(!before.HasProblems());
 assert(before.clips.size()==1);
 assert(before.clips[0].name=="Walk"&&before.clips[0].tracks==1);
 assert(before.clips[0].keys==3&&before.clips[0].duration==30);
 assert(PWSkeletalIntegrity::Compare(before,PWSkeletalIntegrity::Inspect(&object)).empty());

 weights[0]=0.65f;
 auto changedWeight=PWSkeletalIntegrity::Inspect(&object);
 assert(changedWeight.unnormalizedVertices==1);
 assert(PWSkeletalIntegrity::Compare(before,changedWeight).find("weights")!=std::string::npos);
 weights[0]=1.0f;

 bones[0].matTranslation.value[12]=2.0f;
 assert(PWSkeletalIntegrity::Compare(before,PWSkeletalIntegrity::Inspect(&object)).find("Bind")!=std::string::npos);
 bones[0].matTranslation.value[12]=0;

 frame.matOriginal.value[13]=3.0f;
 assert(!PWSkeletalIntegrity::Compare(before,PWSkeletalIntegrity::Inspect(&object)).empty());
 frame.matOriginal.value[13]=0;

 positions[1].vecPos.y=2;
 assert(PWSkeletalIntegrity::Compare(before,PWSkeletalIntegrity::Inspect(&object)).find("Animation")!=std::string::npos);
 positions[1].vecPos.y=1;

 rot[0].Quaternion.w=0.5f;
 assert(!PWSkeletalIntegrity::Compare(before,PWSkeletalIntegrity::Inspect(&object)).empty());
 rot[0].Quaternion.w=1;

 clip.ulLength=25;
 assert(!PWSkeletalIntegrity::Compare(before,PWSkeletalIntegrity::Inspect(&object)).empty());
 clip.ulLength=30;

 verts[1]=9;
 auto invalidIndex=PWSkeletalIntegrity::Inspect(&object);
 assert(invalidIndex.invalidVertexIndices==1);
 assert(!PWSkeletalIntegrity::Compare(before,invalidIndex).empty());
 verts[1]=1;

 track.pRotateKeys=nullptr;
 auto missing=PWSkeletalIntegrity::Inspect(&object);
 assert(missing.missingBuffers==1);
 assert(!PWSkeletalIntegrity::Compare(before,missing).empty());
 track.pRotateKeys=rot;

 assert(PWSkeletalIntegrity::Compare(before,PWSkeletalIntegrity::Inspect(&object)).empty());
 std::cout<<"PW native skeleton bind/weights/animation integrity: PASS\n";
}
