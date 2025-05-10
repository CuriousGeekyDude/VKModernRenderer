# VKModernRenderer
 
# How to build the project

# 1) Install gdown:
- pip install gdown



# 1) Install vcpkg by typing the following commands in cmd or powershell:

-git clone https://github.com/microsoft/vcpkg.git  

-cd vcpkg  

-.\bootstrap-vcpkg.bat

# 2) Integrate vcpkg with Visual Studio:  

.\vcpkg integrate install

# Restart Visual Studio:

Finally restart Visual Studio and you should be able to find vcpkg appearing in Project Properties under C/C++ -> General or Linker -> General. You should turn on the usage of manifest files. If you still cannot see vcpkg in Project's properties then try to refer to the official page of Microsoft regarding this issue. After all these steps the project should build successfully and you should follow the steps discussed above.

# Assets data

The link to the Sponza scene file: https://drive.google.com/drive/folders/1xiEzRC06PI-Ivg7JI6mbctKK9wTozsym?usp=sharing


# Brief explanation as to how to use the software

If it is your first time running the software, then you should try to define CONVERT_SCENE macro in the main.cpp file and compile. Run the program and when it successfully returns, eliminate the macro and recompile again. The renderer is then ready to be used. 



