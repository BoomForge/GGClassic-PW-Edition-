#pragma once
// Pixelwolf editor-only scene hierarchy. GameGuru's native iHasParentIndex is
// the AUTHORITATIVE parent link: entity_savebank already persists it into FPM.
// PW folders are organisational only and use a project-local .pwfolders sidecar.
// Keep object transforms inside GameGuru's existing group/rubber-band subsystem.
#ifdef ENABLEIMGUI
#include <map>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

void editor_rec_addchildrentorubberband(int entityindex);
void gridedit_addEntityToRubberBandHighlights(int e);
void gridedit_clearentityrubberbandlist(void);

struct PWSceneFolder
{
    int id = 0;
    int parent = 0;
    std::string name;
};

static std::vector<PWSceneFolder> g_pwFolders;
static std::map<int, int> g_pwEntityFolders;
static std::string g_pwFolderProject;
static bool g_pwFolderProjectSeen = false;
static bool g_pwFolderDirty = false;
static int g_pwFolderNextId = 1;
static int g_pwRenamingFolder = 0;
static char g_pwRenameFolderText[96] = "";

static bool PWValidSceneEntity(int e)
{
    return e > 0 && e <= g.entityelementlist &&
        t.entityelement[e].bankindex > 0 && t.entityelement[e].obj > 0;
}

static bool PWSceneIsSavedProject(const std::string& name)
{
    if (name.size() <= 4 || _stricmp(name.c_str() + name.size() - 4, ".fpm") != 0)
        return false;
    DWORD attributes = GetFileAttributesA(name.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY);
}

static PWSceneFolder* PWFindFolder(int id)
{
    for (auto& f : g_pwFolders) if (f.id == id) return &f;
    return nullptr;
}

static std::string PWSafeFolderName(const std::string& raw)
{
    std::string name = raw.substr(0, 80);
    for (char& ch : name) if (ch == '\t' || ch == '\r' || ch == '\n' || (unsigned char)ch < 32)
        ch = ' ';
    if (name.find_first_not_of(' ') == std::string::npos) name = "New Folder";
    return name;
}

static void PWSaveSceneFolders()
{
    if (!PWSceneIsSavedProject(g_pwFolderProject)) return;
    const std::string target = g_pwFolderProject + ".pwfolders";
    const std::string temporary = target + ".tmp";
    {
        std::ofstream file(temporary, std::ios::out | std::ios::trunc);
        if (!file) return;
        file << "PW_SCENE_FOLDERS_1\n";
        for (const auto& folder : g_pwFolders)
            file << "F\t" << folder.id << '\t' << folder.parent << '\t'
                 << PWSafeFolderName(folder.name) << '\n';
        for (const auto& item : g_pwEntityFolders)
        {
            if (!PWValidSceneEntity(item.first) || !PWFindFolder(item.second)) continue;
            file << "E\t" << item.first << '\t' << item.second << '\t'
                 << t.entityelement[item.first].bankindex << '\n';
        }
        file.flush();
        if (!file) return;
    }
    if (!MoveFileExA(temporary.c_str(), target.c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
    {
        DeleteFileA(temporary.c_str());
        return;
    }
    g_pwFolderDirty = false;
}

static void PWLoadSceneFolders()
{
    g_pwFolders.clear();
    g_pwEntityFolders.clear();
    g_pwFolderNextId = 1;
    g_pwFolderDirty = false;
    if (!PWSceneIsSavedProject(g_pwFolderProject)) return;
    std::ifstream file(g_pwFolderProject + ".pwfolders");
    if (!file) return;
    std::string line;
    if (!std::getline(file, line) || line != "PW_SCENE_FOLDERS_1") return;
    while (std::getline(file, line))
    {
        std::istringstream parser(line);
        char type = 0;
        parser >> type;
        if (type == 'F')
        {
            PWSceneFolder folder;
            if (!(parser >> folder.id >> folder.parent)) continue;
            std::getline(parser, folder.name);
            if (!folder.name.empty() && folder.name.front() == '\t')
                folder.name.erase(folder.name.begin());
            folder.name = PWSafeFolderName(folder.name);
            if (folder.id < 1 || folder.id > 1000000 || PWFindFolder(folder.id))
                continue;
            g_pwFolders.push_back(folder);
            g_pwFolderNextId = (std::max)(g_pwFolderNextId, folder.id + 1);
        }
        else if (type == 'E')
        {
            int e = 0, folderId = 0, bankId = 0;
            if (!(parser >> e >> folderId >> bankId)) continue;
            if (PWValidSceneEntity(e) && t.entityelement[e].bankindex == bankId)
                g_pwEntityFolders[e] = folderId;
        }
    }
    for (auto& f : g_pwFolders)
        if (f.parent == f.id || (f.parent && !PWFindFolder(f.parent))) f.parent = 0;
    for (auto it = g_pwEntityFolders.begin(); it != g_pwEntityFolders.end();)
        if (!PWFindFolder(it->second)) it = g_pwEntityFolders.erase(it);
        else ++it;
}

static void PWEnsureSceneProject()
{
    const char* current = g.projectfilename_s.Get();
    const std::string path = current ? current : "";
    if (g_pwFolderProjectSeen && path == g_pwFolderProject) return;
    const bool firstSavedCopy = g_pwFolderProjectSeen &&
                                !PWSceneIsSavedProject(g_pwFolderProject) &&
                                g_pwFolderDirty && PWSceneIsSavedProject(path);
    g_pwFolderProject = path;
    g_pwFolderProjectSeen = true;
    if (firstSavedCopy) PWSaveSceneFolders();
    else PWLoadSceneFolders();
}

static void PWChangeSceneOrganisation()
{
    g_pwFolderDirty = true;
    PWSaveSceneFolders();
    // Native FPM parent relationships must also make the editor request a save.
    g.projectmodified = 1;
    gridedit_changemodifiedflag();
}

static int PWCreateSceneFolder(int parent)
{
    if (parent && !PWFindFolder(parent)) parent = 0;
    PWSceneFolder folder;
    folder.id = g_pwFolderNextId++;
    folder.parent = parent;
    folder.name = "New Folder";
    g_pwFolders.push_back(folder);
    PWChangeSceneOrganisation();
    return folder.id;
}

static bool PWIsFolderDescendant(int candidate, int ancestor)
{
    int guard = (int)g_pwFolders.size() + 1;
    while (candidate && guard-- > 0)
    {
        if (candidate == ancestor) return true;
        const PWSceneFolder* folder = PWFindFolder(candidate);
        if (!folder) break;
        candidate = folder->parent;
    }
    return false;
}

static bool PWMoveSceneFolder(int source, int target)
{
    PWSceneFolder* folder = PWFindFolder(source);
    if (!folder || source == target || (target && !PWFindFolder(target)) ||
        PWIsFolderDescendant(target, source))
        return false;
    folder->parent = target;
    PWChangeSceneOrganisation();
    return true;
}

static void PWRemoveSceneFolder(int id)
{
    PWSceneFolder* folder = PWFindFolder(id);
    if (!folder) return;
    const int parent = folder->parent;
    for (auto& item : g_pwEntityFolders)
        if (item.second == id) item.second = parent;
    for (auto& child : g_pwFolders)
        if (child.parent == id) child.parent = parent;
    g_pwFolders.erase(std::remove_if(g_pwFolders.begin(), g_pwFolders.end(),
        [id](const PWSceneFolder& f) { return f.id == id; }), g_pwFolders.end());
    for (auto it = g_pwEntityFolders.begin(); it != g_pwEntityFolders.end();)
        if (it->second == 0) it = g_pwEntityFolders.erase(it);
        else ++it;
    PWChangeSceneOrganisation();
}

static bool PWSetSceneParent(int child, int parent)
{
    if (!PWValidSceneEntity(child) || (parent && !PWValidSceneEntity(parent)) ||
        child == parent || t.entityelement[child].editorlock) return false;
    // Check the ENTIRE ancestor chain, not just the immediate parent.
    int walk = parent, guard = g.entityelementlist + 1;
    while (walk > 0 && guard-- > 0)
    {
        if (walk == child) return false;
        if (!PWValidSceneEntity(walk)) break;
        walk = t.entityelement[walk].iHasParentIndex;
    }
    if (guard <= 0) return false;
    t.entityelement[child].iHasParentIndex = parent;
    g_pwEntityFolders.erase(child);
    PWChangeSceneOrganisation();
    return true;
}

static void PWMoveEntityToFolder(int e, int folder)
{
    if (!PWValidSceneEntity(e) || t.entityelement[e].editorlock) return;
    if (folder && !PWFindFolder(folder)) return;
    t.entityelement[e].iHasParentIndex = 0;
    if (folder) g_pwEntityFolders[e] = folder;
    else g_pwEntityFolders.erase(e);
    PWChangeSceneOrganisation();
}

static void PWSelectHierarchyEntity(int e)
{
    if (!PWValidSceneEntity(e) || t.entityelement[e].editorlock) return;
    widget_switchoff();
    t.widget.pickedEntityIndex = 0;
    t.gridentity = 0;
    t.grideditselect = 5;
    t.inputsys.domodeentity = 1;

    // The native GameGuru grouped-transform path uses this same list when
    // moving/rotating a selected parent in the viewport. Include descendants.
    gridedit_clearentityrubberbandlist();
    bool hasChildren = false;
    for (int child = 1; child <= g.entityelementlist; ++child)
        if (PWValidSceneEntity(child) && t.entityelement[child].iHasParentIndex == e)
            { hasChildren = true; break; }
    if (hasChildren)
    {
        gridedit_addEntityToRubberBandHighlights(e);
        editor_rec_addchildrentorubberband(e);
    }
    iWidgetSelection = e;
}

static void PWEntityDragTarget(int target)
{
    if (!ImGui::BeginDragDropTarget()) return;
    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PW_SCENE_ENTITY"))
        if (payload->DataSize == sizeof(int))
            PWSetSceneParent(*(const int*)payload->Data, target);
    ImGui::EndDragDropTarget();
}

static void PWFolderDragTarget(int target)
{
    if (!ImGui::BeginDragDropTarget()) return;
    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PW_SCENE_ENTITY"))
        if (payload->DataSize == sizeof(int))
            PWMoveEntityToFolder(*(const int*)payload->Data, target);
    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PW_SCENE_FOLDER"))
        if (payload->DataSize == sizeof(int))
            PWMoveSceneFolder(*(const int*)payload->Data, target);
    ImGui::EndDragDropTarget();
}

static void PWDrawSceneEntity(int e,
    const std::vector<std::vector<int>>& children, std::vector<bool>& displayed, int depth)
{
    if (!PWValidSceneEntity(e) || displayed[e] || depth > 40) return;
    displayed[e] = true;
    const char* name = t.entityelement[e].eleprof.name_s.Get();
    if (!name || !*name) name = "Unnamed entity";
    const bool locked = t.entityelement[e].editorlock != 0;
    const bool hasChildren = !children[e].empty();
    ImGui::PushID(e + 200000);
    int flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen |
                (t.widget.pickedEntityIndex == e ? ImGuiTreeNodeFlags_Selected : 0);
    if (!hasChildren) flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    const std::string label = std::string(name) + (locked ? " [locked]" : "");
    bool open = ImGui::TreeNodeEx("##PWEntityNode", flags, "%s", label.c_str());
    if (ImGui::IsItemClicked(0) && !locked) PWSelectHierarchyEntity(e);
    if (!locked && ImGui::BeginDragDropSource())
    {
        ImGui::SetDragDropPayload("PW_SCENE_ENTITY", &e, sizeof(e));
        ImGui::TextUnformatted(name);
        ImGui::EndDragDropSource();
    }
    PWEntityDragTarget(e);
    if (ImGui::BeginPopupContextItem("PWEntityActions"))
    {
        if (ImGui::MenuItem("Select")) PWSelectHierarchyEntity(e);
        if (ImGui::MenuItem("Edit Lua Script..."))
            PWLuaStudio::OpenEntityScript(GetActiveWindow(),
                t.entityelement[e].eleprof.aimain_s.Get());
        if (ImGui::MenuItem("Unparent / move to scene root",
            nullptr, false, !locked && (t.entityelement[e].iHasParentIndex != 0 ||
                                        g_pwEntityFolders.count(e) != 0)))
            PWMoveEntityToFolder(e, 0);
        ImGui::EndPopup();
    }
    if (open && hasChildren)
    {
        for (int child : children[e]) PWDrawSceneEntity(child, children, displayed, depth + 1);
        ImGui::TreePop();
    }
    ImGui::PopID();
}

static void PWDrawSceneFolder(int folderId,
    const std::vector<std::vector<int>>& children, std::vector<bool>& displayed, int depth)
{
    PWSceneFolder* folder = PWFindFolder(folderId);
    if (!folder || depth > 40) return;
    ImGui::PushID(100000 + folderId);
    const std::string label = folder->name;
    const bool open = ImGui::TreeNodeEx("##PWFolderNode",
        ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen, "%s", label.c_str());
    if (ImGui::BeginDragDropSource())
    {
        ImGui::SetDragDropPayload("PW_SCENE_FOLDER", &folderId, sizeof(folderId));
        ImGui::TextUnformatted(label.c_str());
        ImGui::EndDragDropSource();
    }
    PWFolderDragTarget(folderId);
    if (ImGui::BeginPopupContextItem("PWFolderActions"))
    {
        if (ImGui::MenuItem("New subfolder")) PWCreateSceneFolder(folderId);
        if (ImGui::MenuItem("Rename"))
        {
            g_pwRenamingFolder = folderId;
            strncpy_s(g_pwRenameFolderText, sizeof(g_pwRenameFolderText),
                      folder->name.c_str(), _TRUNCATE);
        }
        if (ImGui::MenuItem("Delete folder (keep contents)"))
            PWRemoveSceneFolder(folderId);
        ImGui::EndPopup();
    }
    if (g_pwRenamingFolder == folderId)
    {
        ImGui::InputText("Name", g_pwRenameFolderText, sizeof(g_pwRenameFolderText));
        ImGui::SameLine();
        if (ImGui::SmallButton("Save name"))
        {
            PWSceneFolder* renamed = PWFindFolder(folderId);
            if (renamed) { renamed->name = PWSafeFolderName(g_pwRenameFolderText); PWChangeSceneOrganisation(); }
            g_pwRenamingFolder = 0;
        }
    }
    if (open)
    {
        // Re-find by ID because editing a popup can grow/reallocate the vector.
        std::vector<int> subfolders;
        for (const auto& f : g_pwFolders) if (f.parent == folderId) subfolders.push_back(f.id);
        for (int id : subfolders) PWDrawSceneFolder(id, children, displayed, depth + 1);
        // Item drag/drop can modify the entity->folder map. Render a stable
        // list of IDs rather than iterating while erasing/reparenting.
        std::vector<int> folderEntities;
        for (const auto& entry : g_pwEntityFolders)
            if (entry.second == folderId && PWValidSceneEntity(entry.first) &&
                t.entityelement[entry.first].iHasParentIndex == 0)
                folderEntities.push_back(entry.first);
        for (int e : folderEntities)
            PWDrawSceneEntity(e, children, displayed, depth + 1);
        ImGui::TreePop();
    }
    ImGui::PopID();
}

// Explicit player setup uses the same FPM fields and compatibility marker as
// Classic's drag-on-start-marker workflow. Never modify shared FPE profiles.
static bool PWPlayerSetupEntity(int e, bool marker)
{
    if (!PWValidSceneEntity(e)) return false;
    auto& ent = t.entityelement[e];
    const int bank = ent.bankindex;
    if (bank > g.entitybankmax || ent.editorlock || !ObjectExist(ent.obj)) return false;
    if (marker) return t.entityprofile[bank].ismarker == 1;
    if (t.entityprofile[bank].ischaracter != 1 ||
        t.entityprofile[bank].ismarker != 0 || ent.staticflag != 0 ||
        ent.iHasParentIndex != 0 || t.entityprofile[bank].animmax <= 0) return false;
    for (int child = 1; child <= g.entityelementlist; ++child)
        if (PWValidSceneEntity(child) && t.entityelement[child].iHasParentIndex == e)
            return false;
    return true;
}

static bool PWPlayerSetupIdle()
{
    return g.vrqcontrolmode == 0 && t.gridentity == 0 && t.gridentityinzoomview == 0;
}

static bool PWPlayerSetupCandidate(int e)
{
    if (!PWPlayerSetupEntity(e, false)) return false;
    auto& ent = t.entityelement[e];
    // Do not silently discard an instance-specific NPC script.
    const char* script = ent.eleprof.aimain_s.Get();
    const char* stock = t.entityprofile[ent.bankindex].aimain_s.Get();
    return script && stock && _stricmp(script, stock) == 0;
}

static bool PWAssignScenePlayer(int character, int marker)
{
    if (!PWPlayerSetupIdle() || t.playercontrol.thirdperson.enabled != 0 ||
        character == marker || !PWPlayerSetupCandidate(character) ||
        !PWPlayerSetupEntity(marker, true)) return false;
    auto& avatar = t.entityelement[character];
    auto& start = t.entityelement[marker];
    // Persist the native references, not a sidecar. The empty legacy marker
    // is set internally only for compatibility with Classic's existing path.
    avatar.eleprof.aimain_s = "tpp\\thirdperson.lua";
    avatar.eleprof.aimain = 0;
    avatar.x = start.x; avatar.y = start.y; avatar.z = start.z;
    avatar.rx = start.rx; avatar.ry = start.ry; avatar.rz = start.rz;
    PositionObject(avatar.obj, avatar.x, avatar.y, avatar.z);
    RotateObject(avatar.obj, avatar.rx, avatar.ry, avatar.rz);
    t.playercontrol.thirdperson.charactere = character;
    t.playercontrol.thirdperson.startmarkere = marker;
    t.playercontrol.thirdperson.enabled = 1;
    g.projectmodified = 1;
    g.projectmodifiedstatic = 1;
    gridedit_changemodifiedflag();
    return true;
}

static bool PWCanReturnToFirstPerson()
{
    if (!PWPlayerSetupIdle() || t.playercontrol.thirdperson.enabled != 1) return false;
    const int character = t.playercontrol.thirdperson.charactere;
    const int marker = t.playercontrol.thirdperson.startmarkere;
    if (!PWPlayerSetupEntity(character, false) || !PWPlayerSetupEntity(marker, true)) return false;
    const char* script = t.entityelement[character].eleprof.aimain_s.Get();
    // A custom protagonist script must not be replaced behind the author's back.
    return script && _stricmp(script, "tpp\\thirdperson.lua") == 0;
}

static void PWReturnToFirstPerson()
{
    if (!PWCanReturnToFirstPerson()) return;
    auto& avatar = t.entityelement[t.playercontrol.thirdperson.charactere];
    const int markerObj = t.entityelement[t.playercontrol.thirdperson.startmarkere].obj;
    avatar.eleprof.aimain_s = t.entityprofile[avatar.bankindex].aimain_s;
    avatar.eleprof.aimain = 0;
    PositionObject(avatar.obj, avatar.x, avatar.y, avatar.z);
    RotateObject(avatar.obj, avatar.rx, avatar.ry, avatar.rz);
    DisableObjectZDepth(markerObj);
    DisableObjectZWrite(markerObj);
    DisableObjectZRead(markerObj);
    t.playercontrol.thirdperson.enabled = 0;
    t.playercontrol.thirdperson.charactere = 0;
    t.playercontrol.thirdperson.startmarkere = 0;
    g.projectmodified = 1;
    g.projectmodifiedstatic = 1;
    gridedit_changemodifiedflag();
}

// Asset-specific CSI authoring. A new sibling FPE uses Classic's own parser;
// the original profile, model, other instances, and current map are untouched.
struct PWPlayerClipRow { const char* label; const char* key; int slot; int first; int last; bool edit; };
static int PWPlayerClipLimit(int bank)
{
    if (bank <= 0 || bank > g.entidmaster) return 0;
    const int object = g.entitybankoffset + bank;
    if (!ObjectExist(object)) return 0;
    const sObject* data = GetObjectData(object);
    if (!data || !data->pAnimationSet || !data->pAnimationSet->pAnimation) return 0;
    const auto length = data->pAnimationSet->ulLength;
    return length > 1 && length <= 1000000 ? static_cast<int>(length) : 0;
}

static std::string PWClipTrim(const std::string& s)
{
    const size_t begin = s.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) return "";
    return s.substr(begin, s.find_last_not_of(" \t\r\n") - begin + 1);
}
static std::string PWClipLower(std::string s)
{
    for (char& ch : s) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    return s;
}
static std::string PWClipKey(const std::string& line)
{
    const size_t eq = line.find('=');
    if (eq == std::string::npos) return "";
    return PWClipLower(PWClipTrim(line.substr(0, eq)));
}
static bool PWClipSafeFile(const std::string& path, bool directory)
{
    const DWORD attr = GetFileAttributesA(path.c_str());
    return attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_REPARSE_POINT) &&
        ((attr & FILE_ATTRIBUTE_DIRECTORY) != 0) == directory;
}
static std::string PWPlayerProfilePath(int bank)
{
    if (bank <= 0 || bank > g.entidmaster) return "";
    std::string rel = t.entitybank_s[bank].Get();
    std::replace(rel.begin(), rel.end(), '/', '\\');
    if (PWClipLower(rel).compare(0, 11, "entitybank\\") == 0) rel.erase(0, 11);
    if (PWClipLower(rel).compare(0, 5, "user\\") != 0 || rel.find("..") != std::string::npos ||
        rel.find(':') != std::string::npos || rel.size() < 9 ||
        PWClipLower(rel.substr(rel.size()-4)) != ".fpe") return "";
    std::string path = std::string(g.fpscrootdir_s.Get()) + "\\Files\\entitybank";
    if (!PWClipSafeFile(path, true)) return "";
    size_t at = 0;
    for (;;) {
        const size_t slash = rel.find('\\', at);
        const std::string part = rel.substr(at, slash == std::string::npos ? slash : slash-at);
        if (part.empty() || part == "." || part.back() == '.' || part.back() == ' ') return "";
        path += "\\" + part;
        if (slash == std::string::npos) return PWClipSafeFile(path, false) ? path : "";
        if (!PWClipSafeFile(path, true)) return "";
        at = slash + 1;
    }
}
static bool PWPlayerClipValid(const PWPlayerClipRow& row, int limit)
{
    // Classic uses zero as the unassigned sentinel, and 100..101 as a
    // special idle fallback; avoid authoring either as a real CSI clip.
    return row.first > 0 && row.last > row.first && row.last <= limit &&
        !(row.first == 100 && row.last == 101);
}
static bool PWWritePlayerClipVariant(const std::string& source,
    const std::vector<PWPlayerClipRow>& rows, int limit, std::string& status)
{
    std::ifstream in(source, std::ios::binary | std::ios::ate);
    if (!in || in.tellg() <= 0 || in.tellg() > 1024*1024) {
        status = "Cannot read the source FPE (or it exceeds 1 MB)."; return false;
    }
    in.seekg(0);
    std::vector<std::string> lines;
    std::string line;
    bool haveAnimMax = false, haveModel = false, any = false, mappedVault = false;
    for (const auto& row : rows) {
        if (!row.edit) continue;
        any = true;
        if (std::string(row.key) == "csi_stoodvault") mappedVault = true;
        if (!PWPlayerClipValid(row, limit)) { status = "Invalid enabled frame range."; return false; }
    }
    if (!any) { status = "Enable at least one mapping."; return false; }
    while (std::getline(in, line)) {
        if (line.find('\0') != std::string::npos) { status = "Unsupported FPE encoding."; return false; }
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const std::string key = PWClipKey(line);
        if (key == "animmax") haveAnimMax = true;
        if (key == "model" && !PWClipTrim(line.substr(line.find('=')+1)).empty()) haveModel = true;
        lines.push_back(line);
    }
    if (in.bad() || !haveAnimMax || !haveModel) {
        status = "Source must contain explicit model and animmax entries."; return false;
    }
    const std::string base = source.substr(0, source.size()-4);
    for (int index=1; index<=99; ++index) {
        const std::string suffix = "_pw_player" + (index == 1 ? std::string("") : "_" + std::to_string(index));
        const std::string target = base + suffix + ".fpe";
        // Keep source-relative model/textures/scripts in the same directory.
        // The asset description also distinguishes the new profile in lists.
        const std::string desc = target.substr(target.find_last_of("\\")+1);
        std::ostringstream out;
        bool inserted = false, descWritten = false;
        for (const auto& original : lines) {
            const std::string key = PWClipKey(original);
            bool replaced = false;
            for (const auto& row : rows) if (row.edit && key == row.key) { replaced = true; break; }
            if (replaced || (mappedVault && (key == "jumpvaulttrim" || key == "jumphold"))) continue;
            if (key == "desc") {
                if (!descWritten) out << "desc = " << desc.substr(0,desc.size()-4) << "\r\n";
                descWritten = true; continue;
            }
            out << original << "\r\n";
            // CSI parsing begins only after animmax. Classic uses the first
            // occurrence, so remove old enabled keys and insert here.
            if (!inserted && key == "animmax") {
                // Exact authored jump range: disable Classic\'s automatic 10-frame trim;
                // clear stale jump hold so native code derives one within the new clip.
                if (mappedVault) out << "jumpvaulttrim = 0\r\njumphold = 0\r\n";
                for (const auto& row : rows) if (row.edit)
                    out << row.key << " = " << row.first << "," << row.last << "\r\n";
                inserted = true;
            }
        }
        if (!descWritten) out << "desc = " << desc.substr(0,desc.size()-4) << "\r\n";
        const std::string data = out.str();
        HANDLE file = CreateFileA(target.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
            FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) {
            if (GetLastError() == ERROR_FILE_EXISTS || GetLastError() == ERROR_ALREADY_EXISTS) continue;
            status = "Could not create a new custom profile."; return false;
        }
        DWORD written = 0;
        const bool ok = WriteFile(file, data.data(), static_cast<DWORD>(data.size()), &written, nullptr) &&
            written == data.size() && FlushFileBuffers(file);
        CloseHandle(file);
        if (!ok) { DeleteFileA(target.c_str()); status = "Profile write failed."; return false; }
        status = "Created " + desc + ". Refresh the asset browser (or reopen the editor), place the new variant and assign it as player.";
        return true;
    }
    status = "No free variant filename."; return false;
}

// Implemented next to the existing independent Asset Inspector.
static void PWOpenPlayerAnimationPreview(const std::string& profile);
static void PWRenderPlayerClipEditor(int e)
{
    if (!PWPlayerSetupEntity(e, false)) return;
    const int bank = t.entityelement[e].bankindex;
    const int limit = PWPlayerClipLimit(bank);
    const std::string source = PWPlayerProfilePath(bank);
    if (source.empty() || limit < 2) {
        ImGui::TextDisabled("Animation authoring requires a custom animated FPE in entitybank/user.");
        return;
    }
    if (t.entityprofile[bank].ischaractercreator != 0 || t.entityprofile[bank].appendanimmax > 0) {
        ImGui::TextDisabled("Generated rigs and externally appended animation sets need a separate authoring path.");
        return;
    }
    static std::vector<PWPlayerClipRow> rows;
    static std::string status, openedSource;
    static int openedBank = 0;
    if (ImGui::SmallButton("Animation mapping...")) {
        rows = {
            {"Unarmed idle A","csi_unarmed1",g.csi_unarmedANIM0,0,0,false},
            {"Unarmed idle B","csi_unarmed2",g.csi_unarmedANIM1,0,0,false},
            {"Unarmed walk","csi_unarmedmovefore",g.csi_unarmedmoveforeANIM,0,0,false},
            {"Unarmed run","csi_unarmedmoverun",g.csi_unarmedmoverunANIM,0,0,false},
            {"Unarmed death","csi_unarmeddeath",g.csi_unarmeddeathANIM,0,0,false},
            {"Armed idle / aim","csi_stoodnormal",t.csi_stoodnormalANIM[1],0,0,false},
            {"Armed walk","csi_stoodmovefore",t.csi_stoodmoveforeANIM[1],0,0,false},
            {"Armed backpedal","csi_stoodmoveback",t.csi_stoodmovebackANIM[1],0,0,false},
            {"Armed left","csi_stoodmoveleft",t.csi_stoodmoveleftANIM[1],0,0,false},
            {"Armed right","csi_stoodmoveright",t.csi_stoodmoverightANIM[1],0,0,false},
            {"Armed run","csi_stoodmoverun",t.csi_stoodmoverunANIM[1],0,0,false},
            {"Jump / vault","csi_stoodvault",t.csi_stoodvaultANIM[1],0,0,false},
            {"Punch","csi_stoodpunch",t.csi_stoodpunchANIM[1],0,0,false},
            {"Kick","csi_stoodkick",t.csi_stoodkickANIM[1],0,0,false},
            {"Reload","csi_stoodreload",t.csi_stoodreloadANIM[1],0,0,false},
            {"Crouch down","csi_crouchdown",t.csi_crouchdownANIM[1],0,0,false},
            {"Crouch idle A","csi_crouchidlenormal1",t.csi_crouchidlenormalANIM0[1],0,0,false},
            {"Crouch idle B","csi_crouchidlenormal2",t.csi_crouchidlenormalANIM1[1],0,0,false},
            {"Crouch walk","csi_crouchmovefore",t.csi_crouchmoveforeANIM[1],0,0,false},
            {"Crouch back","csi_crouchmoveback",t.csi_crouchmovebackANIM[1],0,0,false},
            {"Crouch left","csi_crouchmoveleft",t.csi_crouchmoveleftANIM[1],0,0,false},
            {"Crouch right","csi_crouchmoveright",t.csi_crouchmoverightANIM[1],0,0,false},
            {"Stand up","csi_crouchgetup",t.csi_crouchgetupANIM[1],0,0,false}
        };
        const int offset = t.entityprofile[bank].startofaianim;
        for (auto& row : rows) {
            if (offset < 0 || row.slot < 0 || offset > g.animmax || row.slot > g.animmax-offset) continue;
            row.first = t.entityanim[bank][offset+row.slot].start;
            row.last = t.entityanim[bank][offset+row.slot].finish;
        }
        openedBank = bank; openedSource = source; status.clear();
        ImGui::OpenPopup("PWPlayerAnimationMap");
    }
    if (ImGui::BeginPopupModal("PWPlayerAnimationMap", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (openedBank != bank || openedSource != source) {
            ImGui::TextUnformatted("Player asset changed. Close and reopen mapping.");
        } else {
            ImGui::Text("Native animation extent: %d", limit);
            ImGui::TextWrapped("Enable only the ranges to replace. Base weapon style only; the original FPE is never edited.");
            for (size_t i=0;i<rows.size();++i) {
                auto& row=rows[i]; ImGui::PushID(static_cast<int>(i));
                ImGui::Checkbox("##replace",&row.edit); ImGui::SameLine();
                ImGui::TextUnformatted(row.label); ImGui::SameLine(175);
                ImGui::PushItemWidth(85); ImGui::InputInt("##first",&row.first,0); ImGui::SameLine();
                ImGui::InputInt("##last",&row.last,0); ImGui::PopItemWidth();
                if (row.edit && !PWPlayerClipValid(row,limit)) {
                    ImGui::SameLine(); ImGui::TextUnformatted("Invalid");
                }
                ImGui::PopID();
            }
            if (ImGui::Button("Save new character profile"))
                PWWritePlayerClipVariant(source, rows, limit, status);
            if (!status.empty()) ImGui::TextWrapped("%s",status.c_str());
        }
        if (ImGui::Button("Close")) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    if (ImGui::SmallButton("Preview animation frames...")) PWOpenPlayerAnimationPreview(source);
}

static void PWRenderPlayerSetup()
{
    if (!ImGui::CollapsingHeader("Player Setup")) return;
    const bool thirdPerson = t.playercontrol.thirdperson.enabled == 1;
    ImGui::Text("Mode: %s", thirdPerson ? "Third person" : "First person");
    if (thirdPerson)
    {
        const int e = t.playercontrol.thirdperson.charactere;
        const int marker = t.playercontrol.thirdperson.startmarkere;
        if (PWPlayerSetupEntity(e, false) && PWPlayerSetupEntity(marker, true))
        {
            ImGui::TextWrapped("Character: %s (#%d)", t.entityelement[e].eleprof.name_s.Get(), e);
            ImGui::TextWrapped("Start: %s (#%d)", t.entityelement[marker].eleprof.name_s.Get(), marker);
            ImGui::TextWrapped("Camera settings are in the start marker properties.");
            PWRenderPlayerClipEditor(e);
        }
        else ImGui::TextWrapped("Player reference is missing or not eligible for this setup tool. Inspect the existing map before changing it.");
        if (PWCanReturnToFirstPerson() && ImGui::SmallButton("Return to first person..."))
            ImGui::OpenPopup("PWFirstPersonConfirm");
        if (ImGui::BeginPopupModal("PWFirstPersonConfirm", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextWrapped("Keep the character and restore its asset's default AI script? Move the former player away from the start marker before testing.");
            if (ImGui::Button("Restore first person")) { PWReturnToFirstPerson(); ImGui::CloseCurrentPopup(); }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
    }
    else if (PWPlayerSetupIdle())
    {
        ImGui::TextWrapped("Place a player start marker and an animated dynamic character, then choose both below. No manual player script is needed.");
        if (ImGui::SmallButton("Set up third person...")) ImGui::OpenPopup("PWPlayerSetupChoose");
        if (ImGui::BeginPopup("PWPlayerSetupChoose"))
        {
            bool found = false;
            for (int e = 1; e <= g.entityelementlist; ++e)
            {
                if (!PWPlayerSetupCandidate(e)) continue;
                found = true;
                ImGui::PushID(e);
                const std::string label = std::string(t.entityelement[e].eleprof.name_s.Get()) + " (#" + std::to_string(e) + ")";
                if (ImGui::BeginMenu(label.c_str()))
                {
                    bool markerFound = false;
                    for (int m = 1; m <= g.entityelementlist; ++m)
                    {
                        if (!PWPlayerSetupEntity(m, true)) continue;
                        markerFound = true;
                        ImGui::PushID(m);
                        const std::string markerLabel = std::string("Use start: ") + t.entityelement[m].eleprof.name_s.Get() + " (#" + std::to_string(m) + ")";
                        if (ImGui::MenuItem(markerLabel.c_str())) PWAssignScenePlayer(e, m);
                        ImGui::PopID();
                    }
                    if (!markerFound) ImGui::TextDisabled("Place a player start marker first.");
                    ImGui::EndMenu();
                }
                ImGui::PopID();
            }
            if (!found) ImGui::TextDisabled("No eligible animated characters with default AI.");
            ImGui::EndPopup();
        }
        ImGui::TextDisabled("Save the map after changing player mode.");
    }
    else ImGui::TextWrapped("Finish entity placement or properties editing first. VR setup retains the original workflow.");
}

static void PWRenderSceneHierarchy()
{
    PWEnsureSceneProject();
    PWRenderPlayerSetup();
    ImGui::PushItemWidth(-1.0f);
    ImGui::InputText("##PWSceneSearch", g_pwSceneHierarchySearch,
                     sizeof(g_pwSceneHierarchySearch));
    ImGui::PopItemWidth();
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Filter level objects by name");
    const char* project = g.projectfilename_s.Get();
    ImGui::TextDisabled("Level: %s", project && *project ? project : "Untitled");
    if (ImGui::SmallButton("+ Folder")) PWCreateSceneFolder(0);
    ImGui::SameLine();
    ImGui::TextDisabled("Drag an asset onto a parent to group; move the parent to move the group.");
    ImGui::Separator();

    ImGui::BeginChild("##PWSceneEntries", ImVec2(0, 0), false);
    ImGui::Selectable("Scene root (drop here to unparent)", false);
    PWFolderDragTarget(0);

    const int count = g.entityelementlist;
    std::vector<std::vector<int>> children(count + 1);
    std::vector<int> roots;
    std::vector<bool> displayed(count + 1, false);
    for (int e = 1; e <= count; ++e)
    {
        if (!PWValidSceneEntity(e)) continue;
        const int parent = t.entityelement[e].iHasParentIndex;
        if (parent != e && PWValidSceneEntity(parent))
            children[parent].push_back(e);
        else roots.push_back(e);
    }

    if (*g_pwSceneHierarchySearch)
    {
        int matched = 0;
        for (int e = 1; e <= count; ++e)
        {
            if (!PWValidSceneEntity(e)) continue;
            const char* name = t.entityelement[e].eleprof.name_s.Get();
            if (name && pestrcasestr(name, g_pwSceneHierarchySearch))
            {
                ++matched;
                // Flatten during search so a collapsed parent cannot hide a match.
                ImGui::PushID(e);
                const bool selected = t.widget.pickedEntityIndex == e;
                if (ImGui::Selectable(name, selected)) PWSelectHierarchyEntity(e);
                if (ImGui::BeginDragDropSource())
                {
                    ImGui::SetDragDropPayload("PW_SCENE_ENTITY", &e, sizeof(e));
                    ImGui::TextUnformatted(name);
                    ImGui::EndDragDropSource();
                }
                PWEntityDragTarget(e);
                ImGui::PopID();
            }
        }
        if (!matched) ImGui::TextDisabled("No matching objects in this level");
    }
    else
    {
        std::vector<int> rootFolders;
        for (const auto& f : g_pwFolders) if (f.parent == 0) rootFolders.push_back(f.id);
        for (int id : rootFolders) PWDrawSceneFolder(id, children, displayed, 0);
        for (int e : roots)
            if (!g_pwEntityFolders.count(e) || !PWFindFolder(g_pwEntityFolders[e]))
                PWDrawSceneEntity(e, children, displayed, 0);
        // Guard against malformed old cyclic links; show rather than lose entities.
        for (int e = 1; e <= count; ++e)
            if (PWValidSceneEntity(e) && !displayed[e])
                PWDrawSceneEntity(e, children, displayed, 0);
        if (roots.empty() && g_pwFolders.empty() && count == 0)
            ImGui::TextDisabled("No objects in this level");
    }
    ImGui::EndChild();
}
#endif
