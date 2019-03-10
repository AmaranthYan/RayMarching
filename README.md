# RayMarching

My personal rendering project inspired by **@miloyip**'s articles on ZhiHu  
link : https://zhuanlan.zhihu.com/p/30745861
## Improvements
Instead of using CPU to calculate light and outputing results to a png file as the author did in his articles, this project use GPU to accelerate computation and display results directly on screen.  
Light calculations are done by fragment shader in GLSL(OpenGL), however the amount of computation power needed for rendering complex scenes (multiple sdf objects with reflection/refraction depth > 3) is beyond the reach of real-time rendering.  

## Techniques

## Result Demo
Scene with one light source and multiple sdf objects
![Result1](https://github.com/AmaranthYan/RayMarching/blob/master/LIGHT2D_sample.png)
