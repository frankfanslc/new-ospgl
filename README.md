# OSPGL
This repo contains all code for the OSPGL program, despite being called 'new-ospgl', as the original ospgl is now legacy code.
## Collaborating
The project has a pretty big scope, so collaboration of all kinds is needed, from planning to coding and artistic work. 
You can check what's needed on the Issues and Projects. Feel free to create new entries with ideas!

---

# Building
We use CMake for building, so the procedure should be similar on all systems.

```
git clone --recurse-submodules https://github.com/TheOpenSpaceProgram/new-ospgl.git
cd new-ospgl
```

## Linux
```
cmake .
cmake --build . 
```
**Note**: If you run cmake in a subdirectory (for example, `build`), then you may want to copy the `compile_commands.json` file to the root directory for code completion in VIM and other tools. 
If your executable is not in the root directory, move it there as the game requires the `res` folder and the `settings.toml` file

## Windows

Open the CMakeLists.txt using Visual Studio. You may need to copy the `launch.vs.json` to the `.vs` folder so
the debugger launches the executable on the correct working directory.

Set target to OSPGL.exe and build.

## MacOS

(Not done yet, I guess CMake will work fine, too)

# Packaging

TODO (We will probably include a python script or something like that to simplify the process)

# Code Style
We don't have a very strict coding-style, but these rules must be followed:
- Brackets on the next line
- Single-line blocks, when appropiate, are fine: `if(a) { return; }`
- Avoid "bracketless" blocks, it's very easy to introduce a hard to find bug with them
- Snake case naming for functions and variables (`my_function`, `current_velocity`, `parse_json`)
    - Feel free to use contractions for common stuff, such as `pos` for `position`, `nrm` for `normal`...
- Camel case naming for classes and file names (`MyClass`, `JSONFile`)
- Use `nullptr` instead of `NULL`
- There is no need to mark member variables with anything (`m_name`, ...), unless it's absolutely neccesary
- Using `auto` is recommended for long types such as iterators, but should not be abused as some editors may not be able to resolve them, making code confusing.
