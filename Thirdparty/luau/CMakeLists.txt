FetchContent_Declare(
        luaulang
        GIT_REPOSITORY https://github.com/luau-lang/luau.git
        GIT_TAG 0.710
)

set(LUAU_BUILD_CLI OFF CACHE INTERNAL "")
set(LUAU_BUILD_TESTS OFF CACHE INTERNAL "")

FetchContent_MakeAvailable(luaulang)

set_target_properties(isocline PROPERTIES FOLDER "Thirdparty/luau")
set_target_properties(Luau.Analysis PROPERTIES FOLDER "Thirdparty/luau")
set_target_properties(Luau.Ast PROPERTIES FOLDER "Thirdparty/luau")
set_target_properties(Luau.CLI.lib PROPERTIES FOLDER "Thirdparty/luau")
#set_target_properties(Luau.CLI.Test PROPERTIES FOLDER "Thirdparty/luau")
#set_target_properties(Luau.Conformance PROPERTIES FOLDER "Thirdparty/luau")
set_target_properties(Luau.CodeGen PROPERTIES FOLDER "Thirdparty/luau")
set_target_properties(Luau.Common PROPERTIES FOLDER "Thirdparty/luau")
set_target_properties(Luau.Compiler PROPERTIES FOLDER "Thirdparty/luau")
set_target_properties(Luau.Config PROPERTIES FOLDER "Thirdparty/luau")
# set_target_properties(Luau.EqSat PROPERTIES FOLDER "Thirdparty/luau")
set_target_properties(Luau.Require PROPERTIES FOLDER "Thirdparty/luau")
# set_target_properties(Luau.RequireNavigator PROPERTIES FOLDER "Thirdparty/luau")
#set_target_properties(Luau.UnitTest PROPERTIES FOLDER "Thirdparty/luau")
set_target_properties(Luau.VM PROPERTIES FOLDER "Thirdparty/luau")