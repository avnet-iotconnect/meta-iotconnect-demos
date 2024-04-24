This is short guide on how to develop the demo on your local machine outside of the yocto environment.


You will need the CMake extension and CMake Tools extension on VScode to use this


```bash
# cd to src/
git submodule update --init --recursive
code main.c
# Control + F5 to compile and execute the application
```

You can set breakpoints and debug the sample as needed.

Extra care must be made when committing changes to make sure that the commit titled "C: INIT LOCAL DEV ENV" is dropped and not transferred to any other branch as this will break the CMakeLists.txt that is used in the yocto build setting