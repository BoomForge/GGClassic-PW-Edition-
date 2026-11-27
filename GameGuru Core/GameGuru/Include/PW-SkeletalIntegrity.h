// PW Edition: deep, read-only audit of native GameGuru DBO skeleton/animation data.
// This is a verifier, not a converter. It never edits skinned meshes or clips.
#pragma once
#include "PW-SkeletalAudit.h"
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

namespace PWSkeletalIntegrity {
struct Fingerprint {
    std::uint64_t state=14695981039346656037ULL;
    void Byte(unsigned char b) { state^=b;state*=1099511628211ULL; }
    void Integer(std::uint64_t n) {
        for (int i=0;i<8;++i) {Byte((unsigned char)(n&255));n>>=8;}
    }
    void Text(const char* s) {
        if (!s) {Integer(0);return;}
        for (;*s;++s) Byte((unsigned char)*s);
        Byte(0);
    }
    void Real(float n) {
        if (!std::isfinite(n)) {Integer(0x7fffffffffffffffULL);return;}
        const double scaled=(double)n*10000.0;
        if (scaled>9.0e18 || scaled< -9.0e18) {Integer(0x7ffffffffffffffeULL);return;}
        Integer((std::uint64_t)(std::int64_t)std::llround(scaled));
    }
    template<class T> void Floats(const T& value,std::size_t count) {
        // The engine uses fixed-size float matrices/vectors and quaternions;
        // copy their bytes without aliasing the underlying SDK type.
        const std::size_t safe=count*sizeof(float)<=sizeof(T)?count:sizeof(T)/sizeof(float);
        unsigned char raw[64]={0};
        if (safe>16) {Integer(0xBAD5);return;}
        std::memcpy(raw,&value,safe*sizeof(float));
        for (std::size_t i=0;i<safe;++i) {
            float f=0;std::memcpy(&f,raw+i*sizeof(float),sizeof(float));
            Real(f);
        }
        Integer((std::uint64_t)safe);
    }
};
struct Clip {
    std::string name;
    unsigned long long duration=0,keys=0;
    unsigned int tracks=0;
    std::uint64_t fingerprint=0;
};
struct Audit {
    std::uint64_t skin=0,bind=0,animation=0;
    unsigned long long influences=0;
    unsigned int invalidVertexIndices=0,invalidWeights=0,unnormalizedVertices=0;
    unsigned int missingBuffers=0,excessiveData=0;
    std::vector<Clip> clips;
    bool HasProblems() const {
        return invalidVertexIndices||invalidWeights||unnormalizedVertices||missingBuffers||excessiveData;
    }
};
const unsigned int kMaxSafeEntries=2000000;

template<class T> inline void KeyPayload(Fingerprint& h,const T* keys,unsigned int count,std::size_t floats) {
    if (!keys) {h.Integer(0xffffffffffffffffULL);return;}
    for (unsigned int i=0;i<count;++i) {
        h.Integer((std::uint64_t)keys[i].dwTime);
        h.Floats(keys[i].matMatrix,floats);
    }
}
template<class TObject> inline Audit Inspect(const TObject* object) {
    Audit out;
    Fingerprint skin,bind,animation;
    if (!object) {out.skin=skin.state;out.bind=bind.state;out.animation=animation.state;return out;}

    skin.Integer((std::uint64_t)object->iMeshCount);
    bind.Integer((std::uint64_t)object->iFrameCount);
    for (int i=0;i<object->iFrameCount;++i) {
        const auto* frame=object->ppFrameList?object->ppFrameList[i]:nullptr;
        bind.Integer((std::uint64_t)i);
        if (!frame) {bind.Integer(0);continue;}
        bind.Text(frame->szName);
        bind.Text(frame->pParent?frame->pParent->szName:"");
        bind.Floats(frame->matOriginal,16);
    }
    for (int i=0;i<object->iMeshCount;++i) {
        const auto* mesh=object->ppMeshList?object->ppMeshList[i]:nullptr;
        skin.Integer((std::uint64_t)i);
        if (!mesh) {skin.Integer(0);continue;}
        skin.Integer((std::uint64_t)mesh->dwVertexCount);
        skin.Integer((std::uint64_t)mesh->dwBoneCount);
        if (mesh->dwBoneCount>kMaxSafeEntries) {++out.excessiveData;continue;}
        std::vector<double> totals;
        if (mesh->pBones && mesh->dwVertexCount<=kMaxSafeEntries && mesh->dwBoneCount)
            totals.resize(mesh->dwVertexCount,0.0);
        else if (mesh->dwVertexCount>kMaxSafeEntries && mesh->dwBoneCount)
            ++out.excessiveData;
        if (!mesh->pBones&&mesh->dwBoneCount) {++out.missingBuffers;continue;}
        for (unsigned int j=0;j<mesh->dwBoneCount;++j) {
            const auto& bone=mesh->pBones[j];
            skin.Text(bone.szName);
            skin.Integer((std::uint64_t)bone.dwNumInfluences);
            bind.Text(bone.szName);
            bind.Floats(bone.matTranslation,16);
            if (bone.dwNumInfluences>kMaxSafeEntries) {++out.excessiveData;continue;}
            if (bone.dwNumInfluences&&(!bone.pVertices||!bone.pWeights)) {
                ++out.missingBuffers;skin.Integer(0xffffffffffffffffULL);continue;
            }
            for (unsigned int k=0;k<bone.dwNumInfluences;++k) {
                const unsigned int index=bone.pVertices[k];
                const float weight=bone.pWeights[k];
                ++out.influences;
                skin.Integer(index);skin.Real(weight);
                if (index>=mesh->dwVertexCount) {++out.invalidVertexIndices;continue;}
                if (!std::isfinite(weight)||weight<0.0f||weight>1.0001f) {
                    ++out.invalidWeights;continue;
                }
                if (index<totals.size()) totals[index]+=weight;
            }
        }
        // Only judge vertices with at least one bone influence; some source
        // meshes deliberately contain unskinned/rigid vertices.
        for (std::size_t v=0;v<totals.size();++v)
            if (totals[v]>0.000001 && std::fabs(totals[v]-1.0)>0.005)
                ++out.unnormalizedVertices;
    }

    for (const auto* set=object->pAnimationSet;set;set=set->pNext) {
        Clip clip;
        clip.name=set->szName;
        clip.duration=set->ulLength;
        Fingerprint clipHash;
        clipHash.Text(set->szName);
        clipHash.Integer((std::uint64_t)set->ulLength);
        clipHash.Real(set->fAnimSetStart);
        clipHash.Real(set->fAnimSetFinish);
        for (const auto* a=set->pAnimation;a;a=a->pNext) {
            ++clip.tracks;
            clipHash.Text(a->szName);
            clipHash.Integer((std::uint64_t)a->bLoop);
            clipHash.Integer((std::uint64_t)a->bLinear);
            const unsigned int count[4]={
                a->dwNumPositionKeys,a->dwNumRotateKeys,
                a->dwNumScaleKeys,a->dwNumMatrixKeys
            };
            for (int type=0;type<4;++type) {
                clip.keys+=count[type];
                clipHash.Integer(count[type]);
                if (count[type]>kMaxSafeEntries) ++out.excessiveData;
            }
            if (count[0]&&count[0]<=kMaxSafeEntries) {
                if (!a->pPositionKeys) ++out.missingBuffers;
                else for (unsigned int k=0;k<count[0];++k) {
                    clipHash.Integer(a->pPositionKeys[k].dwTime);
                    clipHash.Floats(a->pPositionKeys[k].vecPos,3);
                }
            }
            if (count[1]&&count[1]<=kMaxSafeEntries) {
                if (!a->pRotateKeys) ++out.missingBuffers;
                else for (unsigned int k=0;k<count[1];++k) {
                    clipHash.Integer(a->pRotateKeys[k].dwTime);
                    clipHash.Floats(a->pRotateKeys[k].Quaternion,4);
                }
            }
            if (count[2]&&count[2]<=kMaxSafeEntries) {
                if (!a->pScaleKeys) ++out.missingBuffers;
                else for (unsigned int k=0;k<count[2];++k) {
                    clipHash.Integer(a->pScaleKeys[k].dwTime);
                    clipHash.Floats(a->pScaleKeys[k].vecScale,3);
                }
            }
            if (count[3]&&count[3]<=kMaxSafeEntries) {
                if (!a->pMatrixKeys) ++out.missingBuffers;
                else for (unsigned int k=0;k<count[3];++k) {
                    clipHash.Integer(a->pMatrixKeys[k].dwTime);
                    clipHash.Floats(a->pMatrixKeys[k].matMatrix,16);
                }
            }
        }
        clip.fingerprint=clipHash.state;
        out.clips.push_back(clip);
        animation.Integer(clip.fingerprint);
    }
    out.skin=skin.state;out.bind=bind.state;out.animation=animation.state;
    return out;
}
inline std::string Compare(const Audit& before,const Audit& after) {
    if (after.missingBuffers||after.excessiveData) return "Saved DBO has missing or excessive skin/animation buffers";
    if (after.invalidVertexIndices||after.invalidWeights) return "Saved DBO has invalid skin weights or vertex references";
    if (before.skin!=after.skin) return "Skin influences/weights changed (indices, order, or numeric values)";
    if (before.bind!=after.bind) return "Bind-pose or original frame matrix changed";
    if (before.animation!=after.animation) return "Animation clip name, timing, track or key data changed";
    return std::string();
}
} // namespace PWSkeletalIntegrity
