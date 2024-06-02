set_xmakever("2.5.6")
set_project("ABAutoEnc")

set_version("0.2.0", {build = "%Y%m%d%H%M"})


add_rules("mode.minsizerel", "mode.debug")

set_defaultplat("mingw")
set_defaultarchs("i386")
set_allowedplats("windows", "mingw")
set_allowedarchs("x86", "i386")

includes("packages/*") -- i am not sure is it a proper way to use bit7z

if is_plat("mingw") then
    add_requires("openssl") -- libressl does not support mingw on XMake
else 
    add_requires("libressl")
end
add_requires("bit7z")


target("ABAutoEnc")
    set_kind("shared")
    set_optimize("smallest")

    if is_plat("mingw") then
        add_shflags("-static-libgcc", "-static-libstdc++", { force = true })
    else
        add_cxflags("-GS-", "-MT", { force = true })
    end

    --add_syslinks("kernel32", "user32")
    add_syslinks("ntdll")
    add_syslinks("uuid", "oleaut32") -- for bit7z


    if is_plat("mingw") then
        add_packages("openssl")
    else 
        add_packages("libressl")
    end
    add_packages("bit7z")

    add_files("src/dll/**.cpp", "src/dll/**.c")

    if is_plat("mingw") then
        add_shflags("src/dll/proxy/exports.def", { force = true })
    else
        add_files("src/dll/proxy/exports.def")
    end

target("injector")
    set_kind("binary")
    set_optimize("smallest")

    if is_plat("mingw") then
        add_ldflags("-static-libgcc", { force = true })
    end

    add_files("src/injector/**.c")


