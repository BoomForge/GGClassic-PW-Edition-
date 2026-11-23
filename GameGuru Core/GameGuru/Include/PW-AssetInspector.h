#pragma once
// PW Edition Asset Inspector: a deliberately SEPARATE, user-opened window.
// The permanent bottom Asset Browser and Classic toolbar remain untouched.
// Included in M-GridEdit.cpp after the engine/editor helpers.
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <cstdint>
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
    return Ends(p, ".dbo") || Ends(p, ".x") || Ends(p, ".fbx");
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
    return ModelFile(path) || (ImageFile(path) && !ThumbnailName(path));
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
inline void Release() {
    State& s=Current();
    if (s.modelId && ObjectExist(s.modelId)) DeleteObject(s.modelId);
    if (s.imageId && ImageExist(s.imageId)) DeleteImage(s.imageId);
    if (s.modelTextureId && ImageExist(s.modelTextureId)) DeleteImage(s.modelTextureId);
    if (s.previewId && ImageExist(s.previewId)) DeleteImage(s.previewId);
    if (s.modelEffectId>0) deleteinternaleffect(s.modelEffectId);
    s.modelId=s.imageId=s.modelTextureId=s.previewId=s.modelEffectId=0;
    s.isModel=s.isImage=s.renderRequested=false;
}
inline void Select(const std::string& file) {
    State& s=Current();
    if (file==s.selected) return;
    Release();
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
}
inline void InvalidateBrowser();
inline void Open() {
    State& s=Current();
    s.root=std::string(g.fpscrootdir_s.Get())+"\\Files";
    InvalidateBrowser();
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
// Build the browser once per open/refresh instead of enumerating thousands of
// unrelated files (scripts, setup metadata, baked thumbnails) every frame.
struct Entry {
    std::string name;
    bool folder=false;
    std::vector<Entry> children;
};
struct BrowseIndex {
    std::string root;
    std::vector<Entry> entries;
    bool valid=false;
    bool limited=false;
    size_t fileCount=0;
};
inline BrowseIndex& Index() { static BrowseIndex b; return b; }
inline void InvalidateBrowser() { Index().valid=false; Index().entries.clear(); }
inline void ScanAssets(const std::string& path, int depth, int& budget,
                       std::vector<Entry>& result, BrowseIndex& b) {
    if (depth>15 || !IsFolder(path)) return;
    WIN32_FIND_DATAA data={};
    HANDLE find=FindFirstFileA((path+"\\*").c_str(),&data);
    if (find==INVALID_HANDLE_VALUE) return;
    do {
        if (!strcmp(data.cFileName,".") || !strcmp(data.cFileName,"..") ||
            (data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) continue;
        if (--budget<0) { b.limited=true; break; }
        const std::string name=data.cFileName;
        const std::string full=path+"\\"+name;
        Entry e;
        e.name=name;
        e.folder=(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)!=0;
        if (e.folder) {
            if (AssetDirectory(name) && depth<15)
                ScanAssets(full,depth+1,budget,e.children,b);
            // Hide empty directories and directories containing only metadata.
            if (!e.children.empty()) result.push_back(std::move(e));
        } else if (ViewableFile(name)) {
            result.push_back(std::move(e));
            ++b.fileCount;
        }
    } while(FindNextFileA(find,&data));
    FindClose(find);
    std::sort(result.begin(),result.end(),[](const Entry& a,const Entry& b){
        if (a.folder!=b.folder) return a.folder>b.folder;
        return Lower(a.name)<Lower(b.name);
    });
}
inline void DrawIndexedTree(const std::string& path,const std::string& name,
                            const std::vector<Entry>& entries,int depth) {
    if(depth>15) return;
    ImGui::PushID(path.c_str());
    const bool expanded=ImGui::TreeNodeEx("##dir",ImGuiTreeNodeFlags_OpenOnArrow,
                                         "%s",name.c_str());
    if (expanded) {
        for (const Entry& e : entries) {
            const std::string full=path+"\\"+e.name;
            if (e.folder) DrawIndexedTree(full,e.name,e.children,depth+1);
            else if (ImGui::Selectable(e.name.c_str(),Current().selected==full))
                Select(full);
        }
        ImGui::TreePop();
    }
    ImGui::PopID();
}
inline void DrawTree(const std::string& path,const std::string& name,int depth) {
    BrowseIndex& b=Index();
    if (!b.valid || b.root!=path) {
        b.root=path;
        b.entries.clear();
        b.fileCount=0;
        b.limited=false;
        int budget=100000; // global scan cap keeps malformed/huge installs bounded
        ScanAssets(path,0,budget,b.entries,b);
        b.valid=true;
    }
    DrawIndexedTree(path,name,b.entries,depth);
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
        ImGui::TextUnformatted("Models and texture images");
        if (ImGui::Button("Refresh Files##PWAssetRefresh"))
            InvalidateBrowser();
        ImGui::Separator();
        DrawTree(s.root,"Files",0);
        if (Index().limited)
            ImGui::TextDisabled("Asset scan limit reached; some folders may be omitted.");
        ImGui::EndChild();
        ImGui::SameLine();
        ImGui::BeginChild("##PWAssetPreview",ImVec2(0,0),true);
        ImGui::TextWrapped("%s",s.selected.empty()?"Select a file on the left":NameOf(s.selected).c_str());
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
            const float size=(std::max)(120.f,(std::min)(500.f,ImGui::GetContentRegionAvail().x-5.f));
            if (view) ImGui::Image((ImTextureID)view,ImVec2(size,size));
            else {
                ImGui::Dummy(ImVec2(size,size));
                ImGui::TextDisabled("Preparing 3D preview...");
            }
            if (ImGui::IsItemHovered()) {
                ImGuiIO& io=ImGui::GetIO();
                bool changed=false;
                if (ImGui::IsMouseDragging(0)) {
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
            ImGui::TextDisabled("Drag to rotate. Scroll to zoom.");
            if (ImGui::Button("Reset View")) {
                s.yaw=s.pitch=0.f;s.zoom=1.f;s.renderRequested=true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Insert into Scene"))
                InsertIntoScene();
            if (s.renderRequested && iLaunchAfterSync==0)
                iLaunchAfterSync=32; // render after ImGui, as native preview does
        } else if (!s.status.empty()) {
            ImGui::TextWrapped("%s",s.status.c_str());
        } else {
            ImGui::TextDisabled("Select a DBO, X, FBX model or PNG, DDS, JPG, BMP, TGA image.");
        }
        if (!s.status.empty() && s.isModel) {
            ImGui::Separator();
            ImGui::TextWrapped("%s",s.status.c_str());
        }
        ImGui::EndChild();
    }
    ImGui::End();
    if (!s.open) { Release(); s.selected.clear(); }
}
inline void RenderPending() {
    State& s=Current();
    if (s.open && s.isModel && s.renderRequested &&
        s.modelId>0 && ObjectExist(s.modelId) && s.previewId>0) {
        RenderToPreview(s.modelId,s.previewId,s.yaw,s.pitch,s.zoom);
        s.renderRequested=false;
        if (!ImageExist(s.previewId) || !GetImagePointerView(s.previewId))
            s.status="3D preview capture failed. Select another model, then retry.";
    }
}
} // namespace PWAssetInspector
