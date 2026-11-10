# Compatibility entry point. Release builds use the ordinary .NET compiler.
& (Join-Path $PSScriptRoot 'build_native_controller.ps1')
