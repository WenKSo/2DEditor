// This example is heavily based on the tutorial at https://open.gl
#include <iostream>
#include <fstream>
// OpenGL Helpers to reduce the clutter
#include "Helpers.h"

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
// GLFW is necessary to handle the OpenGL context
#include <GLFW/glfw3.h>
#else
// GLFW is necessary to handle the OpenGL context
#include <GLFW/glfw3.h>
#endif

// Linear Algebra Library
#include <Eigen/Core>
#include <Eigen/Dense>
#include <math.h>

// Timer
#include <chrono>

#define PI 3.14159265

// Shortcut to avoid Eigen:: and std:: everywhere, DO NOT USE IN .h
using namespace std;
using namespace Eigen;

// VertexBufferObject wrapper
VertexBufferObject VBO;
VertexBufferObject VBO_C;

// Contains the vertex positions
MatrixXf V(2,1);
MatrixXf C(3,0);

// Contains the view transformation
Matrix4f view(4,4);

int counter = 0; // Click Counter
int currentMode = 0;
int currentTriangle = -1;
int animationTriangle = -1;
int vertexToPaint = -1;
int scaleTrigger = 0;
int transX = 0;
int transY = 0;

double cursorPosX;
double cursorPosY;

Program* p;

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void exportSVG(){
    ofstream myfile;
    myfile.open ("2DEditor.svg");
    //cout << "hello" << endl;
    myfile << 
    "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" baseProfile=\"full\" width=\"100%\" height=\"100%\">" << endl; 
    for(int i = 0; i < V.cols()-1; i=i+3){
        Vector2f a = V.col(i);
        Vector2f b = V.col(i+1);
        Vector2f c = V.col(i+2);
        myfile << "<polygon points=\""
        << a(0) << ", " << a(1) << " " << b(0) << ", " << b(1) << " " << b(0) << ", " << b(1)
        << "\" style= \"fill:red\" />" << endl;
    }
    myfile << "</svg>" << endl;
    
    myfile.close();
}

void insertionMode_Mouse(GLFWwindow* window, int button, int action, int mods){
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    int width, height;
    glfwGetWindowSize(window, &width, &height);
    //cout << width << " " << height << endl;
    double xworld = ((xpos/double(width))*2)-1;
    double yworld = (((height-1-ypos)/double(height))*2)-1;
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS){
        int m = counter%3;
        switch(m){
            case 0:
                V.conservativeResize(2,V.cols()+1);
                C.conservativeResize(3,C.cols()+1);
                V.col(counter) << xworld, yworld;
                V.col(counter+1) << xworld, yworld;
                C.col(counter) << 0,0,0;
                break;
            case 1:
                V.conservativeResize(NoChange,V.cols()+1);
                C.conservativeResize(3,C.cols()+1);
                C.col(counter) << 0,0,0;
                V.col(counter) << xworld, yworld;
                V.col(counter+1) << xworld, yworld;
                break;
            case 2:
                V.conservativeResize(NoChange,V.cols()+1);
                C.conservativeResize(3,C.cols()+1);
                V.col(counter) << xworld, yworld;
                C.col(counter) << 1,0,0;
                C.col(counter-1) << 1,0,0;
                C.col(counter-2) << 1,0,0;
                break;
            default:
                break;
        }
        // Upload the change to the GPU
        VBO.update(V);
        VBO_C.update(C);
        counter++;
    }
}

bool isInside(Vector2f p,Vector2f a,Vector2f b,Vector2f c){
    Matrix2f A;
    Vector2f r;
    A << a(0)-b(0),a(0)-c(0),
         a(1)-b(1),a(1)-c(1);
    r << a(0)-p(0),
         a(1)-p(1);
    Vector2f x = A.colPivHouseholderQr().solve(r);
    float s = x(0);
    float t = x(1);
    //cout << "s&t: " << s << " " << t << endl;
    if(s>=0 && t>=0 && s+t<=1) return true;
    return false; 
}

int getCurrentTriangle(double& x, double& y){
    //cout << V.cols() << endl;
    for(int i = 0; i < V.cols()-1; i=i+3){
        Vector2f p;
        p << x, y;
        Vector2f a = V.col(i);
        Vector2f b = V.col(i+1);
        Vector2f c = V.col(i+2);
        if(isInside(p,a,b,c)) return i;
    }
    //cout << "No!" << endl;
    return -1;
}

void translationMode_Mouse(GLFWwindow* window, int button, int action, int mods){
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    int width, height;
    glfwGetWindowSize(window, &width, &height);

    double xworld = ((xpos/double(width))*2)-1;
    double yworld = (((height-1-ypos)/double(height))*2)-1;
    int o = getCurrentTriangle(xworld,yworld);
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && o>=0){
        //cout << "Yay!" << endl;
        cursorPosX = xworld;
        cursorPosY = yworld;
        currentTriangle = o;
        C.col(o) << 0,0,1;
        C.col(o+1) << 0,0,1;
        C.col(o+2) << 0,0,1;
        VBO_C.update(C);
    }
}

void removeVector(unsigned int colToRemove)
{
    unsigned int numRows = V.rows();
    unsigned int numCols = V.cols()-3;
    unsigned int numCRows = C.rows();
    unsigned int numCCols = C.cols()-3;
    if( colToRemove < numCols ){
        // cout << "colToRemove: " << colToRemove << endl;
        // cout << "colToRemove: " << colToRemove << endl;
        // cout << "colToRemove: " << colToRemove << endl;
        V.block(0,colToRemove,numRows,numCols-colToRemove+1) = V.rightCols(numCols-colToRemove+1);
        C.block(0,colToRemove,numCRows,numCCols-colToRemove) = C.rightCols(numCCols-colToRemove);
        }
    V.conservativeResize(NoChange,numCols);
}

void deleteMode_Mouse(GLFWwindow* window, int button, int action, int mods){
  double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    double xworld = ((xpos/double(width))*2)-1;
    double yworld = (((height-1-ypos)/double(height))*2)-1;
    int o = getCurrentTriangle(xworld,yworld);
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && o>=0){
        currentTriangle = o;
        removeVector(o);
        VBO.update(V);
    }
}

int getClosestVertex(double& x, double& y){
    double min=100;
    double temp;
    int result;
    for (int i=0;i<V.cols()-1;i++){
        //cout << "Index: " << i << " " << temp << endl;
        temp = (V(0,i)-x)*(V(0,i)-x)+(V(1,i)-y)*(V(1,i)-y);
        if(temp<min) {
            min=temp;
            result = i;
        }
    }
    return result;
}

void colorMode_Mouse(GLFWwindow* window, int button, int action, int mods){
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    int width, height;
    glfwGetWindowSize(window, &width, &height);

    double xworld = ((xpos/double(width))*2)-1;
    double yworld = (((height-1-ypos)/double(height))*2)-1;
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS){
        vertexToPaint = getClosestVertex(xworld,yworld);
    }
}
void rotate(float r){
    //cout << "debug2" << endl;
    Vector2f centroid;
    centroid <<  (V(0,currentTriangle)+V(0,currentTriangle+1)+V(0,currentTriangle+2))/3,
                (V(1,currentTriangle)+V(1,currentTriangle+1)+V(1,currentTriangle+2))/3;
    //cout << "debug3" << endl;
    double x1 = V(0,currentTriangle);
    double y1 = V(1,currentTriangle);
    double x2 = V(0,currentTriangle+1);
    double y2 = V(1,currentTriangle+1);
    double x3 = V(0,currentTriangle+2);
    double y3 = V(1,currentTriangle+2);

    float x = r*PI/180;
    //cout << "debug5" << endl;
    V(0,currentTriangle) = centroid(0) + (x1-centroid(0))*cos(x) - (y1-centroid(1))*sin(x);
    V(1,currentTriangle) = centroid(1) + (x1-centroid(0))*sin(x) + (y1-centroid(1))*cos(x);
    V(0,currentTriangle+1) = centroid(0) + (x2-centroid(0))*cos(x) - (y2-centroid(1))*sin(x);
    V(1,currentTriangle+1) = centroid(1) + (x2-centroid(0))*sin(x) + (y2-centroid(1))*cos(x);
    V(0,currentTriangle+2) = centroid(0) + (x3-centroid(0))*cos(x) - (y3-centroid(1))*sin(x);
    V(1,currentTriangle+2) = centroid(1) + (x3-centroid(0))*sin(x) + (y3-centroid(1))*cos(x);
    //cout << "debug5" << endl;
    VBO.update(V);
}

void keyframeMode_Mouse(GLFWwindow* window, int button, int action, int mods){
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    int width, height;
    glfwGetWindowSize(window, &width, &height);

    double xworld = ((xpos/double(width))*2)-1;
    double yworld = (((height-1-ypos)/double(height))*2)-1;
    int o = getCurrentTriangle(xworld,yworld);
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && o>=0){
        currentTriangle = o;
        animationTriangle = o;
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    switch(currentMode){
        case 1:
            insertionMode_Mouse(window,button,action,mods);
            break;
        case 2:
            translationMode_Mouse(window,button,action,mods);
            break;
        case 3:
            deleteMode_Mouse(window,button,action,mods);
            break;
        case 4:
            colorMode_Mouse(window,button,action,mods);
            break;
        case 5:
            keyframeMode_Mouse(window,button,action,mods);
            break;
    }
}


void insertionMode_Cursor(GLFWwindow* window, double xpos, double ypos){
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    double xworld = ((xpos/double(width))*2)-1;
    double yworld = (((height-1-ypos)/double(height))*2)-1;
    switch(counter%3){
        case 1:
            //if(counter >= V.cols()) V.conservativeResize(Eigen::NoChange,V.cols()+1);
            V.col(counter) << xworld, yworld;
            break;
        case 2:
            //if(counter >= V.cols()) V.conservativeResize(Eigen::NoChange,V.cols()+1);
            V.col(counter) << xworld, yworld;
            break;
        default:
            break;
    }
    VBO.update(V);
}

void translationMode_Cursor(GLFWwindow* window, double xpos, double ypos){
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    double xworld = ((xpos/double(width))*2)-1;
    double yworld = (((height-1-ypos)/double(height))*2)-1;
    int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
    //cout << "debug1" << endl;
    if (state == GLFW_PRESS && getCurrentTriangle(xworld,yworld)>=0){
        //cout << "debug2" << endl;
        double xDisp = xworld - cursorPosX;
        double yDisp = yworld - cursorPosY;
        //cout << "Disp: "<< xDisp << " " << yDisp << endl;      
        V(0,currentTriangle) += xDisp;
        V(1,currentTriangle) += yDisp;
        V(0,currentTriangle+1) += xDisp;
        V(1,currentTriangle+1) += yDisp;
        V(0,currentTriangle+2) += xDisp;
        V(1,currentTriangle+2) += yDisp;
        cursorPosX = xworld;
        cursorPosY = yworld;
        VBO.update(V);
        //glUniform3f((*p).uniform("triangleColor"), 0.0f, 0.0f, 1.0f);
        //cout << "debug3" << endl;
    }
    if (state == GLFW_RELEASE && currentTriangle != -1){
        // cout << "C cols: " << C.cols() << endl;
        // cout << "currentT: " << currentTriangle << endl;
        C.col(currentTriangle) << 1,0,0;
        C.col(currentTriangle+1) << 1,0,0;
        C.col(currentTriangle+2) << 1,0,0;
        VBO_C.update(C);
    }
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos){
    switch(currentMode){
        case 1:
            insertionMode_Cursor(window,xpos,ypos);
            break;
        case 2:
            translationMode_Cursor(window,xpos,ypos);
            break;
        case 3:
            break;
    }
}

void scale(float p){
    Vector2f centroid;
    centroid <<  (V(0,currentTriangle)+V(0,currentTriangle+1)+V(0,currentTriangle+2))/3,
                (V(1,currentTriangle)+V(1,currentTriangle+1)+V(1,currentTriangle+2))/3;
    double x1 = V(0,currentTriangle);
    double y1 = V(1,currentTriangle);
    double x2 = V(0,currentTriangle+1);
    double y2 = V(1,currentTriangle+1);
    double x3 = V(0,currentTriangle+2);
    double y3 = V(1,currentTriangle+2);
    V(0,currentTriangle) = centroid(0)+(x1 - centroid(0))*(1+p);
    V(1,currentTriangle) = centroid(1)+(y1 - centroid(1))*(1+p);
    V(0,currentTriangle+1) = centroid(0)+(x2 - centroid(0))*(1+p);
    V(1,currentTriangle+1) = centroid(1)+(y2 - centroid(1))*(1+p);
    V(0,currentTriangle+2) = centroid(0)+(x3 - centroid(0))*(1+p);
    V(1,currentTriangle+2) = centroid(1)+(y3 - centroid(1))*(1+p);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    switch (key)
    {
        case  GLFW_KEY_I:
            currentMode = 1;
            break;
        case GLFW_KEY_O:
            currentMode = 2;
            break;
        case  GLFW_KEY_P:
            currentMode = 3;
            break;
        case  GLFW_KEY_C:
            currentMode = 4;
            break;
        case GLFW_KEY_H:
            if(currentMode==2) rotate(10);
            break;
        case GLFW_KEY_J:
            if(currentMode==2) rotate(-10);
            break;
        case GLFW_KEY_K:
            if(currentMode==2) scale(0.25);
            break;
        case GLFW_KEY_L:
            if(currentMode==2) scale(-0.25);
            break;
        case GLFW_KEY_1:
            if(currentMode==4) {
                C.col(vertexToPaint) << 0,1,0;
                VBO_C.update(C);
            }
            break;
        case GLFW_KEY_2:
            if(currentMode==4) {
                C.col(vertexToPaint) << 0.5,0.5,0;
                VBO_C.update(C);
            }
            break;
        case GLFW_KEY_3:
            if(currentMode==4) {
                C.col(vertexToPaint) << 0,0.5,0.5;
                VBO_C.update(C);
            }
            break;
        case GLFW_KEY_4:
            if(currentMode==4) {
                C.col(vertexToPaint) << 0.5,0,0.5;
                VBO_C.update(C);
            }
            break;
        case GLFW_KEY_5:
            if(currentMode==4) {
                C.col(vertexToPaint) << 0.7,0.3,0.4;
                VBO_C.update(C);
            }
            break;
        case GLFW_KEY_6:
            if(currentMode==4) {
                C.col(vertexToPaint) << 0.7,0.9,0.2;
                VBO_C.update(C);
            }
            break;
        case GLFW_KEY_7:
            if(currentMode==4) {
                C.col(vertexToPaint) << 0.1,0.3,0.8;
                VBO_C.update(C);
            }
            break;
        case GLFW_KEY_8:
            if(currentMode==4) {
                C.col(vertexToPaint) << 0.2,0.5,0.2;
                VBO_C.update(C);
            }
            break;
        case GLFW_KEY_9:
            if(currentMode==4) {
                C.col(vertexToPaint) << 0,0,1;
                VBO_C.update(C);
            }
            break;
        case GLFW_KEY_EQUAL:
            scaleTrigger++;
            break;
        case GLFW_KEY_MINUS:
            if(scaleTrigger > -5) scaleTrigger--;
            break;
        case GLFW_KEY_W:
            transY++;
            break;
        case GLFW_KEY_A:
            transX--;
            break;
        case GLFW_KEY_S:
            transY--;
            break;
        case GLFW_KEY_D:
            transX++;
            break;
        case GLFW_KEY_F:
            // Keyframe Mode
            currentMode = 5;
            break;
        case GLFW_KEY_M:
            exportSVG();
            break;
        default:
            break;
    }
    // Upload the change to the GPU
    VBO.update(V);
}

void draw(int numOfVertices, GLenum mode, Program& program){
    for(int i = 0; i < numOfVertices; i=i+3){
        if(numOfVertices-i<3){
            switch(mode){
                case GL_LINES:
                    glDrawArrays(mode,i,2);
                    break;
                case GL_LINE_LOOP:
                    glDrawArrays(mode,i,3);
                    break;
                case GL_TRIANGLES:
                    glDrawArrays(mode,i,3);
                    break;   
            }
        }
        else{
            glDrawArrays(GL_TRIANGLES,i,3); 
        }
    }
}

int main(void)
{
    GLFWwindow* window;

    // Initialize the library
    if (!glfwInit())
        return -1;

    // Activate supersampling
    glfwWindowHint(GLFW_SAMPLES, 8);

    // Ensure that we get at least a 3.2 context
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);

    // On apple we have to load a core profile with forward compatibility
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Create a windowed mode window and its OpenGL context
    window = glfwCreateWindow(640, 480, "2D Editor", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    // Make the window's context current
    glfwMakeContextCurrent(window);

    #ifndef __APPLE__
      glewExperimental = true;
      GLenum err = glewInit();
      if(GLEW_OK != err)
      {
        /* Problem: glewInit failed, something is seriously wrong. */
       fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
      }
      glGetError(); // pull and savely ignonre unhandled errors like GL_INVALID_ENUM
      fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));
    #endif

    int major, minor, rev;
    major = glfwGetWindowAttrib(window, GLFW_CONTEXT_VERSION_MAJOR);
    minor = glfwGetWindowAttrib(window, GLFW_CONTEXT_VERSION_MINOR);
    rev = glfwGetWindowAttrib(window, GLFW_CONTEXT_REVISION);
    printf("OpenGL version recieved: %d.%d.%d\n", major, minor, rev);
    printf("Supported OpenGL is %s\n", (const char*)glGetString(GL_VERSION));
    printf("Supported GLSL is %s\n", (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION));

    // Initialize the VAO
    // A Vertex Array Object (or VAO) is an object that describes how the vertex
    // attributes are stored in a Vertex Buffer Object (or VBO). This means that
    // the VAO is not the actual object storing the vertex data,
    // but the descriptor of the vertex data.
    VertexArrayObject VAO;
    VAO.init();
    VAO.bind();

    // Initialize the VBO with the vertices data
    // A VBO is a data container that lives in the GPU memory
    VBO.init();
    VBO.update(V);

    // Second VBO for colors
    VBO_C.init();

    VBO_C.update(C);

    // Initialize the OpenGL Program
    // A program controls the OpenGL pipeline and it must contains
    // at least a vertex shader and a fragment shader to be valid
    Program program;
    const GLchar* vertex_shader =
            "#version 150 core\n"
                    "in vec2 position;"
                    "in vec3 color;"
                    "uniform mat4 view;"
                    "out vec3 f_color;"
                    "void main()"
                    "{"
                    "    gl_Position = view * vec4(position, 0.0, 1.0);"
                    "    f_color = color;"
                    "}";
    const GLchar* fragment_shader =
            "#version 150 core\n"
                    "in vec3 f_color;"
                    "out vec4 outColor;"
                    "uniform vec3 triangleColor;"
                    "void main()"
                    "{"
                    "    outColor = vec4(f_color, 1.0);"
                    "}";

    // Compile the two shaders and upload the binary to the GPU
    // Note that we have to explicitly specify that the output "slot" called outColor
    // is the one that we want in the fragment buffer (and thus on screen)
    program.init(vertex_shader,fragment_shader,"outColor");
    program.bind();
    p = &program;
    // The vertex shader wants the position of the vertices as an input.
    // The following line connects the VBO we defined above with the position "slot"
    // in the vertex shader
    program.bindVertexAttribArray("position",VBO);
    program.bindVertexAttribArray("color",VBO_C);

    // Save the current time --- it will be used to dynamically change the triangle color
    // auto t_start = std::chrono::high_resolution_clock::now();

    // Register the keyboard callback
    glfwSetKeyCallback(window, key_callback);

    // Register the mouse callback
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window,cursor_position_callback);

    // Update viewport
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Loop until the user closes the window
    while (!glfwWindowShouldClose(window))
    {
        // Bind your VAO (not necessary if you have only one)
        VAO.bind();

        // Bind your program
        program.bind();

        // Set the uniform value depending on the time difference
        // auto t_now = std::chrono::high_resolution_clock::now();
        // float time = std::chrono::duration_cast<std::chrono::duration<float> >(t_now - t_start).count();
        //glUniform3f(program.uniform("triangleColor"), 1.0f, 0.0f, 0.0f);
        
        // Get size of the window
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        float aspect_ratio = float(height)/float(width); // corresponds to the necessary width scaling
        
        view <<
        1+scaleTrigger*0.2,0, 0, transX*0.2,
        0,           1+scaleTrigger*0.2, 0, transY*0.2,
        0,           0, 1, 0,
        0,           0, 0, 1;
        glUniformMatrix4fv(program.uniform("view"), 1, GL_FALSE, view.data());
        // Clear the framebuffer
        glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        switch(currentMode){
            case 1:
                if(counter%3 == 1) draw(V.cols()-1,GL_LINES,program);
                if(counter%3 == 2) draw(V.cols()-1,GL_LINE_LOOP,program);
                if(counter%3 == 0 && counter != 0) draw(V.cols()-1,GL_TRIANGLES,program);
                break;
            case 2:
                draw(V.cols()-1,GL_TRIANGLES,program);
                break;
            case 3:
                draw(V.cols()-1,GL_TRIANGLES,program);
                break;
            case 4:
                draw(V.cols()-1,GL_TRIANGLES,program);
                break;
            case 5:
                if(animationTriangle!= -1){
                    rotate(1);
                    //VBO.update(V);
                }
                draw(V.cols()-1,GL_TRIANGLES,program);
                break;
            default:
                break;
        }
        // Draw a triangle
        // Swap front and back buffers
        glfwSwapBuffers(window);

        // Poll for and process events
        glfwPollEvents();
    }

    // Deallocate opengl memory
    program.free();
    VAO.free();
    VBO.free();

    // Deallocate glfw internals
    glfwTerminate();
    return 0;
}
