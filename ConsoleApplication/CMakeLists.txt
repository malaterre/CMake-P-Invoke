cmake_minimum_required(VERSION 3.26)
project(ConsoleApplication LANGUAGES CSharp)

add_executable(ConsoleApplication Program.cs)
set_property(TARGET ConsoleApplication PROPERTY DOTNET_SDK "Microsoft.NET.Sdk")
set_property(TARGET ConsoleApplication PROPERTY DOTNET_TARGET_FRAMEWORK
                                                "net5.0")
