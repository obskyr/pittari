# A wrapper around TODO.exe that will attempt to find the program whether it's
# in the PATH or in the same directory as this script. Thus, while you can't
# copy the EXE around all willy-nilly (because it depends on DLLs), you *can*
# copy this around.
#
# …Hold on. I just realized that this is pointless. If you've added the program
# directory to your PATH, it'll find the DLLs there, so you can copy the EXE.
# And if you haven't added the program directory to your PATH, you can't copy
# this batch file either.
#
# …
#
# Well. I guess I'm committing this and then committing its deletion.

$ProgramName = ([io.fileinfo] $MyInvocation.MyCommand.Path).BaseName

$OriginalPath = $env:Path
$env:Path = $PSScriptRoot + ";" + $env:Path
$ProgramPath = $((Get-Command ($ProgramName + ".exe") -ErrorAction SilentlyContinue).Path)
$env:Path = $OriginalPath

if ($ProgramPath) {
    Invoke-Expression "& `"$ProgramPath`" $Args"
    exit $LastExitCode
} else {
    [Console]::Error.WriteLine("ERROR: $ProgramName.exe not found. Try adding the directory it resides in to your PATH!")
    exit 1
}
