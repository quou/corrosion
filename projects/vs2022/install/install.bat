:: Installs corrosion development files to C:\Program Files\corrosion.
:: Requires the solution directory to be passed as the first argument
:: and the configuration to be passed as the second.

set install_dir=C:\Program Files\corrosion\

md "%install_dir%include"
md "%install_dir%bin"
md "%install_dir%lib"

echo %1..\..\corrosion\include\

xcopy /e /s /y /i "%1..\..\corrosion\include\*" "%install_dir%include"
xcopy /e /s /y /i "%1corrosion\bin\%2\corrosion.lib" "%install_dir%lib"
copy  /y "%1shadercompiler\bin\%2\shadercompiler.exe" "%install_dir%bin\cshc.exe"

setx /m CORROSION_SDK "%install_dir%
setx /m CORROSION_DEPS "vulkan-1.lib;opengl32.lib;winmm.lib;
setx /m CORROSION_DEP_PATHS %VULKAN_SDK%\Lib
setx /m PATH "%install_dir%bin