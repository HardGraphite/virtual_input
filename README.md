# Virtual Input

**Virtual input**, or **vinput**, is a cross-platform tool
that can generate fake input events according to the script.

## How to build

[CMake](https://cmake.org/) is used as the build system.
Type the following commands to build **vinput**:

```sh
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```

You may need to add option "`--config Release`" to build in release mode
if using multi-config generators like Visual Studio.

## How to use

**vinput** reads script from file or stdin and then execute it.

Type "`vinput --help`" to read available command-line options and switches.
Type "`vinput --help-script`" to learn the syntax of the script.

Here are some demos:

```sh
# Type "Hello, world!".
echo 'Hello, world!' | vinput -s

# Move mouse to (1910, 10) and double click. Then wait for 2 sec and type Esc.
echo '\[@1910,10]\<\< \[#2] \[$ESCAPE]' > script && vinput script

# Type enter for 10 times with an interval of 500 ms.
echo '\[{10] \r \[#0.5] \}' | vinput
```

## Supported platforms

The following platforms are supported until now:

- X11 (Xorg)
- Windows
