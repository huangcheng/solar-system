; Inno Setup script for the Globe widget.
; Build steps:
;   1. cmake --build build --config Release
;   2. windeployqt --release build/Release/globe.exe
;   3. Copy build/Release/* to installer/payload/
;   4. Copy src/globe/render/shaders to installer/payload/shaders
;   5. (Optional) run scripts/fetch_textures.ps1 into installer/payload/textures
;   6. Run: iscc installer/globe.iss

#define MyAppName "Globe"
#define MyAppVersion "0.1.0"
#define MyAppPublisher "Globe"

[Setup]
AppId={{8F2B6D5A-3C1E-4GLOBE0001}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\{#MyAppName}
DisableProgramGroupPage=yes
OutputDir=..\dist
OutputBaseFilename=GlobeSetup-{#MyAppVersion}
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
PrivilegesRequired=lowest

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "startup"; Description: "Start Globe when Windows starts"; GroupDescription: "Extras:"

[Files]
Source: "payload\*"; DestDir: "{app}"; Flags: recursesubdirs createallsubdirs ignoreversion; Excludes: "textures\*,shaders\*"
; GLSL shaders ship as loose files next to the binary (loaded from disk).
Source: "payload\shaders\*"; DestDir: "{app}\shaders"; Flags: recursesubdirs createallsubdirs ignoreversion skipifsourcedoesntexist
; HD textures ship as loose files next to the binary (NOT embedded in the .qrc,
; to keep the executable small). Optional: installer still builds without them.
Source: "payload\textures\*"; DestDir: "{app}\textures"; Flags: recursesubdirs createallsubdirs ignoreversion skipifsourcedoesntexist

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\globe.exe"

[Run]
Filename: "{app}\globe.exe"; Description: "Launch Globe"; Flags: nowait postinstall skipifsilent

[Registry]
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; \
  ValueType: string; ValueName: "Globe"; ValueData: """{app}\globe.exe"""; \
  Flags: uninsdeletevalue; Tasks: startup
