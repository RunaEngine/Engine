FetchContent_Declare(
        LUAU
        GIT_REPOSITORY https://github.com/luau-lang/luau.git
        TAG 0.705
)

if (NOT LUAU_POPULATED)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    set(LUAU_BUILD_CLI OFF CACHE INTERNAL "")
    set(LUAU_BUILD_TESTS OFF CACHE INTERNAL "")
endif ()

FetchContent_MakeAvailable(LUAU)
set_target_properties(isocline PROPERTIES FOLDER "/engine/thirdparty/luau")
set_target_properties(Luau.Analysis PROPERTIES FOLDER "/engine/thirdparty/luau")
set_target_properties(Luau.Ast PROPERTIES FOLDER "/engine/thirdparty/luau")
set_target_properties(Luau.CLI.lib PROPERTIES FOLDER "/engine/thirdparty/luau")
#set_target_properties(Luau.CLI.Test PROPERTIES FOLDER "/engine/thirdparty/luau")
#set_target_properties(Luau.Conformance PROPERTIES FOLDER "/engine/thirdparty/luau")
set_target_properties(Luau.CodeGen PROPERTIES FOLDER "/engine/thirdparty/luau")
set_target_properties(Luau.Common PROPERTIES FOLDER "/engine/thirdparty/luau")
set_target_properties(Luau.Compiler PROPERTIES FOLDER "/engine/thirdparty/luau")
set_target_properties(Luau.Config PROPERTIES FOLDER "/engine/thirdparty/luau")
# set_target_properties(Luau.EqSat PROPERTIES FOLDER "/engine/thirdparty/luau")
set_target_properties(Luau.Require PROPERTIES FOLDER "/engine/thirdparty/luau")
# set_target_properties(Luau.RequireNavigator PROPERTIES FOLDER "/engine/thirdparty/luau")
#set_target_properties(Luau.UnitTest PROPERTIES FOLDER "/engine/thirdparty/luau")
set_target_properties(Luau.VM PROPERTIES FOLDER "/engine/thirdparty/luau")