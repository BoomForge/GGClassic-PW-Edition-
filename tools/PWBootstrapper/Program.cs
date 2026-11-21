using System.Diagnostics;
using System.IO.Compression;
using System.Net.Http.Headers;
using System.Security.Cryptography;
using System.Text.Json;
using System.Windows.Forms;

// Original open launcher code. No proprietary GameGuru binaries or media distributed.
// Compiles the latest PW Edition source locally using user's Visual Studio 2022.
// Only explicitly allowlisted files from a successful build are installed, with backup.
namespace PixelwolfGGCUpdater;

internal static class Program
{
    const string Owner = "BoomForge";
    const string Repo = "GGClassic-PW-Edition-";
    const string Branch = "work/pw-community-integration-20261121";

    static readonly string[] FilesToInstall =
    {
        "Guru-MapEditor.exe",
        "pw-updater.ps1",
        "pw-edition-version.txt",
        @"Files\scriptbank\global.lua",
        @"Files\scriptbank\pw_transition_state.lua",
        @"Files\effectbank\reloaded\apbr_animwithtran_emissive.fx"
    };

    [STAThread]
    static int Main()
    {
        try { return Run().GetAwaiter().GetResult(); }
        catch (Exception e)
        {
            Console.Error.WriteLine(e);
            MessageBox.Show(e.Message, "PW Edition updater failed", MessageBoxButtons.OK, MessageBoxIcon.Error);
            Console.WriteLine("Press Enter to close.");
            Console.ReadLine();
            return 1;
        }
    }

    static async Task<int> Run()
    {
        Console.WriteLine("PIXELWOLF GAMEGURU CLASSIC - LOCAL WINDOWS UPDATER");
        Console.WriteLine("Compiles the fork locally. This EXE contains no GameGuru binaries or commercial assets.");
        Console.WriteLine("You need GameGuru Classic from Steam and VS2022 Desktop C++ with x64 SDK/MFC.");
        Console.WriteLine("Your levels, media and Steam GameGuru.exe launcher are never replaced.\n");

        using var picker = new FolderBrowserDialog
        {
            Description = "Select Steam GameGuru Classic folder (containing Files and GameGuru.exe)",
            UseDescriptionForTitle = true,
            ShowNewFolderButton = false
        };
        if (picker.ShowDialog() != DialogResult.OK) return 0;
        string steam = Path.GetFullPath(picker.SelectedPath).TrimEnd(Path.DirectorySeparatorChar);
        ValidateSteam(steam);

        string vswhere = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86),
                                      @"Microsoft Visual Studio\Installer\vswhere.exe");
        if (!File.Exists(vswhere)) throw new InvalidOperationException("Install Visual Studio 2022 Desktop development with C++, x64 MFC and Windows SDK first. Steam is unchanged.");
        string vsroot = (await Capture(vswhere,
            "-latest -products * -requires Microsoft.Component.MSBuild -property installationPath", null)).Trim();
        if (vsroot.Length == 0) throw new InvalidOperationException("MSBuild not found. Steam is unchanged.");
        string msbuild = Path.Combine(vsroot.Split('\n', StringSplitOptions.RemoveEmptyEntries).Last().Trim(),
                                     @"MSBuild\Current\Bin\MSBuild.exe");
        if (!File.Exists(msbuild)) throw new FileNotFoundException("MSBuild.exe not found", msbuild);

        if (MessageBox.Show(
            "Download the latest PW Edition GitHub source and build it on this computer?\n\n" +
            "No Steam files will change until compilation completes and you separately confirm installation.\n\n" +
            "Every replaced file will be backed up outside Steam. This is an experimental engine build.\n\nContinue?",
            "Pixelwolf PW Edition", MessageBoxButtons.YesNo, MessageBoxIcon.Warning) != DialogResult.Yes) return 0;

        using var http = new HttpClient { Timeout = TimeSpan.FromMinutes(30) };
        http.DefaultRequestHeaders.UserAgent.Add(new ProductInfoHeaderValue("Pixelwolf-GGC-Updater", "0.1"));
        string api = $"https://api.github.com/repos/{Owner}/{Repo}/commits/{Uri.EscapeDataString(Branch)}";
        Console.WriteLine("Looking up branch HEAD...");
        using var data = JsonDocument.Parse(await http.GetStringAsync(api));
        string sha = data.RootElement.GetProperty("sha").GetString()!;
        if (sha.Length != 40 || !sha.All(Uri.IsHexDigit))
            throw new InvalidDataException("Unexpected source commit ID.");
        Console.WriteLine("Pinned source: " + sha);
        string temp = Path.Combine(Path.GetTempPath(), "PWEdition-" + Guid.NewGuid().ToString("N"));
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
            string root = roots[0];
            if (!File.Exists(Path.Combine(root, @"GameGuru Core\GameGuru.sln")) ||
                !File.Exists(Path.Combine(root, @"tools\pw-prepare-d3dx.ps1")))
                throw new InvalidDataException("GitHub archive is missing the expected build scripts.");

            Console.WriteLine("Preparing Microsoft DirectX and Steamworks import libraries...");
            await Execute("powershell.exe", "-NoProfile -ExecutionPolicy Bypass -File " + Q(Path.Combine(root, @"tools\pw-prepare-d3dx.ps1")), root);
            await Execute("powershell.exe", "-NoProfile -ExecutionPolicy Bypass -File " + Q(Path.Combine(root, @"tools\pw-prepare-steam-imports.ps1")), root);
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
            foreach (var relative in FilesToInstall)
                if (!File.Exists(Path.Combine(built, relative)))
                    throw new FileNotFoundException("Required PW build output missing; nothing installed: " + relative);
            Console.WriteLine("\nFull x64 build complete. Files ready:");
            foreach (var relative in FilesToInstall) Console.WriteLine("  " + relative);

            ValidateSteam(steam);
            if (Process.GetProcessesByName("GameGuru").Length > 0 ||
                Process.GetProcessesByName("Guru-MapEditor").Length > 0)
                throw new InvalidOperationException("Close GameGuru Classic and its editor before installing. Steam has not been modified.");
            if (MessageBox.Show(
                "Build passed at commit " + sha[..10] + ".\n\n" +
                "Install the six compiled/allowlisted PW Edition files into:\n" + steam + "\n\n" +
                "Original versions will be backed up outside Steam. Your assets and original launcher are preserved.\n\nInstall now?",
                "Confirm local Steam update", MessageBoxButtons.YesNo, MessageBoxIcon.Warning) != DialogResult.Yes)
            {
                Console.WriteLine("Cancelled before installation. Steam is unchanged.");
                return 0;
            }
            string backup = InstallWithRollback(built, steam, sha);
            Console.WriteLine("\nSUCCESS. Steam: " + steam + "\nBackup: " + backup);
            MessageBox.Show(
                "PW Edition installed from commit " + sha[..10] + ".\n\nBackup: " + backup +
                "\n\nRun Guru-MapEditor.exe from the chosen GameGuru Classic folder and test on a disposable level.",
                "Pixelwolf PW Edition", MessageBoxButtons.OK, MessageBoxIcon.Information);
            return 0;
        }
        finally
        {
            try { Directory.Delete(temp, recursive: true); }
            catch (Exception e) { Console.WriteLine("Temporary source cleanup failed: " + temp + " (" + e.Message + ")"); }
        }
    }

    static string InstallWithRollback(string build, string steam, string sha)
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
            foreach (string relative in FilesToInstall)
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
        p.OutputDataReceived += (_, e) => { if (e.Data is not null) Console.WriteLine(e.Data); };
        p.ErrorDataReceived += (_, e) => { if (e.Data is not null) Console.Error.WriteLine(e.Data); };
        p.Start();
        p.BeginOutputReadLine();
        p.BeginErrorReadLine();
        await p.WaitForExitAsync();
        if (p.ExitCode != 0) throw new IOException("Build failed (exit " + p.ExitCode + ") before installing any files: " + command + " " + args);
    }
}
