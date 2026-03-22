# hipCAS

<p align="center">
<img src="https://github.com/user-attachments/assets/06eaafc2-7bfa-4bff-ab48-646230ddd936"></img>
</p>

[Contrast Adaptive Sharpening (CAS)](https://gpuopen.com/fidelityfx-cas/) is a low overhead adaptive sharpening algorithm with optional up-sampling. The technique is developed by Timothy Lottes (creator of FXAA) and was created to provide natural sharpness without artifacts.

It is used in 3D Graphics frameworks like DX12 and Vulkan, and provides a mixed ability to sharpen and optionally scale an image. **This project implements only the sharpening part**. The algorithm adjusts the amount of sharpening per pixel to target an even level of sharpness across the image. Areas of the input image that are already sharp are sharpened less, while areas that lack detail are sharpened more. This allows for higher overall natural visual sharpness with fewer artifacts. CAS was designed to help increase the quality of existing Temporal Anti-Aliasing (TAA) solutions. TAA often introduces a variable amount of blur due to temporal feedback. The adaptive sharpening provided by CAS is ideal to restore detail in images produced after TAA.

</br>
<p align="center">
  <img width="512" height="288" alt="rocm" src="https://github.com/user-attachments/assets/d50056dd-cef8-4777-ae8f-d5f01b0decf6" />
</p>

This project implements CAS as compute kernel, using AMD HIP (ROCm) in order to execute it on AMD or NVIDIA GPUs. The main reasons for porting CAS to HIP are:
1. General purpose. Because CAS is technically a filter, it can also be used for sharpening static images (like local files from disk). The original CAS filter works only in 3D graphics frameworks.
2. Speed. By implementing the CAS algorithm efficiently in compute frameworks, we can expect major speedups compared to CPU implementations by leveraging the GPU's high performance in parallel problems.

NOTE: This re-implementation is inspired by the [ReShade](https://reshade.me/) project, which also implements CAS for Graphics frameworks.

Τhis repository has two projects:

1. **CAS Implementation**. CAS is implemented as a DLL project and provides a C-style interface for interaction. Here is how you can build and run programs that depend on CAS:
    - For Building:
    Ensure the file ```hipCAS-Lib.dll``` is found either in the same directory as the executable, or available in the system PATH. You must also link against the file ```hipCAS-Lib.lib```, and finally include ```CASLibWrapper.h``` for interacting with the DLL functions.
    - For Running:
    Only the ```hipCAS-Lib.dll``` file is required to be present.
2. **GUI Application**. This simple GUI project aims to showcase how to interact with the CAS DLL in order to sharpen images. It automatically has ```Post-Build Events``` that copy the required DLL, and also link against the DLL's import library (.lib) file, while also including the header file.

## Build

The projects are included in a Visual Studio Solution (```.sln```).
The solution provides multiple build configurations, each targeting a specific backend:

| Configuration    | Backend     | Notes                                       |
|------------------|-------------|---------------------------------------------|
| `AMD_Release`    | AMD clang   | AMD backend. Built by AMD's clang compiler to natively run CUDA code on AMD GPUs. |
| `AMD_Debug`      | AMD clang   | AMD backend (debug build). |
| `CUDA_Release`   | CUDA        | CUDA backend. Built by nvcc. Runs only on NVIDIA GPUs. |
| `CUDA_Debug`     | CUDA        | CUDA backend (debug build). |

1. HIP Windows ROCm SDK v7.1 is used.
2. For the CUDA CAS DLL Implementation, ```CUDA Toolkit 12.4``` is required, in order to link with the CUDA libraries and to include the CUDA header files. Higher versions **are not supported** by ROCm v7.1. The Environment Variable ```CUDA_PATH_V12_4``` should be defined (automatically when installing CUDA toolkit, usually with this default value: ```C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.4```).
3. The Qt GUI application requires Qt MSVC (tested with version 6.8.0) in order to use the Qt framework.
4. When building the GUI project, the tool ```windeployqt``` is called in order to copy the required Qt dependencies for running the application. Also, the DLL is copied in the GUI application's output folder.


## GUI Application usage

1. Launch the application.
2. Use the **Open Image** from the File menu to select an image file from the system.
3. Adjust parameters as desired through the user interface.
4. The sharpening is applied in realtime each time a parameter is changed, to allow the user to view the updated image with various configurations.
5. (Optional) Save the processed image using the **Save Image** from the File menu.

## GUI Samples
Original image            |  Sharpened image
:-------------------------:|:-------------------------:
![hipCAS-GUI_cjNBfnQfcG](https://github.com/user-attachments/assets/d651891a-055a-4b4a-8a0e-ad3491676a63) | ![hipCAS-GUI_ASEq2fDmhG](https://github.com/user-attachments/assets/c8cc9ace-b72f-45a6-b0cb-484f363c18f2)
![hipCAS-GUI_HJeBhN7e6S](https://github.com/user-attachments/assets/3e5b5dcb-65ab-4305-8746-636d16e0065f) | ![hipCAS-GUI_IkltwiEL0s](https://github.com/user-attachments/assets/fd20280b-fbbe-4aba-ab8d-2d8be366dbf6)
