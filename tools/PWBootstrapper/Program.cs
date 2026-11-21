using System.Diagnostics;
using System.IO.Compression;
using System.Net.Http.Headers;
using System.Security.Cryptography;
using System.Text;
using System.Drawing;
using System.Drawing.Imaging;
using System.Text.Json;
using System.Windows.Forms;
using System.Runtime.InteropServices;

// Original open launcher code. No proprietary GameGuru binaries or media distributed.
// Compiles the latest PW Edition source locally using user's Visual Studio 2022.
// Only explicitly allowlisted files from a successful build are installed, with backup.
namespace PixelwolfGGCUpdater;

internal static class Program
{
    const string Owner = "BoomForge";
    const string Repo = "GGClassic-PW-Edition-";
    const string Branch = "work/pw-community-integration-20261121";
    private static readonly object OutputLock = new();
    private static readonly Queue<string> RecentBuildOutput = new();
    private static StreamWriter? BuildLog;
    private static string BuildLogPath = "";

    private static void InitLog()
    {
        var folder = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
                                  "Pixelwolf", "GGC-Updater", "Logs");
        Directory.CreateDirectory(folder);
        BuildLogPath = Path.Combine(folder, "update-" + DateTime.Now.ToString("yyyyMMdd-HHmmss-fff") + ".log");
        BuildLog = new StreamWriter(BuildLogPath) { AutoFlush = true };
    }

    private static void LogOutput(string line, bool stderr = false)
    {
        lock (OutputLock)
        {
            if (stderr) Console.Error.WriteLine(line);
            else Console.WriteLine(line);
            BuildLog?.WriteLine(line);
            if (RecentBuildOutput.Count >= 50) RecentBuildOutput.Dequeue();
            RecentBuildOutput.Enqueue(line);
        }
    }

    private static string BuildErrorSummary()
    {
        lock (OutputLock)
            return string.Join(Environment.NewLine, RecentBuildOutput.TakeLast(18));
    }

    static readonly string[] FilesToInstall =
    {
        "Guru-MapEditor.exe",
        "Pixelwolf-GGC-Updater.exe",
        "Launch-PW-Edition.cmd",
        "pw-installed-commit.txt",
        "pw-updater.ps1",
        "pw-edition-version.txt",
        @"Files\scriptbank\global.lua",
        @"Files\scriptbank\pw_transition_state.lua",
        @"Files\effectbank\reloaded\apbr_animwithtran_emissive.fx"
    };

    [STAThread]
    static int Main(string[] args)
    {
        try
        {
            InitLog();
            return Run(args).GetAwaiter().GetResult();
        }
        catch (Exception e)
        {
            Console.Error.WriteLine(e);
            var message = e.Message;
            if (message.Length > 1200) message = message[..1200] + "...";
            if (BuildLogPath.Length > 0)
                message += "\n\nFull log saved to:\n" + BuildLogPath;
            MessageBox.Show(message, "PW Edition updater failed", MessageBoxButtons.OK, MessageBoxIcon.Error);
            return 1;
        }
        finally
        {
            BuildLog?.Dispose();
        }
    }


    // The updater-only GitHub release is public and contains no GameGuru
    // binaries. Future native PW Edition menu updates can refresh BOTH the
    // installer and the editor without manually downloading another installer.
    // The digest is published independently by the updater-only CI workflow.
    static async Task<bool> TrySelfUpdate(string steam)
    {
        const string tag = "pw-updater-dev";
        const string exeName = "Pixelwolf-GGC-Updater.exe";
        const string digestName = "Pixelwolf-GGC-Updater.sha256";
        string currentExe = Environment.ProcessPath ?? throw new IOException("Cannot locate running updater.");
        try
        {
            using var http = new HttpClient { Timeout = TimeSpan.FromMinutes(12) };
            http.DefaultRequestHeaders.UserAgent.Add(new ProductInfoHeaderValue("Pixelwolf-GGC-Updater", "1.0"));
            string api = "https://api.github.com/repos/" + Owner + "/" + Repo + "/releases/tags/" + tag;
            using var releaseResponse = await http.GetAsync(api);
            if (releaseResponse.StatusCode == System.Net.HttpStatusCode.NotFound)
            {
                LogOutput("Updater-only release has not been published yet; continuing with installed updater.");
                return false;
            }
            releaseResponse.EnsureSuccessStatusCode();
            using var document = JsonDocument.Parse(await releaseResponse.Content.ReadAsStringAsync());
            string exeUrl = "", hashUrl = "";
            foreach (var asset in document.RootElement.GetProperty("assets").EnumerateArray())
            {
                string name = asset.GetProperty("name").GetString() ?? "";
                string url = asset.GetProperty("browser_download_url").GetString() ?? "";
                if (name == exeName) exeUrl = url;
                if (name == digestName) hashUrl = url;
            }
            if (exeUrl.Length == 0 || hashUrl.Length == 0)
                throw new IOException("Official release has no paired updater and SHA-256 assets.");
            if (new Uri(exeUrl).Host != "github.com" || new Uri(hashUrl).Host != "github.com")
                throw new IOException("Unrecognised updater download host.");
            string digestText = await http.GetStringAsync(hashUrl);
            string expected = digestText.Split((char[]?)null, StringSplitOptions.RemoveEmptyEntries)[0];
            if (expected.Length != 64 || !expected.All(Uri.IsHexDigit))
                throw new InvalidDataException("Malformed updater SHA-256 manifest.");
            if (string.Equals(Digest(currentExe), expected, StringComparison.OrdinalIgnoreCase))
            {
                LogOutput("Updater executable is already current.");
                return false;
            }

            if (MessageBox.Show(
                "An updated PW Edition updater is available.\n\n" +
                "Update the updater first? It will verify the downloaded EXE, back up this version, " +
                "replace itself only after closing, then resume this engine installation automatically.\n\n" +
                "Only the independently authored updater EXE is downloaded. GameGuru engine builds remain local.",
                "PW Edition - Update Updater", MessageBoxButtons.YesNo, MessageBoxIcon.Information) != DialogResult.Yes)
            {
                LogOutput("Updater self-update declined; continuing with current version.");
                return false;
            }

            string dir = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
                                      "Pixelwolf", "GGC-Updater", "self-update", Guid.NewGuid().ToString("N"));
            Directory.CreateDirectory(dir);
            string staged = Path.Combine(dir, exeName);
            using (var data = await http.GetAsync(exeUrl, HttpCompletionOption.ResponseHeadersRead))
            {
                data.EnsureSuccessStatusCode();
                if (data.Content.Headers.ContentLength > 250L * 1024 * 1024)
                    throw new InvalidDataException("Unexpected updater download size.");
                await using var from = await data.Content.ReadAsStreamAsync();
                await using var to = File.Create(staged);
                await from.CopyToAsync(to);
            }
            if (new FileInfo(staged).Length > 250L * 1024 * 1024 ||
                !string.Equals(Digest(staged), expected, StringComparison.OrdinalIgnoreCase))
                throw new InvalidDataException("Downloaded updater failed SHA-256 verification. Nothing replaced.");

            // PowerShell executes only the local, fixed helper we write here,
            // not any script downloaded from the network.
            string script = Path.Combine(dir, "install-updater.ps1");
            File.WriteAllText(script, """
param(
  [Parameter(Mandatory=$true)][int]$OldProcessId,
  [Parameter(Mandatory=$true)][string]$Target,
  [Parameter(Mandatory=$true)][string]$Staged,
  [Parameter(Mandatory=$true)][string]$Expected,
  [Parameter(Mandatory=$true)][string]$InstallRoot
)
$ErrorActionPreference = 'Stop'
try {
  for ($i = 0; $i -lt 120; $i++) {
    if (-not (Get-Process -Id $OldProcessId -ErrorAction SilentlyContinue)) { break }
    Start-Sleep -Milliseconds 500
  }
  if (Get-Process -Id $OldProcessId -ErrorAction SilentlyContinue) {
    throw 'Existing updater did not close within the safe replacement window.'
  }
  if ((Get-FileHash -LiteralPath $Staged -Algorithm SHA256).Hash -ne $Expected) {
    throw 'Staged updater checksum mismatch.'
  }
  $backupDir = Join-Path $env:USERPROFILE ('PW-Edition-Backups\updater-' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
  New-Item -ItemType Directory -Path $backupDir -Force | Out-Null
  $backup = Join-Path $backupDir 'Pixelwolf-GGC-Updater.exe'
  Copy-Item -LiteralPath $Target -Destination $backup -ErrorAction Stop
  if ((Get-FileHash -LiteralPath $Target -Algorithm SHA256).Hash -ne
      (Get-FileHash -LiteralPath $backup -Algorithm SHA256).Hash) { throw 'Backup verification failed.' }
  $replaced = $false
  try {
    for ($attempt = 0; $attempt -lt 30; $attempt++) {
      try {
        Copy-Item -LiteralPath $Staged -Destination $Target -Force -ErrorAction Stop
        $replaced = $true
        break
      } catch { Start-Sleep -Milliseconds 500 }
    }
    if (-not $replaced -or (Get-FileHash -LiteralPath $Target -Algorithm SHA256).Hash -ne $Expected) {
      throw 'Updater replacement or checksum validation failed.'
    }
  } catch {
    Copy-Item -LiteralPath $backup -Destination $Target -Force
    throw
  }
  $argument = '--install-dir="' + $InstallRoot + '"'
  Start-Process -FilePath $Target -WorkingDirectory (Split-Path -Parent $Target) -ArgumentList $argument
} catch {
  $message = 'Automatic PW Edition updater replacement failed. Original backups retained.' +
             [Environment]::NewLine + $_.Exception.Message
  Add-Type -AssemblyName System.Windows.Forms
  [System.Windows.Forms.MessageBox]::Show($message, 'PW Edition updater') | Out-Null
  exit 1
}
""", Encoding.UTF8);

            var process = new ProcessStartInfo("powershell.exe")
            {
                UseShellExecute = false,
                CreateNoWindow = false,
                WorkingDirectory = dir
            };
            process.ArgumentList.Add("-NoLogo");
            process.ArgumentList.Add("-NoProfile");
            process.ArgumentList.Add("-ExecutionPolicy");
            process.ArgumentList.Add("Bypass");
            process.ArgumentList.Add("-File");
            process.ArgumentList.Add(script);
            process.ArgumentList.Add("-OldProcessId");
            process.ArgumentList.Add(Environment.ProcessId.ToString());
            process.ArgumentList.Add("-Target");
            process.ArgumentList.Add(currentExe);
            process.ArgumentList.Add("-Staged");
            process.ArgumentList.Add(staged);
            process.ArgumentList.Add("-Expected");
            process.ArgumentList.Add(expected);
            process.ArgumentList.Add("-InstallRoot");
            process.ArgumentList.Add(steam);
            if (Process.Start(process) is null)
                throw new IOException("Unable to start verified local updater replacement helper.");
            LogOutput("Verified new updater; helper will replace old EXE and resume installation after exit.");
            return true;
        }
        catch (Exception e)
        {
            LogOutput("Updater self-update check failed safely: " + e.Message, stderr: true);
            MessageBox.Show(
                "Could not complete the updater self-update safely.\n\n" + e.Message +
                "\n\nNo updater files have been overwritten by this check. " +
                "The current updater can still build the engine, or you can retry later.",
                "PW Edition updater", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            return false;
        }
    }

    static async Task<int> Run(string[] args)
    {
        Console.WriteLine("PIXELWOLF GAMEGURU CLASSIC - LOCAL WINDOWS UPDATER");
        Console.WriteLine("Compiles the fork locally. This EXE contains no GameGuru binaries or commercial assets.");
        Console.WriteLine("You need GameGuru Classic from Steam and VS2022 Desktop C++ with x64 SDK/MFC.");
        Console.WriteLine("Your levels, media and Steam GameGuru.exe launcher are never replaced.\n");

        string? requested = args.FirstOrDefault(a => a.StartsWith("--install-dir=", StringComparison.OrdinalIgnoreCase));
        string? selected = requested is null ? null : requested["--install-dir=".Length..].Trim('"');
        if (selected is null)
        {
            using var picker = new FolderBrowserDialog
            {
                Description = "Select Steam GameGuru Classic folder (containing Files and GameGuru.exe)",
                UseDescriptionForTitle = true,
                ShowNewFolderButton = false
            };
            if (picker.ShowDialog() != DialogResult.OK) return 0;
            selected = picker.SelectedPath;
        }
        string steam = Path.GetFullPath(selected).TrimEnd(Path.DirectorySeparatorChar);
        ValidateSteam(steam);
        LogOutput("Selected GameGuru installation: " + steam);
        LogOutput("Target editor (not the Steam launcher): " + Path.Combine(steam, "Guru-MapEditor.exe"));

        // Check the independent updater channel before building the C++ editor.
        // If a newer updater is available, launch its verified replacement helper
        // and resume the install after this process exits.
        if (await TrySelfUpdate(steam)) return 0;

        string vswhere = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86),
                                      @"Microsoft Visual Studio\Installer\vswhere.exe");
        // A plain Visual Studio/VS Code install isn't enough: the GameGuru
        // solution needs the VS2022 v143 x64 compiler, MSBuild and C++ MFC.
        // vswhere ships with the free Microsoft Build Tools installation.
        string vsroot = "";
        if (File.Exists(vswhere))
        {
            vsroot = (await Capture(vswhere,
                "-latest -products * -version \"[17.0,18.0)\" -requires Microsoft.Component.MSBuild Microsoft.VisualStudio.Component.VC.Tools.x86.x64 Microsoft.VisualStudio.Component.VC.ATLMFC -property installationPath",
                null)).Trim();
        }
        if (vsroot.Length == 0)
        {
            await OfferBuildToolsInstallation();
            return 0; // No Steam changes. User reruns after Microsoft setup finishes.
        }
        string msbuild = Path.Combine(vsroot.Split('\n', StringSplitOptions.RemoveEmptyEntries).Last().Trim(),
                                     @"MSBuild\Current\Bin\MSBuild.exe");
        if (!File.Exists(msbuild))
        {
            await OfferBuildToolsInstallation();
            return 0;
        }

        if (MessageBox.Show(
            "Download the latest PW Edition GitHub source and build it on this computer?\n\n" +
            "No Steam files will change until compilation completes and you separately confirm installation.\n\n" +
            "Every replaced file will be backed up outside Steam. This is an experimental engine build.\n\nContinue?",
            "Pixelwolf PW Edition", MessageBoxButtons.YesNo, MessageBoxIcon.Warning) != DialogResult.Yes) return 0;

        using var http = new HttpClient { Timeout = TimeSpan.FromMinutes(30) };
        http.DefaultRequestHeaders.UserAgent.Add(new ProductInfoHeaderValue("Pixelwolf-GGC-Updater", "0.1"));
        string api = $"https://api.github.com/repos/{Owner}/{Repo}/commits/{Branch}";
        Console.WriteLine("Looking up branch HEAD...");
        using var data = JsonDocument.Parse(await http.GetStringAsync(api));
        string sha = data.RootElement.GetProperty("sha").GetString()!;
        if (sha.Length != 40 || !sha.All(Uri.IsHexDigit))
            throw new InvalidDataException("Unexpected source commit ID.");
        Console.WriteLine("Pinned source: " + sha);
        // Keep GameGuru's deeply nested C++ and legacy DirectX SDK paths well
        // below MAX_PATH; the previous GUID + long GitHub ZIP root caused
        // PowerShell 5.1 Expand-Archive failures on some Windows machines.
        string temp = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
                                   "PW", "b-" + Guid.NewGuid().ToString("N")[..8]);
        Directory.CreateDirectory(temp);
        try
        {
            string sourceZip = Path.Combine(temp, "github.zip");
            string url = $"https://codeload.github.com/{Owner}/{Repo}/zip/{sha}";
            Console.WriteLine("Downloading source archive from " + url);
            using (var response = await http.GetAsync(url, HttpCompletionOption.ResponseHeadersRead))
            {
                response.EnsureSuccessStatusCode();
                await using var file = File.Create(sourceZip);
                await response.Content.CopyToAsync(file);
            }
            string extracted = Path.Combine(temp, "source");
            ZipFile.ExtractToDirectory(sourceZip, extracted);
            string[] roots = Directory.GetDirectories(extracted);
            if (roots.Length != 1) throw new InvalidDataException("Unexpected GitHub ZIP layout.");
            string root = Path.Combine(temp, "src");
            Directory.Move(roots[0], root);
            Directory.Delete(extracted);
            if (!File.Exists(Path.Combine(root, @"GameGuru Core\GameGuru.sln")) ||
                !File.Exists(Path.Combine(root, @"tools\pw-prepare-d3dx.ps1")))
                throw new InvalidDataException("GitHub archive is missing the expected build scripts.");

            Console.WriteLine("Preparing Microsoft DirectX and Steamworks import libraries...");
            await Execute("powershell.exe", "-NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File " + Q(Path.Combine(root, @"tools\pw-prepare-d3dx.ps1")), root);
            await Execute("powershell.exe", "-NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File " + Q(Path.Combine(root, @"tools\pw-prepare-steam-imports.ps1")), root);
            foreach (string project in new []
            {
                @"GameGuru Core\SDK\DirectXTex\DirectXTex\DirectXTex_Desktop_2022.vcxproj",
                @"GameGuru Core\SDK\DirectX\Effect11\Effects11_2022.vcxproj",
                @"GameGuru Core\SDK\OGG\ogg_vorbis_static.sln",
                @"GameGuru Core\GameGuru.sln"
            })
            {
                Console.WriteLine("\nBuilding: " + project);
                await Execute(msbuild, Q(Path.Combine(root, project)) + " /t:Build /p:Configuration=Release /p:Platform=x64 /m:2 /nologo /v:minimal", root);
            }
            string built = Path.Combine(root, @"PW-Build\Release");
            // The C++ compiler can report success while staging an older or wrong
            // editor target. Never install unless the PW-branded editor marker
            // is present in the compiled executable itself.
            string engine = Path.Combine(built, "Guru-MapEditor.exe");
            if (!File.Exists(engine) || !HasCompiledPWBrand(engine))
                throw new InvalidDataException("The compiled editor lacks the PW Edition branding marker; refusing to replace Steam files. See updater log.");

            // This bootstrapper is our own independently authored binary. Copy it
            // alongside the engine so Help > PW Edition Updates can launch it.
            string currentUpdater = Environment.ProcessPath ?? throw new IOException("Cannot identify the updater executable.");
            File.Copy(currentUpdater, Path.Combine(built, "Pixelwolf-GGC-Updater.exe"), overwrite: true);
            File.WriteAllText(Path.Combine(built, "Launch-PW-Edition.cmd"),
                "@echo off\r\ncd /d \"%~dp0\"\r\nstart \"\" \"%~dp0Guru-MapEditor.exe\"\r\n", Encoding.ASCII);
            File.WriteAllText(Path.Combine(built, "pw-installed-commit.txt"), sha + Environment.NewLine, Encoding.ASCII);
            // Classic Steam installations may not include the newer ImGui UI
            // images. Draw neutral, original fallback icons only for missing
            // filenames. Never overwrite a paid or existing image.
            var installFiles = FilesToInstall.ToList();
            AddMissingEditorIconFallbacks(built, steam, installFiles);
            // Copy original Classic 42px toolbar artwork out of the OWNER'S
            // installed GameGuru.exe resources (never shipped over the network).
            // This supersedes our temporary, blue PW pictograms wherever an
            // authentic matching Classic resource is present.
            RestoreNativeClassicToolbar(built, steam, installFiles);
            foreach (var relative in installFiles)
                if (!File.Exists(Path.Combine(built, relative)))
                    throw new FileNotFoundException("Required PW build output missing; nothing installed: " + relative);
            LogOutput("\nFull x64 build complete. PW-branded editor verified. Files ready:");
            foreach (var relative in installFiles) LogOutput("  " + relative);

            ValidateSteam(steam);
            if (Process.GetProcessesByName("GameGuru").Length > 0 ||
                Process.GetProcessesByName("Guru-MapEditor").Length > 0)
                throw new InvalidOperationException("Close GameGuru Classic and its editor before installing. Steam has not been modified.");
            if (MessageBox.Show(
                "Build passed at commit " + sha[..10] + ".\n\n" +
                "Install the verified PW-branded editor, updater and supporting files into:\n" + steam + "\n\n" +
                "The original Steam GameGuru.exe will remain untouched.\n\n" +
                "Original versions will be backed up outside Steam. Your assets and original launcher are preserved.\n\nInstall now?",
                "Confirm local Steam update", MessageBoxButtons.YesNo, MessageBoxIcon.Warning) != DialogResult.Yes)
            {
                Console.WriteLine("Cancelled before installation. Steam is unchanged.");
                return 0;
            }
            string backup = InstallWithRollback(built, steam, sha, installFiles);
            var installed = Path.Combine(steam, "Guru-MapEditor.exe");
            if (Digest(engine) != Digest(installed))
                throw new IOException("Post-install editor SHA-256 mismatch. Check backup " + backup);
            LogOutput("\nSUCCESS. PW Edition verified at " + installed + "\nCommit: " + sha + "\nBackup: " + backup);
            if (MessageBox.Show(
                "PW Edition installed and SHA-256 verified from commit " + sha[..10] + ".\n\n" +
                "Editor: " + installed + "\n\n" +
                "Steam's normal Play button may still start stock GameGuru. Use the new Launch-PW-Edition.cmd or the editor EXE directly.\n\n" +
                "Launch the verified PW Edition editor now?",
                "Pixelwolf PW Edition installed", MessageBoxButtons.YesNo, MessageBoxIcon.Information) == DialogResult.Yes)
            {
                Process.Start(new ProcessStartInfo(installed) {
                    UseShellExecute = true,
                    WorkingDirectory = steam
                });
            }
            return 0;
        }
        finally
        {
            try { Directory.Delete(temp, recursive: true); }
            catch (Exception e) { Console.WriteLine("Temporary source cleanup failed: " + temp + " (" + e.Message + ")"); }
        }
    }

    static async Task OfferBuildToolsInstallation()
    {
        // Official Microsoft Build Tools 2022 bootstrapper, not bundled or
        // redistributed by PW Edition. Users approve installation separately.
        const string microsoftSetup = "https://aka.ms/vs/17/release/vs_buildtools.exe";
        const string microsoftDocs = "https://learn.microsoft.com/en-us/visualstudio/install/workload-component-id-vs-build-tools?view=vs-2022";
        var response = MessageBox.Show(
            "The Visual Studio 2022 C++ compiler/build tools are missing or incomplete.\n\n" +
            "PW Edition currently compiles your licensed GameGuru source locally, so it needs Microsoft's FREE Build Tools (not the full Visual Studio editor).\n\n" +
            "Download and open the official Microsoft installer now?\n" +
            "The C++ workload, MFC and Windows SDK will be preselected for review.\n\n" +
            "Setup needs internet, several GB of disk space and may ask for administrator permission. " +
            "When Microsoft's installation finishes, restart this updater.\n\n" +
            "No GameGuru or Steam files have been changed.",
            "Pixelwolf PW Edition - Set up build tools", MessageBoxButtons.YesNo, MessageBoxIcon.Information);
        if (response != DialogResult.Yes)
        {
            Console.WriteLine("Build tools installation declined. See " + microsoftDocs);
            return;
        }
        string folder = Path.Combine(Path.GetTempPath(), "Pixelwolf-GGC-Microsoft-Tools");
        Directory.CreateDirectory(folder);
        string installer = Path.Combine(folder, "vs_buildtools.exe");
        using var http = new HttpClient { Timeout = TimeSpan.FromMinutes(5) };
        Console.WriteLine("Downloading official Microsoft Visual Studio 2022 Build Tools installer...");
        using (var reply = await http.GetAsync(microsoftSetup, HttpCompletionOption.ResponseHeadersRead))
        {
            reply.EnsureSuccessStatusCode();
            await using var destination = File.Create(installer);
            await reply.Content.CopyToAsync(destination);
        }
        if (new FileInfo(installer).Length < 100_000)
            throw new InvalidDataException("Microsoft Build Tools download was unexpectedly small. Open " + microsoftSetup + " manually.");
        Console.WriteLine("Launching Microsoft setup with C++ x64 tools, MFC and Windows SDK selected.");
        Process.Start(new ProcessStartInfo
        {
            FileName = installer,
            Arguments = "--add Microsoft.VisualStudio.Workload.VCTools --includeRecommended " +
                        "--add Microsoft.VisualStudio.Component.VC.ATLMFC " +
                        "--add Microsoft.VisualStudio.Component.Windows11SDK.26100",
            UseShellExecute = true
        });
        MessageBox.Show(
            "Microsoft setup has been opened. Complete its installation, then relaunch the PW Edition updater.\n\n" +
            "You don't need Visual Studio IDE, just the Build Tools.\n\n" +
            "Steam files remain unchanged.",
            "Pixelwolf PW Edition", MessageBoxButtons.OK, MessageBoxIcon.Information);
    }

    // Fresh PW UI ships as source. Original simple fallback graphics are
    // rendered locally at install time, so proprietary stock/Newer GameGuru
    // interface packs are never downloaded, redistributed or overwritten.
    static readonly string[] EditorIcons =
    {
        "ABOUT-Country.png", "ABOUT-Logo.png", "ABOUT-TGC.png",
        "blendmode.png", "builder.png", "ccp.png", "circle.png",
        "circlem.png", "circles.png", "dotcircle.png", "dotcirclem.png",
        "dotcircles.png", "drawwaypoints.png", "ebe-block.png",
        "ebe-column.png", "ebe-control1.png", "ebe-control2.png",
        "ebe-cube.png", "ebe-floor.png", "ebe-new.png", "ebe-row.png",
        "ebe-stairs.png", "ebe-wall.png", "entity.png", "import.png",
        "level.png", "loadlevel.png", "markers.png", "media-pause.png",
        "media-play.png", "media-record.png", "media-recording.png",
        "media-recordprocessing.png", "media-refresh.png",
        "newlevel.png", "newwaypoints.png", "paintgrass.png",
        "painttexture.png", "pointer2.png", "pointer3.png", "rampmode.png",
        "rounding_overlay_style0-h.png", "savelevel.png", "shape-down.png",
        "shape-up.png", "shape.png", "socialvr.png", "storedlevel.png",
        "testgame.png", "theader.png", "vrmode.png", "waypoints.png"
    };

    // The first installer drew the file names over solid coloured tiles. Those
    // were temporary diagnostics, not usable toolbar art. Replace ONLY those
    // recognizable tiles; preserve all existing stock/custom icons unchanged.
    static bool IsLegacyPWTextTile(string path, bool wide)
    {
        try
        {
            using var bitmap = new Bitmap(path);
            int w = wide ? 256 : 64, h = wide ? 96 : 64;
            if (bitmap.Width != w || bitmap.Height != h) return false;
            Color bg = Color.FromArgb(34, 40, 58);
            Color rim = Color.FromArgb(91, 118, 185);
            // GDI+ tile produced by the former updater: full opaque bg and
            // 3-pixel blue rectangle inset by two pixels on each edge.
            // Never replace arbitrary files solely because their names match.
            return bitmap.GetPixel(0, 0).ToArgb() == bg.ToArgb() &&
                   bitmap.GetPixel(w - 1, 0).ToArgb() == bg.ToArgb() &&
                   bitmap.GetPixel(0, h - 1).ToArgb() == bg.ToArgb() &&
                   bitmap.GetPixel(w - 1, h - 1).ToArgb() == bg.ToArgb() &&
                   bitmap.GetPixel(2, 2).ToArgb() == rim.ToArgb() &&
                   bitmap.GetPixel(w - 3, h - 3).ToArgb() == rim.ToArgb();
        }
        catch (Exception) { return false; }
    }

    // Original vector pictograms rendered locally, not downloaded copyrighted
    // GameGuru icon resources. Symbols remain visual; names belong in tooltips.
    static void DrawPWIcon(string filename, string target, bool wide)
    {
        int w = wide ? 256 : 64, h = wide ? 96 : 64;
        using var bmp = new Bitmap(w, h);
        using var g = Graphics.FromImage(bmp);
        g.SmoothingMode = System.Drawing.Drawing2D.SmoothingMode.AntiAlias;
        g.Clear(Color.Transparent);
        using var bg = new SolidBrush(Color.FromArgb(235, 38, 40, 43));
        using var light = new SolidBrush(Color.FromArgb(245, 232, 240, 255));
        using var accent = new SolidBrush(Color.FromArgb(255, 180, 185, 190));
        using var warm = new SolidBrush(Color.FromArgb(255, 210, 205, 190));
        using var cool = new Pen(Color.FromArgb(245, 232, 240, 255), 4f);
        using var blue = new Pen(Color.FromArgb(255, 108, 194, 235), 4f);
        cool.StartCap = cool.EndCap = System.Drawing.Drawing2D.LineCap.Round;
        cool.LineJoin = System.Drawing.Drawing2D.LineJoin.Round;
        blue.StartCap = blue.EndCap = System.Drawing.Drawing2D.LineCap.Round;
        blue.LineJoin = System.Drawing.Drawing2D.LineJoin.Round;
        if (wide)
        {
            g.FillRectangle(bg, 2, 3, w - 4, h - 6);
            g.DrawRectangle(blue, 3, 4, w - 7, h - 9);
            using var title = new Font("Segoe UI", 25, FontStyle.Bold, GraphicsUnit.Pixel);
            using var subtitle = new Font("Segoe UI", 15, FontStyle.Regular, GraphicsUnit.Pixel);
            using var format = new StringFormat { Alignment = StringAlignment.Center, LineAlignment = StringAlignment.Center };
            g.DrawString("PIXELWOLF", title, light, new RectangleF(0, 19, w, 37), format);
            g.DrawString("PW EDITION", subtitle, accent, new RectangleF(0, 52, w, 26), format);
            bmp.Save(target, ImageFormat.Png);
            return;
        }
        g.FillEllipse(bg, 4, 4, 56, 56);
        g.DrawEllipse(blue, 4, 4, 56, 56);
        void Line(float x1, float y1, float x2, float y2) => g.DrawLine(cool, x1, y1, x2, y2);
        void Rect(float x, float y, float width, float height) => g.DrawRectangle(cool, x, y, width, height);
        void Ell(float x, float y, float width, float height) => g.DrawEllipse(cool, x, y, width, height);
        void Polygon(params PointF[] pts) => g.DrawPolygon(cool, pts);
        PointF P(float x, float y) => new PointF(x, y);
        string key = Path.GetFileNameWithoutExtension(filename).ToLowerInvariant();
        switch (key)
        {
            case "loadlevel":
            case "storedlevel":
            case "loadlevel2":
                Polygon(P(12, 26), P(12, 21), P(29, 21), P(34, 26), P(51, 26), P(49, 47), P(14, 47));
                Line(32, 14, 32, 36); Line(25, 29, 32, 36); Line(39, 29, 32, 36);
                break;
            case "savelevel":
                Rect(16, 13, 32, 38); Rect(22, 14, 20, 12); Rect(23, 34, 18, 17);
                break;
            case "newlevel":
            case "ebe-new":
                Rect(14, 14, 30, 35); Line(29, 26, 29, 42); Line(21, 34, 37, 34);
                break;
            case "testgame":
            case "media-play":
                g.FillPolygon(accent, new[] { P(24, 16), P(47, 32), P(24, 48) });
                break;
            case "media-pause":
                g.FillRectangle(light, 21, 17, 8, 30); g.FillRectangle(light, 35, 17, 8, 30);
                break;
            case "media-record":
            case "media-recording":
            case "media-recordprocessing":
                g.FillEllipse(warm, 21, 21, 22, 22); Ell(17, 17, 30, 30);
                break;
            case "media-refresh":
                g.DrawArc(cool, 17, 17, 30, 30, 40, 270); Line(48, 25, 47, 38); Line(47, 38, 34, 36);
                break;
            case "pointer2":
            case "pointer3":
                g.FillPolygon(light, new[] { P(19, 12), P(46, 32), P(33, 34), P(28, 49) });
                break;
            case "entity":
            case "ebe-cube":
            case "ebe-block":
            case "builder":
                Polygon(P(32, 11), P(49, 21), P(49, 43), P(32, 53), P(15, 43), P(15, 21));
                Line(15, 21, 32, 31); Line(49, 21, 32, 31); Line(32, 31, 32, 53);
                break;
            case "import":
                Rect(14, 35, 36, 13); Line(32, 12, 32, 38); Line(23, 29, 32, 38); Line(41, 29, 32, 38);
                break;
            case "shape":
            case "level":
            case "levelmode":
            case "sculpt":
                Polygon(P(12, 47), P(24, 24), P(31, 34), P(38, 16), P(52, 47));
                break;
            case "rampmode":
                Polygon(P(12, 48), P(51, 48), P(51, 20));
                break;
            case "blendmode":
                Ell(14, 16, 27, 27); Ell(26, 23, 27, 27);
                break;
            case "painttexture":
            case "paintgrass":
                if (key == "paintgrass") {
                    Line(22, 47, 18, 31); Line(29, 47, 31, 18); Line(39, 47, 46, 26);
                    Line(17, 40, 13, 36); Line(43, 37, 50, 34);
                } else {
                    Line(43, 16, 25, 39); Polygon(P(20, 37), P(29, 42), P(22, 50), P(14, 50));
                    Line(44, 13, 49, 18);
                }
                break;
            case "markers":
            case "dotcircle":
            case "dotcirclem":
            case "dotcircles":
                Ell(17, 13, 30, 30); g.FillEllipse(warm, 27, 23, 10, 10);
                Line(32, 43, 32, 53);
                break;
            case "waypoints":
            case "newwaypoints":
            case "drawwaypoints":
                g.FillEllipse(accent, 13, 34, 9, 9); g.FillEllipse(accent, 28, 15, 9, 9);
                g.FillEllipse(accent, 43, 37, 9, 9);
                Line(22, 36, 30, 23); Line(36, 23, 44, 38);
                break;
            case "ccp":
            case "socialvr":
                Ell(26, 13, 12, 13); g.DrawArc(cool, 18, 31, 28, 22, 185, 170);
                break;
            case "ebe-stairs":
            case "ebe-row":
            case "ebe-floor":
            case "ebe-column":
            case "ebe-wall":
                for (int step = 0; step < 3; step++) {
                    int xx = 14 + step * 12, yy = key == "ebe-stairs" ? 36 - step * 10 : 30;
                    Rect(xx, yy, 12, key == "ebe-column" ? 24 : 11);
                }
                break;
            case "circle":
            case "circlem":
            case "circles":
            case "rounding_overlay_style0-h":
                Ell(14, 14, 36, 36); Ell(23, 23, 18, 18);
                break;
            case "shape-up":
                Polygon(P(15, 42), P(32, 21), P(49, 42));
                break;
            case "shape-down":
                Polygon(P(15, 22), P(32, 43), P(49, 22));
                break;
            case "media-maximize":
                Rect(15, 15, 34, 34);
                break;
            case "media-minimize":
                Line(18, 43, 47, 43);
                break;
            case "media-recording2":
                g.FillEllipse(warm, 21, 21, 22, 22);
                break;
            default:
                // A recognizable tool/cog pictogram is preferable to a
                // miniature filename even for unclassified legacy buttons.
                Ell(20, 20, 24, 24); g.FillEllipse(accent, 28, 28, 8, 8);
                for (int n = 0; n < 8; ++n) {
                    double a = n * Math.PI / 4;
                    float x = (float)Math.Cos(a), y = (float)Math.Sin(a);
                    Line(32 + 14 * x, 32 + 14 * y, 32 + 20 * x, 32 + 20 * y);
                }
                break;
        }
        bmp.Save(target, ImageFormat.Png);
    }

    static void AddMissingEditorIconFallbacks(string build, string steam, List<string> installFiles)
    {
        int generated = 0, upgraded = 0;
        foreach (var filename in EditorIcons)
        {
            string relative = Path.Combine("Files", "editors", "uiv3", filename);
            string installed = Path.Combine(steam, relative);
            bool wide = filename.StartsWith("ABOUT-", StringComparison.OrdinalIgnoreCase) ||
                        filename.Equals("theader.png", StringComparison.OrdinalIgnoreCase);
            bool hadOldTextTile = File.Exists(installed) && IsLegacyPWTextTile(installed, wide);
            if (File.Exists(installed) && !hadOldTextTile) continue;
            string target = Path.Combine(build, relative);
            Directory.CreateDirectory(Path.GetDirectoryName(target)!);
            DrawPWIcon(filename, target, wide);
            installFiles.Add(relative);
            if (hadOldTextTile) upgraded++; else generated++;
        }
        if (generated + upgraded > 0)
            LogOutput("Restored " + generated + " missing pictographic editor icons and upgraded " +
                      upgraded + " prior PW text-only placeholders. Existing stock/custom art preserved.");
    }


    // The GameGuru Classic IDE resources are defined in GameGuru IDE/Editor.rc:
    // StandardToolbar2 (152), ModeToolbar3 (178), TerrainToolbar (180),
    // WaypointToolbar (182), GameToolbar (186). Each native bitmap is a row
    // of 42x42 icons in the order declared in the original toolbar resource.
    // Source images remain inside the user's licensed Steam copy.
    [DllImport("kernel32.dll", EntryPoint = "LoadLibraryExW", CharSet = CharSet.Unicode, SetLastError = true)]
    static extern IntPtr LoadResourceModule(string fileName, IntPtr file, uint flags);

    [DllImport("kernel32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    static extern bool FreeLibrary(IntPtr module);

    [DllImport("user32.dll", EntryPoint = "LoadImageW", SetLastError = true)]
    static extern IntPtr LoadOriginalBitmap(IntPtr module, IntPtr id, uint imageType, int width, int height, uint flags);

    [DllImport("gdi32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    static extern bool DeleteOriginalBitmap(IntPtr bitmap);

    static bool IsGeneratedPWIcon(string path)
    {
        try
        {
            using var bmp = new Bitmap(path);
            if (IsLegacyPWTextTile(path, bmp.Width != 64)) return true;
            if (bmp.Width != 64 || bmp.Height != 64) return false;
            Color t = bmp.GetPixel(0, 0);
            Color a = bmp.GetPixel(32, 4);
            Color b = bmp.GetPixel(4, 32);
            static bool IsCyanRing(Color c) =>
                c.A >= 150 && Math.Abs(c.R - 108) < 25 &&
                Math.Abs(c.G - 194) < 25 && Math.Abs(c.B - 235) < 25;
            return t.A == 0 && IsCyanRing(a) && IsCyanRing(b);
        }
        catch { return false; }
    }

    static void RestoreNativeClassicToolbar(string build, string steam, List<string> installFiles)
    {
        string stockLauncher = Path.Combine(steam, "GameGuru.exe");
        if (!File.Exists(stockLauncher))
        {
            LogOutput("Original GameGuru.exe unavailable; cannot recover authentic Classic toolbar art.");
            return;
        }
        // LOAD_LIBRARY_AS_DATAFILE prevents running the executable or loading
        // Steam dependencies. RT_BITMAP resources are read-only.
        IntPtr module = LoadResourceModule(stockLauncher, IntPtr.Zero, 0x00000002);
        if (module == IntPtr.Zero)
        {
            LogOutput("Could not read stock toolbar bitmap resources from " + stockLauncher);
            return;
        }
        // Names and column indexes follow GameGuru IDE/Editor.rc, 42px bitmaps.
        var strips = new (int Resource, string[] Names)[]
        {
            (152, new[] { "newlevel.png", "loadlevel.png", "savelevel.png" }),
            (178, new[] { "entity.png", "markers.png", "builder.png" }),
            (180, new[] { "shape.png", "level.png", "storedlevel.png", "blendmode.png", "rampmode.png", "painttexture.png", "paintgrass.png" }),
            (182, new[] { "newwaypoints.png", "waypoints.png" }),
            (186, new[] { "testgame.png" })
        };
        int recovered = 0, missing = 0;
        try
        {
            foreach (var (resource, names) in strips)
            {
                IntPtr hBitmap = LoadOriginalBitmap(module, (IntPtr)resource, 0, 0, 0, 0x2000);
                if (hBitmap == IntPtr.Zero)
                {
                    LogOutput("Classic toolbar RT_BITMAP " + resource + " not present; preserving existing assets.");
                    missing += names.Length;
                    continue;
                }
                try
                {
                    using var strip = Bitmap.FromHbitmap(hBitmap);
                    for (int col = 0; col < names.Length; col++)
                    {
                        string relative = Path.Combine("Files", "editors", "uiv3", names[col]);
                        string existing = Path.Combine(steam, relative);
                        if (File.Exists(existing) && !IsGeneratedPWIcon(existing)) continue;
                        if (strip.Width < (col + 1) * 42 || strip.Height < 42)
                        {
                            LogOutput("Unexpected strip dimensions for RT_BITMAP " + resource + "; preserving " + names[col]);
                            missing++;
                            continue;
                        }
                        string path = Path.Combine(build, relative);
                        Directory.CreateDirectory(Path.GetDirectoryName(path)!);
                        using var icon = new Bitmap(64, 64);
                        using (var graphics = Graphics.FromImage(icon))
                        {
                            graphics.Clear(Color.Transparent);
                            graphics.InterpolationMode = System.Drawing.Drawing2D.InterpolationMode.HighQualityBicubic;
                            graphics.DrawImage(strip, new Rectangle(0, 0, 64, 64),
                                new Rectangle(col * 42, 0, 42, 42), GraphicsUnit.Pixel);
                        }
                        icon.Save(path, ImageFormat.Png);
                        if (!installFiles.Contains(relative, StringComparer.OrdinalIgnoreCase))
                            installFiles.Add(relative);
                        recovered++;
                    }
                }
                finally { DeleteOriginalBitmap(hBitmap); }
            }
        }
        catch (Exception e)
        {
            // Do not install a partially extracted mismatched icon strip:
            // preserve stock files, keep explicit log for user troubleshooting.
            LogOutput("Original Classic resource restoration reported: " + e.Message, stderr: true);
        }
        finally { FreeLibrary(module); }
        LogOutput("Recovered " + recovered + " authentic Classic toolbar icons from licensed GameGuru.exe; missing/unsupported: " + missing + ".");
    }

    static bool HasCompiledPWBrand(string exe)
    {
        // The exact ASCII text used by M-GridEdit.cpp for the editor window title.
        byte[] marker = Encoding.ASCII.GetBytes("GameGuru Classic - PW Edition | ");
        using var stream = File.OpenRead(exe);
        // Read-only inspection of the editor image; this doesn't execute code.
        byte[] data = new byte[stream.Length];
        stream.ReadExactly(data);
        return data.AsSpan().IndexOf(marker) >= 0;
    }

    static string InstallWithRollback(string build, string steam, string sha, IReadOnlyList<string> installFiles)
    {
        string backup = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.UserProfile),
            "PW-Edition-Backups", DateTime.Now.ToString("yyyyMMdd-HHmmss-fff"));
        if (backup.StartsWith(steam + Path.DirectorySeparatorChar, StringComparison.OrdinalIgnoreCase))
            throw new InvalidOperationException("Backup folder would be inside the Steam installation.");
        Directory.CreateDirectory(backup);
        var files = new List<(string name, bool existed)>();
        var written = new List<(string name, bool existed)>();
        try
        {
            // Back up ALL originals first; stop before changing anything if one backup fails.
            foreach (string relative in installFiles)
            {
                string target = Path.Combine(steam, relative);
                string? folder = Path.GetDirectoryName(target);
                while (folder is not null && !string.Equals(folder, steam, StringComparison.OrdinalIgnoreCase))
                {
                    if (Directory.Exists(folder) && (File.GetAttributes(folder) & FileAttributes.ReparsePoint) != 0)
                        throw new IOException("Refusing linked destination folder: " + folder);
                    folder = Path.GetDirectoryName(folder);
                }
                if (File.Exists(target) && (File.GetAttributes(target) & FileAttributes.ReparsePoint) != 0)
                    throw new IOException("Refusing linked destination file: " + target);
                if (Directory.Exists(target)) throw new IOException("Cannot replace directory: " + target);
                bool exists = File.Exists(target);
                if (exists)
                {
                    string saved = Path.Combine(backup, relative);
                    Directory.CreateDirectory(Path.GetDirectoryName(saved)!);
                    File.Copy(target, saved);
                    if (Digest(saved) != Digest(target))
                        throw new IOException("Backup verification failed: " + relative);
                }
                files.Add((relative, exists));
            }
            File.WriteAllText(Path.Combine(backup, "manifest.json"), JsonSerializer.Serialize(new
            {
                repo = Owner + "/" + Repo, sourceCommit = sha, steamFolder = steam,
                savedAt = DateTimeOffset.Now,
                originals = files.Select(x => new { path = x.name, existed = x.existed })
            }, new JsonSerializerOptions { WriteIndented = true }));

            foreach (var item in files)
            {
                string target = Path.Combine(steam, item.name);
                Directory.CreateDirectory(Path.GetDirectoryName(target)!);
                string source = Path.Combine(build, item.name);
                // Windows refuses to overwrite an executable that is currently
                // running from the selected installation. In that case we are
                // already running precisely this updater binary, so leave it.
                if (item.name.Equals("Pixelwolf-GGC-Updater.exe", StringComparison.OrdinalIgnoreCase) &&
                    string.Equals(Path.GetFullPath(Environment.ProcessPath ?? ""), Path.GetFullPath(target),
                                  StringComparison.OrdinalIgnoreCase))
                {
                    if (Digest(source) != Digest(target)) throw new IOException("Running updater differs from staged updater.");
                    LogOutput("Keeping current in-use updater: " + item.name);
                    continue;
                }
                string staged = target + ".pw-tmp-" + Guid.NewGuid().ToString("N");
                try
                {
                    File.Copy(source, staged);
                    if (Digest(source) != Digest(staged))
                        throw new IOException("Stage verification failed: " + item.name);
                    written.Add(item); // Record before mutating, so rollback covers failed moves.
                    File.Move(staged, target, overwrite: true);
                    if (Digest(source) != Digest(target))
                        throw new IOException("Installed checksum mismatch: " + item.name);
                    Console.WriteLine("Installed: " + item.name);
                }
                finally { if (File.Exists(staged)) File.Delete(staged); }
            }
            File.WriteAllText(Path.Combine(backup, "INSTALL-SUCCESS.txt"),
                "Installed Pixelwolf GGC at " + DateTimeOffset.Now + "\nCommit " + sha + "\n");
            return backup;
        }
        catch
        {
            Console.Error.WriteLine("Install failed - attempting automatic rollback...");
            foreach (var item in written.AsEnumerable().Reverse())
            {
                string target = Path.Combine(steam, item.name);
                try
                {
                    if (item.existed) File.Copy(Path.Combine(backup, item.name), target, true);
                    else if (File.Exists(target)) File.Delete(target);
                    Console.WriteLine("Restored: " + item.name);
                }
                catch (Exception e) { Console.Error.WriteLine("MANUAL RESTORE NEEDED: " + item.name + " - " + e.Message); }
            }
            Console.Error.WriteLine("Backups: " + backup);
            throw;
        }
    }

    static void ValidateSteam(string root)
    {
        if (!Directory.Exists(Path.Combine(root, "Files")) ||
            (!File.Exists(Path.Combine(root, "GameGuru.exe")) &&
             !File.Exists(Path.Combine(root, "Guru-MapEditor.exe"))))
            throw new InvalidOperationException("Select the actual Steam GameGuru Classic install root containing Files and GameGuru.exe. No changes made.");
    }

    static string Q(string value) => "\"" + value.Replace("\"", "\\\"") + "\"";
    static string Digest(string path)
    {
        using var file = File.OpenRead(path);
        return Convert.ToHexString(SHA256.HashData(file));
    }

    static async Task<string> Capture(string command, string args, string? dir)
    {
        using var p = new Process { StartInfo = new ProcessStartInfo(command, args) {
            UseShellExecute = false, CreateNoWindow = true, RedirectStandardOutput = true,
            RedirectStandardError = true, WorkingDirectory = dir ?? Environment.CurrentDirectory
        }};
        p.Start();
        var stdout = p.StandardOutput.ReadToEndAsync();
        var stderr = p.StandardError.ReadToEndAsync();
        await p.WaitForExitAsync();
        string output = await stdout, error = await stderr;
        if (p.ExitCode != 0) throw new IOException(command + " failed: " + error);
        return output;
    }

    static async Task Execute(string command, string args, string dir)
    {
        using var p = new Process { StartInfo = new ProcessStartInfo(command, args) {
            UseShellExecute = false, CreateNoWindow = true, RedirectStandardOutput = true,
            RedirectStandardError = true, WorkingDirectory = dir
        }};
        LogOutput("RUN: " + command + " " + args);
        p.OutputDataReceived += (_, e) => { if (e.Data is not null) LogOutput(e.Data); };
        p.ErrorDataReceived += (_, e) => { if (e.Data is not null) LogOutput(e.Data, true); };
        p.Start();
        p.BeginOutputReadLine();
        p.BeginErrorReadLine();
        await p.WaitForExitAsync();
        if (p.ExitCode != 0)
            throw new IOException("Build failed (exit " + p.ExitCode + ") before installing any files.\n" +
                                  "Command: " + Path.GetFileName(command) + "\n" +
                                  "Last output:\n" + BuildErrorSummary());
    }
}
