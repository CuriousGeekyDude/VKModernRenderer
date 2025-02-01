# VKModernRenderer
 
# How to build the project
You need visual studio 2019 or newer along with vcpkg in order to successfully build this project. If you already have both then you only need to download the project, open the visual studio solution and go to its properties. Make sure under the vcpkg section, the option "Use vcpkg manifest files" is set to yes. Then you compile the project. Finally you need to create a folder in the project's root directory called "Assets" and download the Sponza scene file from the link of the google drive below into "Assets". In the event that you do not have the vcpkg configured, try to follow the following steps:

# 1) Install vcpkg by typing the following commands in cmd or powershell:

-git clone https://github.com/microsoft/vcpkg.git  

-cd vcpkg  

-.\bootstrap-vcpkg.bat

# 2) Integrate vcpkg with Visual Studio:  

.\vcpkg integrate install

# Restart Visual Studio:

Finally restart Visual Studio and you should be able to find vcpkg appearing in Project Properties under C/C++ -> General or Linker -> General. You should turn on the usage of manifest files. If you still cannot see vcpkg in Project's properties then try to refer to the official page of Microsoft regarding this topic. After all these steps the project should build successfully and you should again do the  same as before: create a folder called "Assets" and place the Sponza scene you have downloaded into it and run the program.

# Assets data

The link to the Sponza scene file: https://drive.google.com/drive/folders/1xiEzRC06PI-Ivg7JI6mbctKK9wTozsym?usp=sharing


# Brief explanation as to how to use the software

If it is your first time running the software, then you should try to define CONVERT_SCENE macro and compile. Run the program and when it successfully returns, eliminate the macro and recompile again. The renderer is ready to be used now. 



