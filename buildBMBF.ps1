# Builds a .zip file for loading with BMBF
$NDKPath = Get-Content $PSScriptRoot/ndkpath.txt

$buildScript = "$NDKPath/build/ndk-build"
if (-not ($PSVersionTable.PSEdition -eq "Core")) {
    $buildScript += ".cmd"
}

& $buildScript NDK_PROJECT_PATH=$PSScriptRoot APP_BUILD_SCRIPT=$PSScriptRoot/Android.mk NDK_APPLICATION_MK=$PSScriptRoot/Application.mk

Compress-Archive -Path "./libs/arm64-v8a/libauto-debris_0_1_0.so", "./extern/libcodegen_0_3_4.so", "./extern/libbeatsaber-hook_0_7_8.so", "./bmbfmod.json" -DestinationPath "./auto-debris_v0.1.0.zip" -Update
