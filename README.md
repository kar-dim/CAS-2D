# CAS-2D

<p align="center">
<img src="https://github.com/user-attachments/assets/06eaafc2-7bfa-4bff-ab48-646230ddd936"></img>
</p>

[Contrast Adaptive Sharpening (CAS)](https://gpuopen.com/fidelityfx-cas/) is a low overhead adaptive sharpening algorithm with optional up-sampling. The technique is developed by Timothy Lottes (creator of FXAA) and was created to provide natural sharpness without artifacts.

It is used in 3D Graphics frameworks like DX12 and Vulkan, and provides a mixed ability to sharpen and optionally scale an image. **This project implements only the sharpening part**. The algorithm adjusts the amount of sharpening per pixel to target an even level of sharpness across the image. Areas of the input image that are already sharp are sharpened less, while areas that lack detail are sharpened more. This allows for higher overall natural visual sharpness with fewer artifacts. CAS was designed to help increase the quality of existing Temporal Anti-Aliasing (TAA) solutions. TAA often introduces a variable amount of blur due to temporal feedback. The adaptive sharpening provided by CAS is ideal to restore detail in images produced after TAA.

</br>
<p align="center">
  <img src="https://github.com/user-attachments/assets/670b2932-8c3c-4e6d-88ee-be4f5dae2d28" width="33%">
&nbsp; &nbsp; &nbsp; &nbsp;
  <img src="https://github.com/user-attachments/assets/e92fc01d-50cd-4ab0-ba97-b51e57be47a0" width="33%">
</p>

This project implements CAS as compute kernel, CUDA and OpenCL. The main reasons for porting CAS to these compute frameworks are:
1. General purpose. Because CAS is technically a filter, it can also be used for sharpening static images (like local files from disk). The original CAS filter works only in 3D graphics frameworks.
2. Speed. By implementing the CAS algorithm efficiently in Compute frameworks, we can expect major speedups compared to CPU implementations by leveraging the GPU's high performance in parallel problems.

Τhis repository has two projects:

1. **CAS Implementation**. CAS is implemented as a DLL project and provides a C-style interface for interaction. Here is how you can build and run programs that depend on CAS:
    - For Building:
    Ensure the following files are available in your output directory: ```CAS-2D-Lib.dll``` , ```CAS-2D-Lib.lib``` and ```CASLibWrapper.h``` (for interacting with the DLL)
    - For Running:
    Only the ```CAS-2D-Lib.dll``` is required. It must be either be present in the same directory as the executable, or available in the system PATH.
2. **GUI Application**. This simple GUI project aims to showcase how to interact with the CAS DLL in order to sharpen images.

## Build

The projects are included in a Visual Studio Solution (```.sln```).
The solution provides multiple build configurations, each targeting a specific backend:

| Configuration    | Backend     | Notes                                       |
|------------------|-------------|---------------------------------------------|
| `OPENCL_Release` | OpenCL      | Most recommended backend with very high performance. |
| `OPENCL_Debug`   | OpenCL      | Most recommended backend with very high performance (debug build). |
| `CUDA_Release`   | CUDA        | CUDA backend. Slightly faster than OpenCL, but works only for NVIDIA GPUs. |
| `CUDA_Debug`     | CUDA        | CUDA backend. Slightly faster than OpenCL, but works only for NVIDIA GPUs (debug build). |

1. For the CUDA CAS DLL Implementation, CUDA Toolkit (tested with version 12.9) is required, in order to link with the CUDA libraries and to include the CUDA header files.
2. For the OpenCL CAS DLL Implementation, the relevant [OpenCL Headers](https://github.com/KhronosGroup/OpenCL-Headers), [OpenCL C++ Bindings](https://github.com/KhronosGroup/OpenCL-CLHPP) and [OpenCL Library file](https://github.com/KhronosGroup/OpenCL-SDK) are already included and configured for this project.
   - **NOTE**: Because there may be more than one devices that support OpenCL, the DLL automatically tries to guess the faster device based on some device characteristics.
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
![FidelityFX-CAS-CUDA-GUI_y8DbO9k5dL](https://github.com/user-attachments/assets/5343a3c1-788b-4d42-b5d2-851357001e9c)  |  ![FidelityFX-CAS-CUDA-GUI_UomvduN1Rm](https://github.com/user-attachments/assets/b31b7cfe-1546-4cfc-a677-76d6ca9694dc)
![FidelityFX-CAS-CUDA-GUI_jUkSm8z70Q](https://github.com/user-attachments/assets/d5fb820b-a71e-4425-ac1f-a7b5d6bc3191)  |  ![FidelityFX-CAS-CUDA-GUI_a6DUhPrKd0](https://github.com/user-attachments/assets/0edbc210-55f0-49fd-b2b0-f3a5f62a8f2e)


## Prerequisites/Dependencies

- **NVIDIA GPU**: For the CUDA CAS DLL Implementation, an NVIDIA GPU is required in order to use the CAS DLL. The OpenCL implementation works for most GPU Vendors (NVIDIA, AMD, INTEL).
