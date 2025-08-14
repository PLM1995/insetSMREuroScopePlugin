cd build
cmake --build .
copy Debug\myPlugIn.dll "%APPDATA%\EuroScope\UK\Data\Plugin\PLM1995Plugin.dll"
start "" /D "C:\Users\peter\AppData\Roaming\EuroScope\" "C:\Program Files (x86)\EuroScope\EuroScope.exe"