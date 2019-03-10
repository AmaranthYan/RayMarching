# RayMarching 2D Rendering
My personal rendering project inspired by **@miloyip**'s articles on ZhiHu [link](https://zhuanlan.zhihu.com/p/30745861)
## Improvements
Instead of using CPU to calculate light and outputing results to a png file as the author did in his articles, this project use GPU to accelerate computation and display results directly on screen.  
Light calculations are done by fragment shader in GLSL(OpenGL), however the amount of computation power needed for rendering complex **2D**<sup>1</sup> sdf scenes (multiple sdf objects with reflection/refraction depth > 3) is beyond the reach of real-time rendering.  
Thus the rendering of one result image is seperated into multiple iterations each draws a portion of the frame to the frame buffer object, but we can always trade lower rendering quality for less drawing time. 
## Techniques
* OpenGL + GLSL
* SDF objects
* Reflection + Refraction + Fresnel-Schlik + Beer-Lambert
* RGB color support
## Result Demo
Scene with one light source and multiple sdf objects
![Result1](https://github.com/AmaranthYan/RayMarching/blob/master/LIGHT2D_sample.png)

<sup>1</sup> But not a problem for 3D scenes, while rendering 3D sdf traces only 1 ray per pixel, 2D rendering requires at least hundreds of rays (128, 256 or more) per pixel to get a good result.
