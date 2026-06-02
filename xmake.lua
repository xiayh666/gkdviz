add_rules("mode.debug", "mode.release")

set_languages("cxx26")
set_warnings("all")

-- Monorepo backend target
-- Frontend (Vite + Node.js + TypeScript) is built via npm scripts under apps/gkdviz-ui.
target("gkdviz-backend")
    set_kind("binary")
    add_files("apps/gkdviz-backend/src/main.cpp")
    add_files("driver/ControlLib/src/controller.cpp")
    add_files("driver/ControlLib/src/pid.cpp")
    add_files("driver/ControlLib/src/user_lib.cpp")
    add_includedirs("driver/ControlLib/include")
    add_defines("BOOST_ASIO_DISABLE_CONCEPTS")
    add_cxxflags("-freflection", {force = true})

target("gkdviz-backend-smoke")
    set_kind("binary")
    add_files("apps/gkdviz-backend/smoke/*.cpp")
    add_files("driver/ControlLib/src/controller.cpp")
    add_files("driver/ControlLib/src/pid.cpp")
    add_files("driver/ControlLib/src/user_lib.cpp")
    add_includedirs("driver/ControlLib/include")
    add_cxxflags("-freflection", {force = true})
