## Run this 
```
g++ src/main.cpp src/glad.c src/imgui.cpp src/imgui_draw.cpp src/imgui_widgets.cpp src/imgui_demo.cpp src/imgui_impl_glfw.cpp src/imgui_impl_opengl3.cpp src/imgui_tables.cpp -Iinclude -Llib -lglfw3 -lopengl32 -lgdi32 -lm -o imgui_opengl
./imgui_opengl
```