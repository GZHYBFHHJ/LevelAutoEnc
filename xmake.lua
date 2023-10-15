set_xmakever("2.5.6")
set_project("ABAutoEnc")

set_version("0.1.0", {build = "%Y%m%d%H%M"})


add_rules("mode.minsizerel", "mode.debug")

set_defaultplat("windows")
set_defaultarchs("x86")
set_allowedplats("windows")
set_allowedarchs("x86")


add_requires("libressl")


target("ABAutoEnc")
    set_kind("shared")
    set_optimize("smallest")

    if is_plat("windows") then
        add_cxflags("-GS-", "-MT", { force = true })
    end

    add_links("kernel32", "user32", "ntdll")


    add_packages("libressl")

    add_files("src/dll/**.c")

    add_files("src/dll/proxy/exports.def")
