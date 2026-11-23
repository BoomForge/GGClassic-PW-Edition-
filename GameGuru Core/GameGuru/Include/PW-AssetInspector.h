#pragma once
// PW Edition Asset Inspector: a deliberately SEPARATE, user-opened window.
// The permanent bottom Asset Browser and Classic toolbar remain untouched.
// Included in M-GridEdit.cpp after the engine/editor helpers.
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <cstdint>
#include <deque>
#include <fstream>
#include <string>
#include <utility>
#include <vector>
#include <windows.h>

namespace PWAssetInspector {
struct State {
    bool open = false;
    bool renderRequested = false;
    std::string selected;
    std::string root;
    std::string status;
    char searchQuery[128] = {};
    unsigned long loadMilliseconds = 0;
    std::string modelSource;
    std::string albedoSource;
    int imageId = 0;
    int modelId = 0;
    int modelTextureId = 0;
    int modelEffectId = 0;
    int previewId = 0;
    float yaw = 0.f;
    float pitch = 0.f;
    float zoom = 1.f;
    bool isImage = false;
    bool isModel = false;
};
inline State& Current() { static State value; return value; }
inline bool IsFile(const std::string& p) {
    const DWORD attr = GetFileAttributesA(p.c_str());
    return attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY);
}
inline bool IsFolder(const std::string& p) {
    const DWORD attr = GetFileAttributesA(p.c_str());
    return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY)
           && !(attr & FILE_ATTRIBUTE_REPARSE_POINT);
}
inline std::string Lower(const std::string& s) {
    std::string value = s;
    for (char& c : value) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return value;
}
inline bool Ends(const std::string& s, const char* ext) {
    const std::string e = ext;
    return s.size() >= e.size() && Lower(s.substr(s.size()-e.size())) == e;
}
inline bool ImageFile(const std::string& p) {
    return Ends(p, ".png") || Ends(p, ".dds") || Ends(p, ".jpg") ||
           Ends(p, ".jpeg") || Ends(p, ".bmp") || Ends(p, ".tga");
}
inline bool ModelFile(const std::string& p) {
    // FPE is metadata for placement, not a viewable 3D model file.
    // GLB/OBJ are not in the supported GameGuru preview-import path.
    return Ends(p, ".dbo") || Ends(p, ".x");
}
inline bool ThumbnailName(const std::string& filename) {
    const std::string n=Lower(filename);
    return n.find("thumbnail")!=std::string::npos ||
           n.find("_thumb")!=std::string::npos ||
           n.find("-thumb")!=std::string::npos ||
           n.find("thumb_")==0 || n.find("thumb-")==0 ||
           n.find("_preview")!=std::string::npos ||
           n.find("-preview")!=std::string::npos;
}
inline bool ViewableFile(const std::string& path) {
    return ModelFile(path) && !ThumbnailName(path);
}
inline bool AssetDirectory(const std::string& name) {
    const std::string n=Lower(name);
    return n!="thumb" && n!="thumbs" && n!="thumbnail" &&
           n!="thumbnails" && n!="thumbbank" && n!="thumbnailbank" &&
           n!="preview" && n!="previews" && n!="previewbank" &&
           n!="__pycache__";
}
inline std::string FolderOf(const std::string& file) {
    const size_t p = file.find_last_of("\\/");
    return p == std::string::npos ? "" : file.substr(0,p);
}
inline std::string NameOf(const std::string& file) {
    const size_t p = file.find_last_of("\\/");
    return file.substr(p == std::string::npos ? 0 : p+1);
}
inline std::string Stem(const std::string& file) {
    std::string n = NameOf(file);
    const size_t p = n.find_last_of('.');
    if (p != std::string::npos) n.resize(p);
    return n;
}
inline std::string Trim(const std::string& value) {
    const size_t a = value.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    const size_t b = value.find_last_not_of(" \t\r\n");
    return value.substr(a,b-a+1);
}
inline std::string ProfileField(const std::string& fpe, const char* field) {
    std::ifstream in(fpe, std::ios::binary);
    std::string line;
    const std::string wanted = Lower(field);
    int budget = 512;
    while (budget-- > 0 && std::getline(in,line)) {
        const size_t equals = line.find('=');
        if (equals == std::string::npos || Trim(line).empty() ||
            Trim(line)[0] == ';') continue;
        if (Lower(Trim(line.substr(0,equals))) == wanted) {
            std::string v = Trim(line.substr(equals+1));
            const size_t comment = v.find(';');
            if (comment != std::string::npos) v.resize(comment);
            return Trim(v);
        }
    }
    return "";
}
inline std::string ResolveProfileAsset(const std::string& fpe, const std::string& asset) {
    if (asset.empty() || asset.find(':') != std::string::npos ||
        asset[0] == '\\' || asset[0] == '/' || asset.find("..") != std::string::npos)
        return "";
    const std::string dir = FolderOf(fpe);
    const std::string local = dir + "\\" + asset;
    if (IsFile(local)) return local;
    const std::string bank = Current().root + "\\entitybank\\" + asset;
    if (IsFile(bank)) return bank;
    return "";
}
// The left tree deliberately hides FPE setup files, but native GameGuru
// entity models often rely on those sidecars to specify their real texture.
// Use the sidecar internally so directly browsing a DBO/X still previews it
// textured, without putting setup files back in the tree.
inline std::string TextureForModel(const std::string& model) {
    const std::string folder=FolderOf(model);
    WIN32_FIND_DATAA data={};
    HANDLE find=FindFirstFileA((folder+"\\*.fpe").c_str(),&data);
    if (find==INVALID_HANDLE_VALUE) return "";
    std::string result;
    int remaining=512;
    do {
        if (--remaining<0) break;
        if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        const std::string profile=folder+"\\"+data.cFileName;
        const std::string pointed=ProfileField(profile,"model");
        if (pointed.empty() || Lower(NameOf(pointed))!=Lower(NameOf(model)))
            continue;
        result=ResolveProfileAsset(profile,ProfileField(profile,"textured"));
        if (!result.empty()) break;
    } while(FindNextFileA(find,&data));
    FindClose(find);
    return result;
}
inline int FreeObjectId() {
    // 26400 was INSIDE g.gamerealtimeobjoffset..max, so runtime entities
    // could claim the preview model ID and leave an empty/broken preview.
    const int base = 220000;
    for (int i=0;i<900;++i) {
        const int id=base+i;
        if (id>0 && !ObjectExist(id)) return id;
    }
    return 0;
}
inline int FreeImageId() {
    // 71750 falls inside the editor's live thumbnail/image allocation bank.
    // Reserve IDs beyond established image ranges instead.
    const int base=120500;
    for (int i=0;i<900;++i) {
        const int id=base+i;
        if (id>0 && !ImageExist(id)) return id;
    }
    return 0;
}
inline void Release(bool keepShader=false) {
    State& s=Current();
    if (s.modelId && ObjectExist(s.modelId)) DeleteObject(s.modelId);
    if (s.imageId && ImageExist(s.imageId)) DeleteImage(s.imageId);
    if (s.modelTextureId && ImageExist(s.modelTextureId)) DeleteImage(s.modelTextureId);
    if (s.previewId && ImageExist(s.previewId)) DeleteImage(s.previewId);
    if (!keepShader && s.modelEffectId>0) deleteinternaleffect(s.modelEffectId);
    s.modelId=s.imageId=s.modelTextureId=s.previewId=0;
    if (!keepShader) s.modelEffectId=0;
    s.isModel=s.isImage=s.renderRequested=false;
}
inline void Select(const std::string& file) {
    State& s=Current();
    if (file==s.selected) return;
    Release(true); // Keep the compiled preview shader between model selections.
    const DWORD loadStarted=GetTickCount();
    s.loadMilliseconds=0;
    s.selected=file; s.status.clear(); s.modelSource.clear(); s.albedoSource.clear();
    s.yaw=s.pitch=0.f; s.zoom=1.f;
    if (!IsFile(file)) { s.status="File is unavailable."; return; }
    if (ImageFile(file)) {
        s.imageId=FreeImageId();
        if (s.imageId) {
            LoadImage((LPSTR)file.c_str(),s.imageId);
            s.isImage=ImageExist(s.imageId)!=0;
        }
        if (!s.isImage) s.status="The selected image could not be decoded.";
        return;
    }
    if (!ModelFile(file)) {
        s.status="Select an image or a 3D model to see its visual preview.";
        return;
    }
    s.modelSource=file;
    if (!Ends(file,".fpe"))
        s.albedoSource=TextureForModel(file);
    if (Ends(file,".fpe")) {
        s.modelSource=ResolveProfileAsset(file,ProfileField(file,"model"));
        s.albedoSource=ResolveProfileAsset(file,ProfileField(file,"textured"));
        if (s.modelSource.empty()) {
            s.status="FPE model reference could not be resolved.";
            return;
        }
    }
    s.modelId=FreeObjectId();
    if (!s.modelId) { s.status="No free temporary preview object ID."; return; }
    if (Ends(s.modelSource,".fbx")) {
        // Match the working native FBX importer conversion path; restore the
        // global cache preference immediately so preview does not affect
        // ordinary editor model imports.
        const bool oldCache = ::g_bOverrideXCacheWhenUsingImporter;
        ::g_bOverrideXCacheWhenUsingImporter = true;
        std::vector<char> writable(s.modelSource.begin(),s.modelSource.end());
        writable.push_back(0);
        LoadFBX(writable.data(),s.modelId);
        ::g_bOverrideXCacheWhenUsingImporter = oldCache;
    } else {
        LoadObject((LPSTR)s.modelSource.c_str(),s.modelId);
    }
    if (!ObjectExist(s.modelId)) {
        s.status="Could not load this model; no placeholder mesh was substituted.";
        s.modelId=0;
        return;
    }
    const PWImportedGeometry::Report geometry = PWImportedGeometry::Inspect(GetObjectData(s.modelId));
    if (!geometry.usable || !PWImportedGeometry::SensibleExtent(
        ObjectSizeX(s.modelId,2), ObjectSizeY(s.modelId,2), ObjectSizeZ(s.modelId,2))) {
        s.status="Model loaded without usable geometry; insertion is disabled.";
        DeleteObject(s.modelId);
        s.modelId=0;
        return;
    }
    // A standalone raw DBO/X/FBX isn't registered as an entity and normally
    // has no GameGuru object effect. Assign an inspector-owned shader just as
    // the native model importer does: without this it may render no pixels.
    char basicShader[]="effectbank\\reloaded\\entity_basic.fx";
    if (s.modelEffectId<=0)
        s.modelEffectId=loadinternaleffectunique(basicShader,1);
    if (s.modelEffectId>0) {
        SetObjectEffect(s.modelId,s.modelEffectId);
        MakeVector4(g.characterkitvector);
        SetVector4(g.characterkitvector,500000,1,0,0);
        SetEffectConstantV(s.modelEffectId,"EntityEffectControl",g.characterkitvector);
        DeleteVector4(g.characterkitvector);
    }
    if (!s.albedoSource.empty()) {
        s.modelTextureId=FreeImageId();
        if (s.modelTextureId) {
            LoadImage((LPSTR)s.albedoSource.c_str(),s.modelTextureId);
            // Bind AFTER the effect: the native importer does the same
            // because a new effect can discard old texture stages.
            if (ImageExist(s.modelTextureId)) TextureObject(s.modelId,s.modelTextureId);
        }
    }
    s.previewId=FreeImageId();
    if (!s.previewId) { s.status="No free image ID for a 3D preview."; return; }
    HideObject(s.modelId);
    s.isModel=true;
    s.renderRequested=true;
    s.loadMilliseconds=GetTickCount()-loadStarted;
}
inline void InvalidateBrowser();
inline void Open() {
    State& s=Current();
    const std::string root=std::string(g.fpscrootdir_s.Get())+"\\Files";
    // Preserve the completed lightweight filename index across popup opens.
    // Refresh Models is the explicit way to discover newly installed assets.
    if (s.root!=root) InvalidateBrowser();
    s.root=root;
    s.open=true;
}
inline bool ValidProfilePath(const std::string& full, std::string& relative) {
    const std::string prefix=Current().root+"\\entitybank\\";
    if (Lower(full).find(Lower(prefix)) != 0) return false;
    relative=full.substr(prefix.size());
    return !relative.empty() && relative.find("..")==std::string::npos;
}
inline std::string SafeName(const std::string& name) {
    std::string result="PW_";
    for (char c : name) {
        if (result.size() >= 58) break;
        const unsigned char value=static_cast<unsigned char>(c);
        result+=std::isalnum(value)?c:'_';
    }
    return result=="PW_"?"PW_Asset":result;
}
inline bool MakeModelProfile(std::string& relative, std::string& error) {
    State& s=Current();
    const std::string user=s.root+"\\entitybank\\user";
    const std::string root=user+"\\PWInspector";
    if (!IsFolder(user) && !CreateDirectoryA(user.c_str(),nullptr)) {
        error="Cannot create the user asset directory."; return false;
    }
    if (!IsFolder(root) && !CreateDirectoryA(root.c_str(),nullptr)) {
        error="Cannot create the custom inspector asset directory."; return false;
    }
    const std::string base=SafeName(Stem(s.selected));
    std::string name;
    for (int i=1;i<=999;++i) {
        const std::string candidate=base+(i==1?"":"_"+std::to_string(i));
        if (GetFileAttributesA((root+"\\"+candidate).c_str())==INVALID_FILE_ATTRIBUTES) {
            name=candidate; break;
        }
    }
    if (name.empty()) { error="No free custom asset name."; return false; }
    const std::string destination=root+"\\"+name;
    if (!CreateDirectoryA(destination.c_str(),nullptr)) {
        error="Cannot create custom model folder."; return false;
    }
    const std::string target=destination+"\\"+name+".dbo";
    bool copied=false;
    if (Ends(s.modelSource,".dbo"))
        copied=CopyFileA(s.modelSource.c_str(),target.c_str(),TRUE)!=0;
    else if (s.modelId && ObjectExist(s.modelId)) {
        // A raw X/FBX requires a native DBO before entitybank can place it.
        SaveObject((LPSTR)target.c_str(),s.modelId);
        copied=IsFile(target);
    }
    if (!copied) {
        RemoveDirectoryA(destination.c_str());
        error="Model could not be copied/converted into a native DBO.";
        return false;
    }
    // Include nearby material images in this asset's OWN subfolder so another
    // asset cannot silently reuse a texture with the same filename.
    const std::string sourceFolder=FolderOf(s.modelSource);
    WIN32_FIND_DATAA data={};
    HANDLE find=FindFirstFileA((sourceFolder+"\\*").c_str(),&data);
    std::uint64_t total=0;
    int count=0;
    if (find!=INVALID_HANDLE_VALUE) {
        do {
            if (data.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT))
                continue;
            const std::string filename=data.cFileName;
            if (!ImageFile(filename)) continue;
            const std::uint64_t bytes=(std::uint64_t(data.nFileSizeHigh)<<32)|data.nFileSizeLow;
            if (count>=48 || total+bytes>128ULL*1024ULL*1024ULL) break;
            const std::string original=sourceFolder+"\\"+filename;
            if (CopyFileA(original.c_str(),(destination+"\\"+filename).c_str(),TRUE)) {
                total+=bytes; ++count;
            }
        } while(FindNextFileA(find,&data));
        FindClose(find);
    }
    std::string textureLine;
    if (!s.albedoSource.empty() && ImageFile(s.albedoSource)) {
        const std::string imageName=NameOf(s.albedoSource);
        const std::string dest=destination+"\\"+imageName;
        if (IsFile(dest) || CopyFileA(s.albedoSource.c_str(),dest.c_str(),TRUE))
            textureLine="textured = "+imageName+"\r\n";
    }
    const std::string profile=destination+"\\"+name+".fpe";
    std::ofstream out(profile,std::ios::binary|std::ios::trunc);
    if (out) {
        out << "; PW Edition Asset Inspector auto-generated entity\r\n"
            << "desc = " << name << "\r\n"
            << "model = " << name << ".dbo\r\n"
            << textureLine
            << "effect = effectbank\\reloaded\\entity_basic.fx\r\n"
            << "defaultstatic = 0\r\n"
            << "scale = 100\r\n";
        out.flush();
    }
    if (!out.good()) {
        out.close();
        DeleteFileA(profile.c_str());
        DeleteFileA(target.c_str());
        // Preserve any copied images for diagnosis; never delete other assets.
        error="Could not save the model's custom FPE profile.";
        return false;
    }
    relative="user\\PWInspector\\"+name+"\\"+name+".fpe";
    return true;
}
inline bool InsertIntoScene() {
    State& s=Current();
    if (!s.isModel || !ObjectExist(s.modelId)) {
        s.status="Select a usable model before inserting."; return false;
    }
    std::string relative,error;
    if (!Ends(s.selected,".fpe") || !ValidProfilePath(s.selected,relative)) {
        if (!MakeModelProfile(relative,error)) { s.status=error; return false; }
    }
    t.addentityfile_s=relative.c_str();
    if (t.addentityfile_s=="") { s.status="Entity profile path was empty."; return false; }
    entity_adduniqueentity(false);
    const int id=t.entid;
    if (id<=0 || !ObjectExist(g.entitybankoffset+id) ||
        !PWImportedGeometry::Inspect(GetObjectData(g.entitybankoffset+id)).usable) {
        s.status="Entity profile failed to load usable geometry; no scene object created.";
        return false;
    }
    // Initialise the same entity defaults as the normal Asset Browser. Then
    // place on the ground directly in FRONT of the editor camera, never at
    // the mouse cursor or the world origin.
    t.grideditselect=5;
    t.inputsys.constructselection=id;
    editor_constructionselection();
    if (t.gridentity!=id) {
        s.status="Could not initialise entity placement.";
        t.inputsys.constructselection=0;
        return false;
    }
    const float distance=350.0f;
    const float x=NewXValue(CameraPositionX(),CameraAngleY(),distance);
    const float z=NewZValue(CameraPositionZ(),CameraAngleY(),distance);
    const float y=t.terrain.TerrainID>0
        ? BT_GetGroundHeight(t.terrain.TerrainID,x,z) : g.gdefaultterrainheight;
    t.gridentityposx_f=x;
    t.gridentityposz_f=z;
    t.gridentityposy_f=y+t.entityprofile[id].defaultheight+t.entityprofile[id].offy;
    t.gridentityhasparent=0;
    t.gridentityoverwritemode=0;
    gridedit_addentitytomap(); // native entity instance + undo recording
    t.gridentity=0;
    t.inputsys.constructselection=0;
    t.refreshgrideditcursor=1;
    editor_refreshentitycursor();
    g.projectmodified=1;
    s.status="Inserted into the scene in front of the camera.";
    return true;
}
// Index only model filenames. Large Steam Files folders are scanned in short
// batches on editor frames; never traverse the full directory tree in one UI
// call or load any 3D geometry until the user selects a model.
struct Entry {
    std::string name; // original filename for load; label is Stem(name)
    bool folder=false;
    std::vector<Entry> children;
};
struct FolderScan {
    std::string path;
    std::vector<size_t> indices; // stable until final sort, no raw vector pointers
    int depth=0;
};
struct SearchEntry {
    std::string path;
    std::string label;
    std::string lowerLabel;
};
struct BrowseIndex {
    std::string root;
    std::vector<Entry> entries;
    std::deque<FolderScan> pending;
    std::vector<SearchEntry> searchable;
    HANDLE find=INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAA found={};
    bool valid=false;
    bool scanning=false;
    bool limited=false;
    int remaining=100000;
    size_t fileCount=0;
    size_t visited=0;
};
inline BrowseIndex& Index() { static BrowseIndex b; return b; }
inline void InvalidateBrowser() {
    BrowseIndex& b=Index();
    if (b.find!=INVALID_HANDLE_VALUE) {
        FindClose(b.find);
        b.find=INVALID_HANDLE_VALUE;
    }
    b.pending.clear();
    b.entries.clear();
    b.searchable.clear();
    b.valid=false;
    b.scanning=false;
    b.limited=false;
    b.fileCount=b.visited=0;
}
inline void BeginBrowserScan(const std::string& root) {
    InvalidateBrowser();
    BrowseIndex& b=Index();
    b.root=root;
    b.valid=true;
    b.scanning=true;
    b.remaining=100000;
    FolderScan first;
    first.path=root;
    b.pending.push_back(std::move(first));
}
inline std::vector<Entry>* EntriesFor(BrowseIndex& b,
                                      const std::vector<size_t>& indices) {
    std::vector<Entry>* entries=&b.entries;
    for (size_t index: indices) {
        if (index>=entries->size()) return nullptr;
        entries=&(*entries)[index].children;
    }
    return entries;
}
inline void SortPruneUnique(std::vector<Entry>& entries) {
    for (Entry& e: entries)
        if (e.folder) SortPruneUnique(e.children);
    entries.erase(std::remove_if(entries.begin(),entries.end(),
        [](const Entry& e){ return e.folder && e.children.empty(); }),
        entries.end());
    // Prefer .dbo to .x when both formats exist for the same model in
    // the same directory. Do not deduplicate distinct folders.
    std::sort(entries.begin(),entries.end(),
        [](const Entry& a,const Entry& b){
            if (a.folder!=b.folder) return a.folder>b.folder;
            const std::string al=Lower(a.folder?a.name:Stem(a.name));
            const std::string bl=Lower(b.folder?b.name:Stem(b.name));
            if (al!=bl) return al<bl;
            if (!a.folder && !b.folder && Ends(a.name,".dbo")!=Ends(b.name,".dbo"))
                return Ends(a.name,".dbo");
            return Lower(a.name)<Lower(b.name);
        });
    for (size_t i=1;i<entries.size();) {
        if (!entries[i].folder && !entries[i-1].folder &&
            Lower(Stem(entries[i].name))==Lower(Stem(entries[i-1].name)))
            entries.erase(entries.begin()+i);
        else ++i;
    }
}
inline void IndexSearch(BrowseIndex& b,const std::string& folder,
                        const std::vector<Entry>& entries) {
    for (const Entry& e: entries) {
        const std::string path=folder+"\\"+e.name;
        if (e.folder) IndexSearch(b,path,e.children);
        else {
            SearchEntry found;
            found.path=path;
            found.label=Stem(e.name);
            found.lowerLabel=Lower(found.label);
            b.searchable.push_back(std::move(found));
        }
    }
}
inline void FinaliseBrowserScan(BrowseIndex& b) {
    if (b.find!=INVALID_HANDLE_VALUE) {
        FindClose(b.find);
        b.find=INVALID_HANDLE_VALUE;
    }
    b.pending.clear();
    SortPruneUnique(b.entries);
    b.searchable.clear();
    IndexSearch(b,b.root,b.entries);
    b.fileCount=b.searchable.size();
    b.scanning=false;
}
inline void TickBrowserScan() {
    BrowseIndex& b=Index();
    if (!b.valid || !b.scanning) return;
    const DWORD frameStarted=GetTickCount();
    // Bound each editor frame by both directory entries and milliseconds.
    // Searches work once the index is complete, rather than repeatedly
    // traversing the filesystem on every typed character.
    for (int processed=0;processed<192 && GetTickCount()-frameStarted<6;) {
        if (b.pending.empty() || b.remaining<=0) {
            if (b.remaining<=0) b.limited=true;
            FinaliseBrowserScan(b);
            break;
        }
        FolderScan& task=b.pending.front();
        if (b.find==INVALID_HANDLE_VALUE) {
            b.find=FindFirstFileA((task.path+"\\*").c_str(),&b.found);
            if (b.find==INVALID_HANDLE_VALUE) {
                b.pending.pop_front();
                continue;
            }
        }
        const WIN32_FIND_DATAA info=b.found;
        const std::string dir=task.path;
        const int depth=task.depth;
        const std::vector<size_t> indices=task.indices;
        const bool hasNext=FindNextFileA(b.find,&b.found)!=0;
        ++processed;
        ++b.visited;
        --b.remaining;
        if (strcmp(info.cFileName,".") && strcmp(info.cFileName,"..") &&
            !(info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
            const std::string name=info.cFileName;
            std::vector<Entry>* children=EntriesFor(b,indices);
            if (children) {
                if (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    if (depth<15 && AssetDirectory(name)) {
                        Entry item;
                        item.name=name;
                        item.folder=true;
                        children->push_back(std::move(item));
                        FolderScan child;
                        child.path=dir+"\\"+name;
                        child.indices=indices;
                        child.indices.push_back(children->size()-1);
                        child.depth=depth+1;
                        b.pending.push_back(std::move(child));
                    }
                } else if (ViewableFile(name)) {
                    Entry item;
                    item.name=name;
                    children->push_back(std::move(item));
                }
            }
        }
        if (!hasNext) {
            FindClose(b.find);
            b.find=INVALID_HANDLE_VALUE;
            b.pending.pop_front();
        }
    }
    if (b.scanning && b.pending.empty()) FinaliseBrowserScan(b);
}
inline void DrawIndexedTree(const std::string& path,const std::string& name,
                            const std::vector<Entry>& entries,int depth) {
    if (depth>15) return;
    ImGui::PushID(path.c_str());
    const bool expanded=ImGui::TreeNodeEx("##dir",ImGuiTreeNodeFlags_OpenOnArrow,
                                         "%s",name.c_str());
    if (expanded) {
        for (const Entry& e: entries) {
            const std::string full=path+"\\"+e.name;
            if (e.folder) DrawIndexedTree(full,e.name,e.children,depth+1);
            else {
                ImGui::PushID(full.c_str());
                if (ImGui::Selectable(Stem(e.name).c_str(),Current().selected==full))
                    Select(full);
                ImGui::PopID();
            }
        }
        ImGui::TreePop();
    }
    ImGui::PopID();
}
inline void DrawTree(const std::string& path,const std::string& name,int depth) {
    BrowseIndex& b=Index();
    if (!b.valid || b.root!=path) BeginBrowserScan(path);
    TickBrowserScan();
    if (b.scanning)
        ImGui::TextDisabled("Indexing models... %u files checked",
                            (unsigned)b.visited);
    else
        DrawIndexedTree(path,name,b.entries,depth);
}
inline void DrawSearchResults(const std::string& query) {
    BrowseIndex& b=Index();
    if (!b.valid) return;
    if (b.scanning) {
        ImGui::TextDisabled("Searching after model index completes...");
        return;
    }
    const std::string needle=Lower(query);
    size_t matches=0;
    for (const SearchEntry& e: b.searchable) {
        if (e.lowerLabel.find(needle)==std::string::npos) continue;
        ++matches;
        if (matches>200) continue; // Keep very broad search responsive.
        ImGui::PushID(e.path.c_str());
        if (ImGui::Selectable(e.label.c_str(),Current().selected==e.path))
            Select(e.path);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s",e.path.c_str());
        ImGui::PopID();
    }
    if (matches==0) ImGui::TextDisabled("No matching models.");
    if (matches>200)
        ImGui::TextDisabled("Showing first 200 of %u matches. Refine search.",
                            (unsigned)matches);
}
inline void Draw() {
    State& s=Current();
    if (!s.open) return;
    if (s.root.empty()) s.root=std::string(g.fpscrootdir_s.Get())+"\\Files";
    ImGui::SetNextWindowSize(ImVec2(920,610),ImGuiCond_FirstUseEver);
    const bool visible=ImGui::Begin("Asset Inspector##PWAssetInspector",&s.open,
                                    ImGuiWindowFlags_NoDocking);
    if (visible) {
        const float left=(std::min)(300.f,ImGui::GetContentRegionAvail().x*0.35f);
        ImGui::BeginChild("##PWAssetDirectoryTree",ImVec2(left,0),true);
        ImGui::TextUnformatted("3D Models");
        if (ImGui::Button("Refresh Models##PWAssetRefresh"))
            InvalidateBrowser();
        ImGui::InputText("Search models##PWAssetSearch",s.searchQuery,
                         sizeof(s.searchQuery));
        ImGui::Separator();
        // The scan runs in small main-thread slices: no heavy blocking
        // recursive walk when the Inspector first opens.
        if (!Index().valid || Index().root!=s.root)
            BeginBrowserScan(s.root);
        TickBrowserScan();
        if (Index().scanning)
            ImGui::TextDisabled("Indexing models... %u entries checked",
                                (unsigned)Index().visited);
        else {
            ImGui::TextDisabled("%u models", (unsigned)Index().fileCount);
            if (s.searchQuery[0])
                DrawSearchResults(s.searchQuery);
            else
                DrawIndexedTree(s.root,"Files",Index().entries,0);
        }
        if (Index().limited)
            ImGui::TextDisabled("Scan limit reached; some folders may be omitted.");
        ImGui::EndChild();
        ImGui::SameLine();
        ImGui::BeginChild("##PWAssetPreview",ImVec2(0,0),true);
        ImGui::TextWrapped("%s",s.selected.empty()?"Select a model on the left":Stem(s.selected).c_str());
        ImGui::Separator();
        if (s.isImage && s.imageId && ImageExist(s.imageId)) {
            const float w=(std::max)(1.f,(float)ImageWidth(s.imageId));
            const float h=(std::max)(1.f,(float)ImageHeight(s.imageId));
            const ImVec2 area=ImGui::GetContentRegionAvail();
            const float scale=(std::min)(area.x/(std::max)(w,1.f),
                                           (std::max)(120.f,area.y-75.f)/h);
            ID3D11ShaderResourceView* view=GetImagePointerView(s.imageId);
            if (view) ImGui::Image((ImTextureID)view,ImVec2(w*scale,h*scale));
            ImGui::Text("%d x %d pixels",ImageWidth(s.imageId),ImageHeight(s.imageId));
        } else if (s.isModel && s.modelId && ObjectExist(s.modelId)) {
            ID3D11ShaderResourceView* view=s.previewId?GetImagePointerView(s.previewId):NULL;
            const float availableWidth=ImGui::GetContentRegionAvail().x;
            const float size=(std::max)(120.f,(std::min)(500.f,availableWidth-5.f));
            // Keep the square 3D viewport centred within the right pane,
            // independently of the model-space camera centring below.
            const float leftPadding=(std::max)(0.f,(availableWidth-size)*0.5f);
            ImGui::SetCursorPosX(ImGui::GetCursorPosX()+leftPadding);
            // An Image is only drawn; it does not claim the mouse as an
            // active UI control. Without a capturing item, dragging the
            // preview can move the entire inspector window instead of the model.
            const ImVec2 previewCorner=ImGui::GetCursorScreenPos();
            ImGui::InvisibleButton("##PWAssetModelOrbit",ImVec2(size,size));
            const bool previewHovered=ImGui::IsItemHovered();
            const bool previewActive=ImGui::IsItemActive();
            if (view) {
                ImGui::GetWindowDrawList()->AddImage((ImTextureID)view,
                    previewCorner,ImVec2(previewCorner.x+size,previewCorner.y+size));
            } else {
                ImGui::TextDisabled("Preparing 3D preview...");
            }
            if (previewHovered || previewActive) {
                ImGuiIO& io=ImGui::GetIO();
                bool changed=false;
                if (previewActive && ImGui::IsMouseDragging(0)) {
                    s.yaw+=io.MouseDelta.x*0.7f;
                    s.pitch+=io.MouseDelta.y*0.7f;
                    if (s.pitch>80.f) s.pitch=80.f;
                    if (s.pitch<-80.f) s.pitch=-80.f;
                    changed=true;
                }
                if (io.MouseWheel!=0.f) {
                    s.zoom*=io.MouseWheel>0.f?0.86f:1.16f;
                    if (s.zoom<0.35f) s.zoom=0.35f;
                    if (s.zoom>4.f) s.zoom=4.f;
                    changed=true;
                }
                if (changed) s.renderRequested=true;
            }
            ImGui::TextDisabled("Drag to rotate. Scroll to zoom. Load: %lu ms",
                                s.loadMilliseconds);
            if (ImGui::Button("Reset View")) {
                s.yaw=s.pitch=0.f;s.zoom=1.f;s.renderRequested=true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Refresh 3D Preview")) {
                s.renderRequested=true;
                s.status.clear();
            }
            if (ImGui::Button("Insert into Scene"))
                InsertIntoScene();
            if (s.renderRequested && iLaunchAfterSync==0)
                iLaunchAfterSync=32; // render after ImGui, as native preview does
        } else if (!s.status.empty()) {
            ImGui::TextWrapped("%s",s.status.c_str());
        } else {
            ImGui::TextDisabled("Select a DBO or X model to preview.");
        }
        if (!s.status.empty() && s.isModel) {
            ImGui::Separator();
            ImGui::TextWrapped("%s",s.status.c_str());
        }
        ImGui::EndChild();
    }
    ImGui::End();
    if (!s.open) {
        Release();
        // Do not keep a Windows directory enumeration handle open after
        // closing midway through indexing; completed indexes stay cached.
        if (Index().scanning) InvalidateBrowser();
        s.selected.clear();
    }
}
inline void RenderPending() {
    State& s=Current();
    if (s.open && s.isModel && s.renderRequested &&
        s.modelId>0 && ObjectExist(s.modelId) && s.previewId>0) {
        RenderToPreview(s.modelId,s.previewId,s.yaw,s.pitch,s.zoom);
        s.renderRequested=false;
        if (!ImageExist(s.previewId) || !GetImagePointerView(s.previewId))
            s.status="3D preview capture failed. Use Refresh 3D Preview to retry.";
        else if (s.status.find("3D preview capture failed")==0)
            s.status.clear();
    }
}
} // namespace PWAssetInspector
