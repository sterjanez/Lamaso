If (!(test-path bin))
{
    mkdir bin
}

If (!(test-path build))
{
    mkdir build
}

Set-Location build\
cmake -DCMAKE_INSTALL_PREFIX="$PSScriptRoot" ..
cmake --build . --target ALL_BUILD --config Release
Set-Location ..
cmake --install build --config Release