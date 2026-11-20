# Build Additive 16 online (Windows 10, no developer tools)

1. Create/sign in to a free account at https://github.com.
2. Create a new **public** repository, e.g. `Additive16`. (Public repositories have GitHub-hosted Actions minutes available.)
3. Upload **the contents** of the `Additive16` project directory to the repository root, including `.github/workflows/build-windows.yml`. GitHub's browser upload may hide dotfolders; if so, create the workflow file via GitHub's **Add file > Create new file**, entering `.github/workflows/build-windows.yml` as its filename and copying the file contents from this package.
4. Open **Actions** > **Build Windows VST3** > **Run workflow**. If prompted, enable Actions for the repository.
5. Wait for a green success indicator. Open the completed run and download the `Additive16-Windows-VST3` artifact.
6. Extract the artifact ZIP, then extract `Additive16-Windows-VST3.zip` inside it. Copy the resulting `Additive 16.vst3` folder to `C:\Program Files\Common Files\VST3\`.
7. Restart REAPER. Go to Options > Preferences > Plug-ins > VST and select Re-scan. Add an instrument track and search for `Additive 16`.

The workflow builds from source on a GitHub-hosted Windows 2022 runner. No compiler installation is needed on your PC. This source has not been compiled or verified in REAPER yet; if the build fails, share the error log so it can be corrected.
