param(
    [Parameter(Mandatory = $true)][string]$Source,
    [Parameter(Mandatory = $true)][string]$Destination
)
$ErrorActionPreference = 'Stop'
$sourceRoot = [IO.Path]::GetFullPath($Source).TrimEnd('\', '/')
$destinationRoot = [IO.Path]::GetFullPath($Destination).TrimEnd('\', '/')
if ($sourceRoot -eq $destinationRoot) { throw 'Runtime output must differ from the source directory.' }
New-Item -ItemType Directory -Force -Path $destinationRoot | Out-Null
foreach ($directory in @('Data', 'Shaders', 'Fonts')) {
    Copy-Item -LiteralPath (Join-Path $sourceRoot $directory) -Destination $destinationRoot -Recurse -Force
}
Get-ChildItem -LiteralPath (Join-Path $sourceRoot 'Assets') -Recurse -File |
    Where-Object {
        $_.Extension -in @('.png', '.json') -and
        $_.FullName -notlike (Join-Path $sourceRoot 'Assets\Previews\*')
    } | ForEach-Object {
        $relative = $_.FullName.Substring($sourceRoot.Length + 1)
        $target = Join-Path $destinationRoot $relative
        New-Item -ItemType Directory -Force -Path ([IO.Path]::GetDirectoryName($target)) | Out-Null
        Copy-Item -LiteralPath $_.FullName -Destination $target -Force
    }
$licenses = Join-Path $destinationRoot 'Licenses'
New-Item -ItemType Directory -Force -Path $licenses | Out-Null
Copy-Item -LiteralPath (Join-Path $sourceRoot 'Dependencies/glfw/LICENSE.md') -Destination (Join-Path $licenses 'GLFW.txt') -Force
Copy-Item -LiteralPath (Join-Path $sourceRoot 'Dependencies/glew/LICENSE.txt') -Destination (Join-Path $licenses 'GLEW.txt') -Force
Copy-Item -LiteralPath (Join-Path $sourceRoot 'Assets/PixelArt/Effects/quest-glimmer-LICENSE.txt') -Destination (Join-Path $licenses 'PixelfxQuestGlimmer.txt') -Force
Copy-Item -LiteralPath (Join-Path $sourceRoot 'Assets/PixelArt/Effects/chatacabra-hard-LICENSE.txt') -Destination (Join-Path $licenses 'PixelfxChatacabraHard.txt') -Force
