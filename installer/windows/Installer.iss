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
WizardImageFile=assets\wizard-image.png
WizardSmallImageFile=assets\wizard-small.png
SignTool=AzureSign $f
SignedUninstaller=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "..\..\staging\VST3\{#Vst3Bundle}\*"; DestDir: "{commoncf}\VST3\{#Vst3Bundle}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Dirs]
Name: "{commondocs}\Unravel"; Permissions: everyone-modify

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf}\VST3\{#Vst3Bundle}"

[SignTools]
Name: "AzureSign"; Command: "sign.bat $f"

[Code]
function InitializeSetup(): Boolean;
begin
  if DirExists(ExpandConstant('{commoncf}\VST3\{#Vst3Bundle}')) then
  begin
    MsgBox('Please close any DAWs using Unravel before continuing.', mbInformation, MB_OK);
  end;
  Result := True;
end;
