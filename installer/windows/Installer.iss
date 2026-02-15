#define AppName "Unravel"
#define AppVersion "0.4.0"
#define AppPublisher "threadbare"
#define AppId "threadbare.unravel"
#define Vst3Bundle "unravel.vst3"

[Setup]
AppId={#AppId}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher={#AppPublisher}
DefaultDirName={commoncf}\VST3\{#Vst3Bundle}
DisableDirPage=yes
DisableProgramGroupPage=yes
PrivilegesRequired=admin
OutputBaseFilename=Unravel-Installer
OutputDir={#SourcePath}\..\..\Output
Compression=lzma
SolidCompression=yes
WizardStyle=modern
WizardResizable=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "..\..\staging\VST3\{#Vst3Bundle}\*"; DestDir: "{commoncf}\VST3\{#Vst3Bundle}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Dirs]
Name: "{commondocs}\Unravel"; Permissions: everyone-modify

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf}\VST3\{#Vst3Bundle}"
