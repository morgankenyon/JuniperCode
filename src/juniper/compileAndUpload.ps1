## Compiles juniper file, outputs a main.cpp to ../compiled folder

Write-Output "Compiling"

Juniper.exe -s fade.jun -o ../compiled/src/main.cpp

Write-Output "Compiled to ../compiled/src/main.cpp"

Get-Location

Set-Location -Path ../compiled -PassThru

./uploadToUno.ps1

Set-Location -Path ../juniper -PassThru