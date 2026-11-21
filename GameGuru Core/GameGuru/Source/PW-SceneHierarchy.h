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
            g_pwFolderNextId = std::max(g_pwFolderNextId, folder.id + 1);
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

static void PWRenderSceneHierarchy()
{
    PWEnsureSceneProject();
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
