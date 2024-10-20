#include <glad/glad.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <cstdio>
#include <cmath>
#include <vector>

enum Shape { NONE, RECTANGLE, ROUND_RECTANGLE, ELLIPSE, ARC, LINE, CUBIC_CURVE, QUAD_CURVE, POLYGON };
enum Transform { NO_TRANSFORM, TRANSLATION, ROTATION, SCALING, SHEARING, REFLECTION };

Shape currentShape = NONE;
Transform currentTransform = NO_TRANSFORM;

// Shape drawing variables
ImVec2 startPoint(0, 0), endPoint(0, 0);
bool isDrawing = false;
float translationX = 0.0f, translationY = 0.0f;
float rotationAngle = 0.0f;
float scaleX = 1.0f, scaleY = 1.0f;
float shearX = 0.0f, shearY = 0.0f;
bool reflectX = false, reflectY = false;

ImVec2 SubtractImVec2(const ImVec2& a, const ImVec2& b) {
    return ImVec2(a.x - b.x, a.y - b.y);
}

struct ShapeData {
    Shape shape;
    ImVec2 start;
    ImVec2 end;
};

std::vector<ShapeData> shapes;

void DrawShapes() {
    for (const auto& shapeData : shapes) {
        if (shapeData.shape == RECTANGLE) {
            ImGui::GetWindowDrawList()->AddRect(shapeData.start, shapeData.end, IM_COL32(255, 0, 0, 255));
        } else if (shapeData.shape == ROUND_RECTANGLE) {
            ImGui::GetWindowDrawList()->AddRect(shapeData.start, shapeData.end, IM_COL32(255, 165, 0, 255), 10.0f);
        } else if (shapeData.shape == ELLIPSE) {
            ImGui::GetWindowDrawList()->AddEllipse(ImVec2((shapeData.start.x + shapeData.end.x) / 2, (shapeData.start.y + shapeData.end.y) / 2),
                                                   ImVec2(fabs(shapeData.end.x - shapeData.start.x) / 2, fabs(shapeData.end.y - shapeData.start.y) / 2),
                                                   IM_COL32(0, 255, 0, 255), 50);
        } else if (shapeData.shape == ARC) {
            ImGui::GetWindowDrawList()->PathArcTo(shapeData.start, 50.0f, 0.0f, 3.14f);
            ImGui::GetWindowDrawList()->PathStroke(IM_COL32(0, 0, 255, 255), 0, 2.0f);
        } else if (shapeData.shape == LINE) {
            ImGui::GetWindowDrawList()->AddLine(shapeData.start, shapeData.end, IM_COL32(0, 255, 0, 255), 2.0f);
        } else if (shapeData.shape == CUBIC_CURVE) {
            ImGui::GetWindowDrawList()->AddBezierCubic(
                shapeData.start,
                ImVec2((shapeData.start.x + shapeData.end.x) / 2, shapeData.start.y - 50),
                ImVec2((shapeData.start.x + shapeData.end.x) / 2, shapeData.end.y + 50),
                shapeData.end,
                IM_COL32(0, 0, 255, 255), 2.0f
            );
        } else if (shapeData.shape == QUAD_CURVE) {
            ImGui::GetWindowDrawList()->AddBezierQuadratic(
                shapeData.start,
                ImVec2((shapeData.start.x + shapeData.end.x) / 2, shapeData.start.y - 50),
                shapeData.end,
                IM_COL32(255, 255, 0, 255), 2.0f
            );
        } else if (shapeData.shape == POLYGON) {
            ImVec2 topLeft = ImVec2(shapeData.start.x + 30, shapeData.start.y);
            ImVec2 topRight = ImVec2(shapeData.end.x - 30, shapeData.start.y);
            ImVec2 bottomRight = shapeData.end;
            ImVec2 bottomLeft = ImVec2(shapeData.start.x, shapeData.end.y);

            ImVec2 points[4] = { topLeft, topRight, bottomRight, bottomLeft };
            ImGui::GetWindowDrawList()->AddPolyline(points, 4, IM_COL32(255, 255, 0, 255), true, 2.0f);
        }
    }
}

void DrawTransformedRectangle(ImVec2 center, ImVec2 size) {
    ImVec2 points[4] = {
        ImVec2(center.x - size.x / 2, center.y - size.y / 2),
        ImVec2(center.x + size.x / 2, center.y - size.y / 2),
        ImVec2(center.x + size.x / 2, center.y + size.y / 2),
        ImVec2(center.x - size.x / 2, center.y + size.y / 2)
    };

    ImVec2 transformedPoints[4];
    for (int i = 0; i < 4; ++i) {
        transformedPoints[i].x = points[i].x + translationX;
        transformedPoints[i].y = points[i].y + translationY;

        transformedPoints[i].x += shearX * transformedPoints[i].y;
        transformedPoints[i].y += shearY * transformedPoints[i].x;

        float tempX = transformedPoints[i].x - center.x;
        float tempY = transformedPoints[i].y - center.y;
        transformedPoints[i].x = tempX * cos(rotationAngle) - tempY * sin(rotationAngle) + center.x;
        transformedPoints[i].y = tempX * sin(rotationAngle) + tempY * cos(rotationAngle) + center.y;

        transformedPoints[i].x = center.x + (transformedPoints[i].x - center.x) * scaleX;
        transformedPoints[i].y = center.y + (transformedPoints[i].y - center.y) * scaleY;

        if (reflectX) transformedPoints[i].x = 2 * center.x - transformedPoints[i].x;
        if (reflectY) transformedPoints[i].y = 2 * center.y - transformedPoints[i].y;
    }

    ImGui::GetWindowDrawList()->AddPolyline(transformedPoints, 4, IM_COL32(255, 0, 0, 255), true, 2.0f);
}

int main(int, char**) {
    if (!glfwInit()) return -1;
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Shape Drawing and Affine Transforms Program", NULL, NULL);
    glfwMakeContextCurrent(window);
    
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        printf("Failed to initialize GLAD\n");
        return -1;
    }

    glfwSwapInterval(1); 

    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");
    ImGui::StyleColorsDark();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("Choose Objects")) {
                if (ImGui::MenuItem("Rectangle")) currentShape = RECTANGLE;
                if (ImGui::MenuItem("RoundRectangle")) currentShape = ROUND_RECTANGLE;
                if (ImGui::MenuItem("Ellipse")) currentShape = ELLIPSE;
                if (ImGui::MenuItem("Arc")) currentShape = ARC;
                if (ImGui::MenuItem("Line")) currentShape = LINE;
                if (ImGui::MenuItem("CubicCurve")) currentShape = CUBIC_CURVE;
                if (ImGui::MenuItem("QuadCurve")) currentShape = QUAD_CURVE;
                if (ImGui::MenuItem("Polygon")) currentShape = POLYGON;
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Transforms")) {
                if (ImGui::MenuItem("Translation")) currentTransform = TRANSLATION;
                if (ImGui::MenuItem("Rotation")) currentTransform = ROTATION;
                if (ImGui::MenuItem("Scaling")) currentTransform = SCALING;
                if (ImGui::MenuItem("Shearing")) currentTransform = SHEARING;
                if (ImGui::MenuItem("Reflection")) currentTransform = REFLECTION;
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("System")) {
                if (ImGui::MenuItem("Close")) {
                    glfwSetWindowShouldClose(window, true);
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        if (currentTransform == TRANSLATION) {
            ImGui::Begin("Translation Controls");
            ImGui::SliderFloat("Translate X", &translationX, -300.0f, 300.0f);
            ImGui::SliderFloat("Translate Y", &translationY, -300.0f, 300.0f);
            ImGui::End();
        } else if (currentTransform == ROTATION) {
            ImGui::Begin("Rotation Controls");
            ImGui::SliderAngle("Rotate Angle", &rotationAngle);
            ImGui::End();
        } else if (currentTransform == SCALING) {
            ImGui::Begin("Scaling Controls");
            ImGui::SliderFloat("Scale X", &scaleX, 0.1f, 3.0f);
            ImGui::SliderFloat("Scale Y", &scaleY, 0.1f, 3.0f);
            ImGui::End();
        } else if (currentTransform == SHEARING) {
            ImGui::Begin("Shearing Controls");
            ImGui::SliderFloat("Shear X", &shearX, -1.0f, 1.0f);
            ImGui::SliderFloat("Shear Y", &shearY, -1.0f, 1.0f);
            ImGui::End();
        } else if (currentTransform == REFLECTION) {
            ImGui::Begin("Reflection Controls");
            ImGui::Checkbox("Reflect X", &reflectX);
            ImGui::Checkbox("Reflect Y", &reflectY);
            ImGui::End();
        }

        ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2(240, 100), ImGuiCond_Always);
        ImGui::Begin("Drawing Window", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

        ImVec2 center(400, 300);
        ImVec2 size(100, 100);
        DrawTransformedRectangle(center, size);

        ImGui::GetWindowDrawList()->AddLine(ImVec2(center.x - 400, center.y), ImVec2(center.x + 400, center.y), IM_COL32(255, 255, 255, 255), 1.0f);
        ImGui::GetWindowDrawList()->AddLine(ImVec2(center.x, center.y - 300), ImVec2(center.x, center.y + 300), IM_COL32(255, 255, 255, 255), 1.0f);

        if (ImGui::IsMouseClicked(0) && ImGui::IsWindowHovered()) {
            startPoint = SubtractImVec2(ImGui::GetMousePos(), ImGui::GetWindowPos());
            isDrawing = true;
        } else if (ImGui::IsMouseReleased(0) && isDrawing) {
            endPoint = SubtractImVec2(ImGui::GetMousePos(), ImGui::GetWindowPos());
            isDrawing = false;

            shapes.push_back({ currentShape, startPoint, endPoint });
        }

        DrawShapes();
        ImGui::End();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
