// PW Edition: read native GameGuru skeletal/animation data without changing it.
#pragma once
#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <set>
#include <string>
#include <vector>
namespace PWSkeletalAudit {
struct Entry { const char* source; const char* target; };
inline bool Eq(const char* a,const char* b) {
 if (!a||!b) return false;
 while (*a&&*b) {if (std::tolower((unsigned char)*a)!=std::tolower((unsigned char)*b)) return false;++a;++b;}
 return !*a&&!*b;
}
inline bool Starts(const char* a,const char* b) {
 if (!a||!b) return false;
 while (*b) {if (!*a||std::tolower((unsigned char)*a)!=std::tolower((unsigned char)*b)) return false;++a;++b;}return true;
}
inline const char* MapMixamo(const char* name) {
 if (!Starts(name,"mixamorig")) return nullptr;
 const char* tail=name+9;
 while (*tail==':'||*tail=='_') ++tail;
 static const Entry map[]={
 {"Hips","Bip01_Pelvis"},{"Spine","Bip01_Spine"},{"Spine1","Bip01_Spine1"},
 {"Spine2","Bip01_Spine2"},{"Neck","Bip01_Neck"},{"Head","Bip01_Head"},
 {"LeftShoulder","Bip01_L_Clavicle"},{"RightShoulder","Bip01_R_Clavicle"},
 {"LeftArm","Bip01_L_UpperArm"},{"RightArm","Bip01_R_UpperArm"},
 {"LeftForeArm","Bip01_L_Forearm"},{"RightForeArm","Bip01_R_Forearm"},
 {"LeftHand","Bip01_L_Hand"},{"RightHand","Bip01_R_Hand"},
 {"LeftUpLeg","Bip01_L_Thigh"},{"RightUpLeg","Bip01_R_Thigh"},
 {"LeftLeg","Bip01_L_Calf"},{"RightLeg","Bip01_R_Calf"},
 {"LeftFoot","Bip01_L_Foot"},{"RightFoot","Bip01_R_Foot"},
 {"LeftToeBase","Bip01_L_Toe0"},{"RightToeBase","Bip01_R_Toe0"}
 };
 for (std::size_t i=0;i<sizeof(map)/sizeof(map[0]);++i)
  if (Eq(tail,map[i].source)) return map[i].target;
 return nullptr;
}
inline void Unique(std::vector<std::string>& v,const char* s) {
 if (!s||!*s) return;
 for (std::size_t i=0;i<v.size();++i) if (Eq(s,v[i].c_str())) return;
 v.push_back(s);
}
struct Snapshot {
 int meshes=0,skinnedMeshes=0,boneRefs=0,frames=0,animSets=0,animTracks=0,mappedMixamo=0;
 unsigned long long animKeys=0;
 bool hasMixamo=false,hasBip01=false;
 std::vector<std::string> boneNames,frameNames,unmapped,collisions;
 bool HasSkinOrAnimation() const {return skinnedMeshes>0||animTracks>0;}
};
template<class T> inline Snapshot Inspect(const T* o) {
 Snapshot r;if (!o) return r;
 r.meshes=o->iMeshCount;r.frames=o->iFrameCount;
 for (int i=0;i<o->iFrameCount;++i) {
  const auto* f=o->ppFrameList?o->ppFrameList[i]:nullptr;
  if (f) Unique(r.frameNames,f->szName);
 }
 for (int i=0;i<o->iMeshCount;++i) {
  const auto* m=o->ppMeshList?o->ppMeshList[i]:nullptr;
  if (!m||!m->pBones||m->dwBoneCount==0) continue;
  ++r.skinnedMeshes;r.boneRefs+=(int)m->dwBoneCount;
  for (unsigned int j=0;j<m->dwBoneCount;++j) Unique(r.boneNames,m->pBones[j].szName);
 }
 for (const auto* s=o->pAnimationSet;s;s=s->pNext) {
  ++r.animSets;
  for (const auto* a=s->pAnimation;a;a=a->pNext) {
   ++r.animTracks;
   r.animKeys+=(unsigned long long)a->dwNumPositionKeys;
   r.animKeys+=(unsigned long long)a->dwNumRotateKeys;
   r.animKeys+=(unsigned long long)a->dwNumScaleKeys;
   r.animKeys+=(unsigned long long)a->dwNumMatrixKeys;
  }
 }
 std::vector<std::string> all=r.frameNames;
 for (std::size_t i=0;i<r.boneNames.size();++i) Unique(all,r.boneNames[i].c_str());
 std::set<std::string> targets;
 for (std::size_t i=0;i<all.size();++i) {
  const char* name=all[i].c_str();
  if (Starts(name,"Bip01")) r.hasBip01=true;
  if (!Starts(name,"mixamorig")) continue;
  r.hasMixamo=true;
  const char* mapped=MapMixamo(name);
  if (!mapped) {r.unmapped.push_back(name);continue;}
  ++r.mappedMixamo;
  std::string lower=mapped;
  for (std::size_t k=0;k<lower.size();++k) lower[k]=(char)std::tolower((unsigned char)lower[k]);
  if (!targets.insert(lower).second) Unique(r.collisions,mapped);
 }
 return r;
}
inline bool Contains(const std::vector<std::string>& names,const std::string& name) {
 for (std::size_t i=0;i<names.size();++i) if (Eq(name.c_str(),names[i].c_str())) return true;
 return false;
}
inline std::string Compare(const Snapshot& a,const Snapshot& b) {
 if (a.skinnedMeshes>b.skinnedMeshes) return "Saved DBO lost skinned meshes";
 if (a.boneRefs>b.boneRefs) return "Saved DBO lost skin bone references";
 if (a.animTracks>b.animTracks) return "Saved DBO lost animation tracks";
 if (a.animKeys>b.animKeys) return "Saved DBO lost animation keys";
 for (std::size_t i=0;i<a.boneNames.size();++i)
  if (!Contains(b.boneNames,a.boneNames[i])) return std::string("Saved DBO lost bone: ")+a.boneNames[i];
 return std::string();
}
inline bool VerifyEnabled() {
 const char* v=std::getenv("GGCPW_VERIFY_SKELETON_SAVE");
 return v&&v[0]=='1'&&v[1]==0;
}
}
