# **Comprehensive Engineering Standards for Audio Plugin Deployment and Installer User Experience**

The maturation of the digital audio workstation ecosystem has shifted the criteria for professional software from purely sonic performance to the holistic user experience, beginning at the moment of installation. For the Unravel reverb plugin, a JUCE 8-based product supporting VST3 and Audio Unit formats, the installation architecture represents the first point of professional contact between the developer and the end-user. The contemporary landscape of 2024 and 2025 demands a deployment strategy that navigates increasingly stringent operating system security, standardized file system topographies, and the necessity for automated, high-integrity build pipelines.1

## **Architectural Foundations of Modern Plugin Distribution**

The transition to JUCE 8 has solidified the industry's reliance on the VST3 and Audio Unit (AU) formats as the primary vectors for cross-platform audio processing. While the legacy VST2 format allowed for a degree of flexibility in installation paths, it often led to fragmented file systems and increased support overhead. VST3, by contrast, mandates a unified installation structure that host applications like Ableton Live, Logic Pro, and Cubase expect by default.3 For the Unravel plugin, the deployment strategy must leverage these standards to ensure immediate visibility within the host's plugin manager while maintaining a cohesive brand identity throughout the setup wizard.

The user-friendly installation process is not merely a convenience but a critical component of software reliability. Industry data suggests that a significant percentage of plugin-related technical support queries originate from installation errors, such as incorrect directory placement or permission conflicts.5 By adhering to the established pathing logic for Windows and macOS, the developer minimizes these failure points. Furthermore, the exclusion of the Pro Tools AAX format in the initial release of Unravel simplifies the cryptographic requirements, as AAX binaries require a proprietary PACE signing process that adds significant complexity to the build orchestration.1

### **Comparative Framework of Plugin Formats and Host Requirements**

| Format | Extension | Platform | Standard Installation Path | Host Recognition Mechanism |
| :---- | :---- | :---- | :---- | :---- |
| VST3 | .vst3 | Windows | C:\\Program Files\\Common Files\\VST3 | Standardized Directory Scan 3 |
| VST3 | .vst3 | macOS | /Library/Audio/Plug-Ins/VST3 | System-wide Bundle Discovery 3 |
| Audio Unit | .component | macOS | /Library/Audio/Plug-Ins/Components | Core Audio Component Manager 4 |

The architecture of the Unravel installer must account for the distinct ways these platforms handle binary execution and security. Windows utilizes a flat file structure or a bundle format for VST3, whereas macOS treats both AU and VST3 as bundles—specialized directory structures that appear as single files to the user but contain the executable binary, resources, and metadata required for proper operation.3

## **Standardized Installation Topographies and Permission Logic**

The professional audio industry has established rigorous standards for where software components should reside to ensure stability across different user accounts and system configurations. Deviating from these paths is considered an anti-pattern that can lead to DAW scanning failures and permission-related crashes.8

### **Windows File System Architecture**

On the Windows platform, the Unravel plugin must target the 64-bit Program Files directory. Modern audio production has almost entirely migrated to 64-bit environments, and maintaining 32-bit compatibility is often an unnecessary technical debt that can introduce instability.8 The standard VST3 path is explicitly defined to prevent the chaos of the VST2 era, where users frequently lost track of plugin locations.

For the support data associated with Unravel—such as impulse responses, preset libraries, and configuration files—it is recommended to use the C:\\Users\\Public\\Documents directory. This ensures that the data is accessible to all users on the machine and, more importantly, that the plugin can read and write to these folders without requiring elevated administrator privileges after the initial installation.8 Placing large sample libraries on the system drive is common, but providing the user with the option to select a custom path for high-capacity data is a hallmark of a user-friendly installer.8

### **macOS Directory Standards**

MacOS installation is governed by the system's Library structure. While macOS provides both a system-level and a user-level library, professional audio installers almost exclusively target the system-level library to ensure the plugin is available to all accounts and to maintain consistency with other professional tools.3

The primary paths for Unravel on macOS are as follows:

* **VST3:** /Library/Audio/Plug-Ins/VST3/Unravel.vst3 4  
* **Audio Unit:** /Library/Audio/Plug-Ins/Components/Unravel.component 4  
* **Application Support:** /Library/Application Support/Unravel/ for global presets and resources.10

The macOS file system is highly sensitive to permissions. The installer must be configured to request root privileges via the system's authentication prompts to write to these directories. Failure to set the correct ownership and permissions during the installation phase can result in the plugin being blocked by the DAW or failing to load its internal resources.6

## **The Security Framework: Code Signing and Operating System Trust**

In the contemporary computing environment, an unsigned installer is functionally equivalent to malware in the eyes of the operating system. To achieve a professional, branded experience for Unravel, the developer must implement a rigorous code-signing and notarization workflow. This process verifies the identity of the developer and ensures that the software has not been tampered with since its creation.1

### **Apple Gatekeeper and the Notarization Workflow**

MacOS employs a security feature known as Gatekeeper, which requires all distributed software to be signed with an authorized Apple Developer ID and notarized by Apple's servers. For a plugin developer, this necessitates an active Apple Developer Program membership ($99/year).1

The process involves two distinct certificates:

1. **Developer ID Application Certificate:** Used to sign the individual plugin binaries (.vst3 and .component).2  
2. **Developer ID Installer Certificate:** Used to sign the final flat installer package (.pkg) or disk image (.dmg).2

A critical technical requirement for JUCE 8 plugins on macOS is the "Hardened Runtime." Notarization will fail unless the binaries are signed with this option enabled. This provides a layer of protection against certain classes of exploits but requires the plugin to declare any necessary entitlements, such as the ability to load third-party libraries or access the microphone.13

### **Microsoft SmartScreen and Azure Trusted Signing**

Windows security is handled primarily by Defender SmartScreen, which evaluates the "reputation" of an application. Historically, new developers faced a significant hurdle where their software would trigger a "Windows Protected Your PC" warning until a sufficient number of users had installed it. In 2024 and 2025, the standard for bypassing this hurdle has shifted toward Azure Trusted Signing.1

Azure Trusted Signing is a cloud-based service that replaces traditional physical USB tokens. By using an identity-verified signing account, the Unravel installer receives an immediate reputation boost, allowing it to bypass SmartScreen warnings from day one.1 This integration is performed using Microsoft's signtool.exe and a specialized library (dlib) that connects to the Azure signing service.16

| Platform | Warning Mechanism | Trust Requirement | Solution for 2025 |
| :---- | :---- | :---- | :---- |
| macOS | Gatekeeper | Notarization Ticket | notarytool \+ Developer ID 2 |
| Windows | SmartScreen | Certificate Reputation | Azure Trusted Signing 1 |

## **Windows Deployment Mechanics: Creating the Branded Installer**

For the Windows version of Unravel, Inno Setup has emerged as the industry-standard tool for creating professional, scriptable installers. It offers a balance of ease of use for the developer and a familiar, reliable interface for the end-user.18

### **Designing the Branded Experience in Inno Setup**

A branded user experience is achieved through the customization of the Inno Setup wizard. This includes the use of high-resolution images that reflect the visual identity of the Unravel reverb. The script utilizes specific directives to override the default assets.20

The WizardImageFile directive defines the large image displayed on the left side of the "Welcome" and "Finished" pages. In contemporary high-DPI environments, this image should be at least 240x459 pixels to ensure clarity on 4K monitors.21 The WizardSmallImageFile is displayed in the upper right corner of the subsequent pages and should be 55x58 pixels.22 Modern versions of Inno Setup support transparent PNG files, which allow for a much cleaner, more integrated aesthetic than the legacy BMP format.21

### **Technical Logic for Plugin Installation**

The Inno Setup script (.iss) must handle several critical tasks beyond simply copying files. It must detect if the plugin is already installed, handle the overwriting of older versions, and provide an uninstaller that completely removes the software.19

For Unravel, the script should be configured to target the {commoncf}\\VST3 directory. This is an Inno Setup constant that maps to the correct system-wide VST3 folder regardless of the user's drive configuration.9 Additionally, if Unravel requires specific registry keys for authorization or state management, these are defined in the \`\` section of the script.23 A crucial feature for user friendliness is the "In-Use" file detection; if the user has a DAW open and is currently using Unravel, the installer should prompt them to close the application rather than attempting a reboot.19

## **macOS Deployment Mechanics: Packages and Managed Workflows**

The macOS installation experience is typically delivered via a flat installer package (.pkg) or a disk image (.dmg). For a multi-format plugin like Unravel, the .pkg format is technically superior because it allows for a structured, guided installation process that can place different formats into their respective directories simultaneously.2

### **Creating the Professional Package**

A macOS package is constructed using the pkgbuild and productbuild utilities. A professional installer for Unravel should include:

* **Component Packages:** Individual packages for the AU and VST3 versions.  
* **Distribution XML:** A configuration file that defines the welcome text, license agreement, and the visual appearance of the installer wizard.24  
* **Scripts:** Pre-install and post-install shell scripts that handle system-level configuration.11

A significant branding opportunity exists within the .pkg structure through the use of a background image. This image is defined in the distribution file and appears behind the installation steps, providing a cohesive aesthetic that aligns with the plugin's GUI.

### **The Role of Post-Install Scripts**

Post-install scripts are essential for ensuring a seamless experience on macOS. For Unravel, a post-install script can be used to clear the system's plugin cache or to force the OS to rescan the component directory.11 This is particularly important for Audio Units, which are managed by the audiocomponentrepl service. Without a proper rescan, the user might not see the new plugin in Logic Pro until they restart their computer—a significant friction point that a professional installer should eliminate.4

## **Build System Orchestration: Integrating JUCE 8 and CMake**

The development of the Unravel plugin within the JUCE 8 framework provides a streamlined path toward automation via CMake. JUCE's CMake API allows the developer to define the plugin's metadata once and have it propagated across both Windows and macOS build targets.14

### **Staging Artifacts for Installation**

The build process should be designed to output the compiled binaries into a "staging" directory. This directory structure mimics the final installation paths and serves as the source for the installer creation tools. In JUCE's CMake, the juce\_add\_plugin function provides several helpers for this purpose.14

The property JUCE\_PLUGIN\_ARTEFACT\_FILE is particularly useful for locating the built bundle in a platform-independent way. Because CMake generators (like Xcode or Visual Studio) may append configuration-specific suffixes to the output path, using this property ensures that the staging script always finds the correct binary for signing and packaging.26

### **Automating the Signing Step**

The build pipeline must integrate the code-signing tools as a post-build step. This ensures that the binaries are protected before they are bundled into the installer.

| OS | Build Tool | Signing Command Requirement |
| :---- | :---- | :---- |
| Windows | MSVC / CMake | signtool.exe sign /dlib... /dmdf... 16 |
| macOS | Xcode / CMake | codesign \--timestamp \--options runtime... 2 |

By configuring these commands within the CMake environment, the developer ensures that every build—whether local or on a CI server—is ready for distribution. This reduces the risk of accidentally distributing an unsigned or "ad-hoc" signed binary, which would immediately fail Gatekeeper or SmartScreen checks.1

## **Advanced User Experience: Updates and Maintenance**

The lifecycle of the Unravel plugin extends beyond the initial installation. The way the software handles updates and uninstallation is a key differentiator between amateur and professional products.5

### **Managing Overwrites and Versioning**

A common failure mode in plugin updates is the "merging" of bundles. If a new version of Unravel changes the internal file structure of the .vst3 bundle, simply copying the new files over the old ones on macOS can leave behind orphaned files that break the cryptographic signature.11

The professional solution is a "clean slate" update logic:

1. **Pre-Install Detection:** The installer checks for existing versions of Unravel.  
2. **Removal:** The old plugin bundles are deleted entirely from the /Library/Audio/Plug-Ins/ directories.11  
3. **Clean Installation:** The new version is copied into the now-empty location.

This approach is highly recommended for all audio plugins, particularly those that may eventually support the AAX format, where PACE signatures are extremely sensitive to bundle integrity.6

### **Preserving User Sovereignty over Data**

While the plugin binary should be cleanly replaced, the user's presets, IR samples, and license information must remain untouched. Professional installers achieve this by explicitly separating the "App Data" from the "App Binary".8 The Unravel installer should never delete files in the Documents or Application Support folders unless specifically instructed by the user during an uninstallation process. This preservation of user data is a fundamental aspect of trust and user-friendliness in the audio community.28

## **Continuous Integration and the Automated Pipeline**

In the modern development cycle, the manual creation of installers is considered a high-risk activity. For Unravel, the implementation of a Continuous Integration (CI) pipeline using GitHub Actions is the most effective way to ensure consistent, high-quality releases.1

### **The Pamplejuce Methodology**

The "Pamplejuce" template has become a cornerstone for JUCE-based CI. It provides a standardized framework for building, signing, and packaging plugins on remote virtual machines.2

For the Unravel project, the CI pipeline must manage several sensitive assets:

* **Certificate Storage:** MacOS certificates (.p12) are stored as Base64-encoded secrets and imported into a temporary keychain during the build run.2  
* **Azure Credentials:** Windows signing requires the storage of the Azure client ID and secret to authenticate the signtool with the cloud signing service.15  
* **Notarization Credentials:** Apple ID and app-specific passwords must be provided to the notarytool to facilitate the automated upload and verification process.27

This automated approach ensures that every release of Unravel is identical in its structural integrity and security compliance, providing a reliable foundation for the brand's reputation.1

## **Implementation Blueprints for AI-Assisted Development**

To facilitate the implementation of these complex systems using an AI tool like Cursor, the developer should provide clear, modular blueprints. These blueprints translate the high-level research into actionable code structures.

### **Blueprint for Inno Setup (Windows)**

When directing an AI to generate the Windows installer script, the prompt should specify the following logic:

* **Structure:** "Generate an Inno Setup 6.2+ script for a VST3 plugin named Unravel. Use {commoncf}\\VST3 as the destination for the .vst3 bundle." 9  
* **Branding:** "Include placeholders for WizardImageFile and WizardSmallImageFile. Set WizardStyle=modern and WizardResizable=yes." 21  
* **Update Logic:** "Implement a check in the \[Code\] section to detect existing installations by registry key and prompt for removal or handle the overwrite automatically." 11  
* **Signing:** "Configure the SignTool directive to use a batch file wrapper that passes the $f parameter to the Azure Trusted Signing client." 30

### **Blueprint for macOS Notarization Script**

For the macOS build script, the AI should be instructed to:

* **Environment:** "Create a Bash script for macOS that uses notarytool (Xcode 13+). Do not use the deprecated altool." 2  
* **Workflow:** "The script should first sign the .component and .vst3 bundles using codesign with the \--options runtime flag. Then, it should package them into a .pkg using productbuild." 2  
* **Submission:** "The final package should be submitted to the Apple Notary Service. Include logic to wait for the status and then staple the ticket using xcrun stapler staple." 13

## **UX Optimization: The Psychology of the Installer**

The visual and functional design of the installer has a direct impact on the user's perception of the plugin's quality. A "branded" experience goes beyond placing a logo; it involves the clarity of communication and the speed of the process.32

### **Clarity and Transparency**

The Unravel installer should provide the user with clear information at every step. This includes a concise "Readme" or "What's New" section and a transparent license agreement. During the format selection screen, the installer should clearly explain the difference between VST3 and AU, helping users who may be less technically inclined to choose only what they need, thereby saving disk space and reducing DAW scan times.32

### **The Role of Customization**

While standard paths are recommended, the ability for advanced users to customize their installation is a key component of user-friendliness. For Unravel, this means allowing the user to select which plugin formats to install and where to place large data folders.8 This level of control respects the user's workflow and is common among high-end audio software manufacturers.7

## **Common Pitfalls and Mitigation Strategies**

Even with a robust plan, several technical hurdles can derail the installation experience. Identifying these early is crucial for the Unravel deployment strategy.

### **Digital Detritus and Bundle Issues**

A frequent issue on macOS is the presence of "resource forks" or "Finder information" within the plugin bundle, which can cause code-signing failures. The installer build script should utilize the dot\_clean utility to remove these hidden files before the signing process begins.27

### **DAW Scanning and Cache Conflicts**

DAWs often cache plugin metadata to speed up launch times. If a user installs an update to Unravel and the version number is not correctly incremented in the Info.plist (macOS) or version resources (Windows), the DAW may fail to recognize the new version, leading to confusion.5 The build system must ensure that version strings are consistent across all metadata files and the installer itself.

### **The AAX Signing Trap**

Although Unravel does not yet support AAX, future integration will require a specialized signing tool from PACE. A common pitfall for developers adding AAX support is attempting to sign a binary that has already been signed by the PACE WrapTool. This results in a "Error 38" signing failure.1 When Unravel eventually expands to include Pro Tools support, the signing logic must be updated to handle the platform-specific signature before the PACE signature is applied.1

## **Final Synthesis of the Installation Strategy**

The deployment of the Unravel reverb plugin requires a sophisticated intersection of software engineering, security compliance, and user interface design. By leveraging the JUCE 8 framework and modern CMake practices, the developer can create a high-integrity build pipeline that automates the complexities of code-signing and packaging.

For the Windows market, the combination of Inno Setup and Azure Trusted Signing provides a path toward immediate trust and a professional aesthetic. On macOS, the use of notarytool and flat packages ensures that the plugin is treated as a first-class citizen by the operating system's security layers.

The ultimate measure of success for the Unravel installer is its invisibility; a process so smooth, branded, and reliable that the user can transition from the purchase to the first reverb tail in a matter of seconds. This professional foundation not only reduces technical support requirements but also reinforces the value of the Unravel brand in a competitive marketplace.32

The deployment ecosystem is constantly evolving, with Apple and Microsoft frequently updating their security requirements. The maintenance of the Unravel installer should be considered an ongoing task, with quarterly reviews of the build pipeline and security certificates to ensure uninterrupted distribution and a superior user experience.1

#### **Works cited**

1. Code signing audio plugins in 2025, a round-up \- Moonbase, accessed January 28, 2026, [https://moonbase.sh/articles/code-signing-audio-plugins-in-2025-a-round-up/](https://moonbase.sh/articles/code-signing-audio-plugins-in-2025-a-round-up/)  
2. How to code sign and notarize macOS audio plugins in CI · Melatonin, accessed January 28, 2026, [https://melatonin.dev/blog/how-to-code-sign-and-notarize-macos-audio-plugins-in-ci/](https://melatonin.dev/blog/how-to-code-sign-and-notarize-macos-audio-plugins-in-ci/)  
3. Plug-in Locations \- VST 3 Developer Portal \- GitHub Pages, accessed January 28, 2026, [https://steinbergmedia.github.io/vst3\_dev\_portal/pages/Technical+Documentation/Locations+Format/Plugin+Locations.html](https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical+Documentation/Locations+Format/Plugin+Locations.html)  
4. VST plug-in locations on Mac OS X and macOS \- Steinberg Help Center, accessed January 28, 2026, [https://helpcenter.steinberg.de/hc/en-us/articles/115000171310-VST-plug-in-locations-on-Mac-OS-X-and-macOS](https://helpcenter.steinberg.de/hc/en-us/articles/115000171310-VST-plug-in-locations-on-Mac-OS-X-and-macOS)  
5. I ran the update, but I still see the old version of the plugin in my DAW, accessed January 28, 2026, [https://support.xferrecords.com/article/15-i-ran-the-update-but-i-still-see-the-old-version-of-the-plugin-in-my-daw](https://support.xferrecords.com/article/15-i-ran-the-update-but-i-still-see-the-old-version-of-the-plugin-in-my-daw)  
6. VST Installer? \- MacOSX and iOS \- JUCE Forum, accessed January 28, 2026, [https://forum.juce.com/t/vst-installer/16654](https://forum.juce.com/t/vst-installer/16654)  
7. Where Are My Air Music Technology Plugins Stored on My Computer? \- Akai Professional, accessed January 28, 2026, [https://support.akaipro.com/en/support/solutions/articles/69000858423-air-music-tech-where-are-my-air-music-technology-plugins-stored-on-my-computer-](https://support.akaipro.com/en/support/solutions/articles/69000858423-air-music-tech-where-are-my-air-music-technology-plugins-stored-on-my-computer-)  
8. Best Practices for deploying VST plugins on Windows \- Gig Performer Community, accessed January 28, 2026, [https://community.gigperformer.com/t/best-practices-for-deploying-vst-plugins-on-windows/2514](https://community.gigperformer.com/t/best-practices-for-deploying-vst-plugins-on-windows/2514)  
9. Automatic Installer for Windows: Inno Setup Script \- HISE Forum, accessed January 28, 2026, [https://forum.hise.audio/topic/7832/automatic-installer-for-windows-inno-setup-script](https://forum.hise.audio/topic/7832/automatic-installer-for-windows-inno-setup-script)  
10. Need help to figure out how updates for users work. \- HISE Forum, accessed January 28, 2026, [https://forum.hise.audio/topic/6247/need-help-to-figure-out-how-updates-for-users-work](https://forum.hise.audio/topic/6247/need-help-to-figure-out-how-updates-for-users-work)  
11. Packages: how overwite existing plugin installs \- MacOSX and iOS ..., accessed January 28, 2026, [https://forum.juce.com/t/packages-how-overwite-existing-plugin-installs/40584](https://forum.juce.com/t/packages-how-overwite-existing-plugin-installs/40584)  
12. Prepare plugins for distribution on macOS (notarization, code signing etc) \- JUCE Forum, accessed January 28, 2026, [https://forum.juce.com/t/prepare-plugins-for-distribution-on-macos-notarization-code-signing-etc/53124](https://forum.juce.com/t/prepare-plugins-for-distribution-on-macos-notarization-code-signing-etc/53124)  
13. Notarizing macOS software before distribution | Apple Developer Documentation, accessed January 28, 2026, [https://developer.apple.com/documentation/security/notarizing-macos-software-before-distribution](https://developer.apple.com/documentation/security/notarizing-macos-software-before-distribution)  
14. JUCE/docs/CMake API.md at master · juce-framework/JUCE · GitHub, accessed January 28, 2026, [https://github.com/juce-framework/JUCE/blob/master/docs/CMake%20API.md](https://github.com/juce-framework/JUCE/blob/master/docs/CMake%20API.md)  
15. How To Sign Installers With Azure Trusted Signing And Paquet Builder, accessed January 28, 2026, [https://www.installpackbuilder.com/tutorials/installer-azure-trusted-signing](https://www.installpackbuilder.com/tutorials/installer-azure-trusted-signing)  
16. Set up signing integrations to use Artifact Signing \- Microsoft Learn, accessed January 28, 2026, [https://learn.microsoft.com/en-us/azure/artifact-signing/how-to-signing-integrations](https://learn.microsoft.com/en-us/azure/artifact-signing/how-to-signing-integrations)  
17. Microsoft Trusted Code Signing \- Steps to set it up \- ClarionHub, accessed January 28, 2026, [https://clarionhub.com/t/microsoft-trusted-code-signing-steps-to-set-it-up/7861](https://clarionhub.com/t/microsoft-trusted-code-signing-steps-to-set-it-up/7861)  
18. Creating Installations \- Inno Setup Help, accessed January 28, 2026, [https://jrsoftware.org/ishelp/topic\_creatinginstallations.htm](https://jrsoftware.org/ishelp/topic_creatinginstallations.htm)  
19. Inno Setup \- JRSoftware.org, accessed January 28, 2026, [https://jrsoftware.org/isinfo.php](https://jrsoftware.org/isinfo.php)  
20. \[Setup\]: WizardSmallImageFile \- Inno Setup Help, accessed January 28, 2026, [https://jrsoftware.org/ishelp/topic\_setup\_wizardsmallimagefile.htm](https://jrsoftware.org/ishelp/topic_setup_wizardsmallimagefile.htm)  
21. \[Setup\]: WizardImageFile \- Inno Setup Help, accessed January 28, 2026, [https://jrsoftware.org/ishelp/topic\_setup\_wizardimagefile.htm](https://jrsoftware.org/ishelp/topic_setup_wizardimagefile.htm)  
22. Change the image in the Inno Setup wizard banner \- Stack Overflow, accessed January 28, 2026, [https://stackoverflow.com/questions/7502365/change-the-image-in-the-inno-setup-wizard-banner](https://stackoverflow.com/questions/7502365/change-the-image-in-the-inno-setup-wizard-banner)  
23. Inno Setup Example: Creating a Simple Installer | by UATeam | Medium, accessed January 28, 2026, [https://medium.com/@aleksej.gudkov/inno-setup-example-creating-a-simple-installer-73aa59941feb](https://medium.com/@aleksej.gudkov/inno-setup-example-creating-a-simple-installer-73aa59941feb)  
24. Customizing the notarization workflow | Apple Developer Documentation, accessed January 28, 2026, [https://developer.apple.com/documentation/security/customizing-the-notarization-workflow](https://developer.apple.com/documentation/security/customizing-the-notarization-workflow)  
25. How to use CMake with JUCE · Melatonin \- Sine Machine, accessed January 28, 2026, [https://melatonin.dev/blog/how-to-use-cmake-with-juce/](https://melatonin.dev/blog/how-to-use-cmake-with-juce/)  
26. Moving to CMake & creating installers \- JUCE Forum, accessed January 28, 2026, [https://forum.juce.com/t/moving-to-cmake-creating-installers/52791](https://forum.juce.com/t/moving-to-cmake-creating-installers/52791)  
27. Apple Audio Plugin Notarization (Dmitri Volkov), accessed January 28, 2026, [https://www.dmitrivolkov.com/misc/apple-audio-plugin-notarization/](https://www.dmitrivolkov.com/misc/apple-audio-plugin-notarization/)  
28. Installing over older version; should I remove that version first? \- PG Music Forums, accessed January 28, 2026, [https://www.pgmusic.com/forums/ubbthreads.php?ubb=showflat\&Number=453664](https://www.pgmusic.com/forums/ubbthreads.php?ubb=showflat&Number=453664)  
29. how do you approach deleting plugins? (considering/worried about old projects) \- Reddit, accessed January 28, 2026, [https://www.reddit.com/r/AdvancedProduction/comments/1g0msm5/how\_do\_you\_approach\_deleting\_plugins/](https://www.reddit.com/r/AdvancedProduction/comments/1g0msm5/how_do_you_approach_deleting_plugins/)  
30. \[Setup\]: SignTool \- Inno Setup Help, accessed January 28, 2026, [https://jrsoftware.org/ishelp/topic\_setup\_signtool.htm](https://jrsoftware.org/ishelp/topic_setup_signtool.htm)  
31. MS Azure Trusted Signing Accounts, accessed January 28, 2026, [https://groups.google.com/g/innosetup/c/VqXDQ1G-KLg](https://groups.google.com/g/innosetup/c/VqXDQ1G-KLg)  
32. The Ultimate Guide To Audio Plugins For Producers 2025 \- Mixing Monster, accessed January 28, 2026, [https://mixingmonster.com/audio-plugins/](https://mixingmonster.com/audio-plugins/)  
33. 2024-25 Plugin Recommendations WithTrial : r/mixingmastering \- Reddit, accessed January 28, 2026, [https://www.reddit.com/r/mixingmastering/comments/1jbsgq1/202425\_plugin\_recommendations\_withtrial/](https://www.reddit.com/r/mixingmastering/comments/1jbsgq1/202425_plugin_recommendations_withtrial/)  
34. How to Use VST3 or AU Plugins in Reaper (macOS & Windows) : \- Freshdesk, accessed January 28, 2026, [https://lostin70s.freshdesk.com/support/solutions/articles/203000046366-how-to-use-vst3-or-au-plugins-in-reaper-macos-windows-](https://lostin70s.freshdesk.com/support/solutions/articles/203000046366-how-to-use-vst3-or-au-plugins-in-reaper-macos-windows-)  
35. Best guitar plugins 2025: level up your recorded tones, accessed January 28, 2026, [https://www.guitarworld.com/gear/plugins-apps/best-guitar-plugins](https://www.guitarworld.com/gear/plugins-apps/best-guitar-plugins)