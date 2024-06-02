package("bit7z")
    set_homepage("https://rikyoz.github.io/bit7z")
    set_description("A C++ static library offering a clean and simple interface to the 7-zip shared libraries.")
    set_license("MPL-2.0")

    add_urls("https://github.com/rikyoz/bit7z/archive/refs/tags/$(version).tar.gz",
             "https://github.com/rikyoz/bit7z.git")

    add_versions("v4.0.7", "19cbefe920a785834f981a5946e448d70d72b6f0adaf62c19c5bf3d03eaf8ee4")

    add_patches("v4.0.7", path.join(os.scriptdir(), "patches", "fix_mingw.patch"), "0cac7caca83e74c2355a2fc5c2a73f553063a252b42179242be42a5765e51dbd")

    add_deps("cmake")

    add_includedirs("include")

    on_load(function (package)
        if package:is_plat("windows", "mingw") then
            package:add("syslinks", "uuid", "oleaut32") --huh doesn't work??
        end
    end)

    on_install(function (package)
        local configs = {}
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:is_debug() and "Debug" or "Release"))
        table.insert(configs, "-DBUILD_SHARED_LIBS=" .. (package:config("shared") and "ON" or "OFF"))

        table.insert(configs, "-DBIT7Z_BUILD_TESTS=OFF")
        table.insert(configs, "-DBIT7Z_BUILD_DOCS=OFF")
        
        import("package.tools.cmake").install(package, configs)

        os.cp("include/*", package:installdir("include"))
        os.cp("lib/*/*.*", package:installdir("lib"))
    end)

    on_test(function (package)
        assert(package:has_cxxtypes("bit7z::BitFileExtractor", {includes = "bit7z/bitfileextractor.hpp"}))
    end)