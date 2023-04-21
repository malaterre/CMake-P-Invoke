cmake_minimum_required(VERSION 3.26)
project(CMakePInvoke LANGUAGES CXX CSharp)

add_library(NativeLibrary MODULE NativeLibrary/Source.cpp)

add_executable(ConsoleApplication ConsoleApplication/Program.cs)
set_property(TARGET ConsoleApplication PROPERTY DOTNET_SDK "Microsoft.NET.Sdk")
set_property(TARGET ConsoleApplication PROPERTY DOTNET_TARGET_FRAMEWORK_VERSION
                                                "net5.0")
