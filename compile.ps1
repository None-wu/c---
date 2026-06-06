# 使用环境变量中的 MinGW，若未设置则尝试常见路径
if (Test-Path "$env:MSYS2_HOME\mingw64\bin") {
    $env:PATH = "$env:MSYS2_HOME\mingw64\bin;$env:PATH"
} elseif (Test-Path "C:\msys64\mingw64\bin") {
    $env:PATH = "C:\msys64\mingw64\bin;$env:PATH"
}

$libParams = @(
    "-lpangocairo-1.0",
    "-lpangowin32-1.0",
    "-lpango-1.0",
    "-lcairo-gobject",
    "-lcairo",
    "-lharfbuzz",
    "-lgdk_pixbuf-2.0",
    "-latk-1.0",
    "-lgio-2.0",
    "-lgobject-2.0",
    "-lglib-2.0",
    "-lintl",
    "-lm"
)

gcc -finput-charset=UTF-8 -fexec-charset=UTF-8 -o study_system.exe "学习管理系统\学习管理系统.c" -IC:/msys64/mingw64/include/gtk-3.0 -IC:/msys64/mingw64/include/pango-1.0 -IC:/msys64/mingw64/include -IC:/msys64/mingw64/include/harfbuzz -IC:/msys64/mingw64/include/cairo -IC:/msys64/mingw64/include/freetype2 -IC:/msys64/mingw64/include/pixman-1 -IC:/msys64/mingw64/include/gdk-pixbuf-2.0 -IC:/msys64/mingw64/include/libpng16 -IC:/msys64/mingw64/include/webp -IC:/msys64/mingw64/include/atk-1.0 -IC:/msys64/mingw64/include/fribidi -IC:/msys64/mingw64/include/glib-2.0 -IC:/msys64/mingw64/lib/glib-2.0/include -LC:/msys64/mingw64/lib -lgtk-3 -lgdk-3 -lz -lgdi32 -limm32 -lshell32 -lole32 -luuid -lwinmm -ldwmapi -lsetupapi -lcfgmgr32 -lhid -lwinspool -lcomctl32 -lcomdlg32 @libParams
