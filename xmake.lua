set_xmakever("2.5.6")
set_project("ABAutoEnc")

set_version("0.1.0", {build = "%Y%m%d%H%M"})


add_rules("mode.minsizerel", "mode.debug")

set_defaultplat("mingw")
set_defaultarchs("i386")
set_allowedplats("windows", "mingw")
set_allowedarchs("x86", "i386")

if is_plat("mingw") then
    add_requires("openssl") -- libressl does not support mingw on XMake
else 
    add_requires("libressl")
end


target("ABAutoEnc")
    set_kind("shared")
    set_optimize("smallest")

    if is_plat("mingw") then
        add_shflags("-static-libgcc", { force = true })
    else
        add_cxflags("-GS-", "-MT", { force = true })
    end

    add_links("kernel32", "user32", "ntdll")


    if is_plat("mingw") then
        add_packages("openssl")
    else 
        add_packages("libressl")
    end

    add_files("src/dll/**.c")

    if is_plat("mingw") then
        add_shflags("src/dll/proxy/exports.def", { force = true })
    else
        add_files("src/dll/proxy/exports.def")
    end

    
