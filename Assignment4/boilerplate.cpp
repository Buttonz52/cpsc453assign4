// ==========================================================================
// Barebones OpenGL Core Profile Boilerplate
//    using the GLFW windowing system (http://www.glfw.org)
//
// Loosely based on
//  - Chris Wellons' example (https://github.com/skeeto/opengl-demo) and
//  - Camilla Berglund's example (http://www.glfw.org/docs/latest/quick.html)
//
// Author:  Sonny Chan, University of Calgary
// Date:    December 2015
// ==========================================================================

#include <iostream>
#include <fstream>
#include <string>
#include <iterator>
#include <algorithm>
#include <vector>
#include <sstream>

// specify that we want the OpenGL core profile before including GLFW headers
#include <glad/glad.h>
#include "imageBuffer.h"
#include <GLFW/glfw3.h>
#include <glm\glm.hpp>

using namespace glm;
using namespace std;

struct light
{
	vec3 position;
	float intensity;
};

struct sphere
{
	vec3 center;
	float radius;
	vec3 color;
};

struct plane
{
	vec3 normal;
	vec3 position;
	vec3 color;
};

struct triangle
{
	vec3 P0;
	vec3 P1;
	vec3 P2;
	vec3 color;
};

struct ray
{
	vec3 origin;
	vec3 direction;
};

vector<light> lights;
vector<sphere> spheres;
vector<plane> planes;
vector<triangle> triangles;
int windowX = 512;
int windowY = 512;
float PI = 3.14159265;
int degree = 60;
float FoV = degree * PI/180; //in radians
int scene = 1;

//function declarations

void loadAllObjects(string filename);
float max(float a, float b);
vec3 Phong(light light, vec3 point, vec3 normal, vec3 color, ray r, bool draw);
vec4 intersectSphere(ray r, sphere sphere, light light);
vec4 intersectPlane(ray ray, plane plane, light light);
vec4 intersectTriangle(ray ray, triangle tri, light light);

// --------------------------------------------------------------------------
// OpenGL utility and support function prototypes

void QueryGLVersion();
bool CheckGLErrors();

string LoadSource(const string &filename);
GLuint CompileShader(GLenum shaderType, const string &source);
GLuint LinkProgram(GLuint vertexShader, GLuint fragmentShader);

// --------------------------------------------------------------------------
// Functions to set up OpenGL shader programs for rendering

struct MyShader
{
    // OpenGL names for vertex and fragment shaders, shader program
    GLuint  vertex;
    GLuint  fragment;
    GLuint  program;

    // initialize shader and program names to zero (OpenGL reserved value)
    MyShader() : vertex(0), fragment(0), program(0)
    {}
};

// load, compile, and link shaders, returning true if successful
bool InitializeShaders(MyShader *shader)
{
    // load shader source from files
    string vertexSource = LoadSource("vertex.glsl");
    string fragmentSource = LoadSource("fragment.glsl");
    if (vertexSource.empty() || fragmentSource.empty()) return false;

    // compile shader source into shader objects
    shader->vertex = CompileShader(GL_VERTEX_SHADER, vertexSource);
    shader->fragment = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);

    // link shader program
    shader->program = LinkProgram(shader->vertex, shader->fragment);

    // check for OpenGL errors and return false if error occurred
    return !CheckGLErrors();
}

// deallocate shader-related objects
void DestroyShaders(MyShader *shader)
{
    // unbind any shader programs and destroy shader objects
    glUseProgram(0);
    glDeleteProgram(shader->program);
    glDeleteShader(shader->vertex);
    glDeleteShader(shader->fragment);
}

// --------------------------------------------------------------------------
// Functions to set up OpenGL buffers for storing geometry data

struct MyGeometry
{
    // OpenGL names for array buffer objects, vertex array object
    GLuint  vertexBuffer;
    GLuint  colourBuffer;
    GLuint  vertexArray;
    GLsizei elementCount;

    // initialize object names to zero (OpenGL reserved value)
    MyGeometry() : vertexBuffer(0), colourBuffer(0), vertexArray(0), elementCount(0)
    {}
};

// create buffers and fill with geometry data, returning true if successful
bool InitializeGeometry(MyGeometry *geometry)
{
    // three vertex positions and assocated colours of a triangle
    const GLfloat vertices[][2] = {
        { -0.6, -0.4 },
        {  0.6, -0.4 },
        {  0.0,  0.6 }
    };
    const GLfloat colours[][3] = {
        { 1.0, 0.0, 0.0 },
        { 0.0, 1.0, 0.0 },
        { 0.0, 0.0, 1.0 }
    };
    geometry->elementCount = 3;

    // these vertex attribute indices correspond to those specified for the
    // input variables in the vertex shader
    const GLuint VERTEX_INDEX = 0;
    const GLuint COLOUR_INDEX = 1;

    // create an array buffer object for storing our vertices
    glGenBuffers(1, &geometry->vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, geometry->vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // create another one for storing our colours
    glGenBuffers(1, &geometry->colourBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, geometry->colourBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(colours), colours, GL_STATIC_DRAW);

    // create a vertex array object encapsulating all our vertex attributes
    glGenVertexArrays(1, &geometry->vertexArray);
    glBindVertexArray(geometry->vertexArray);

    // associate the position array with the vertex array object
    glBindBuffer(GL_ARRAY_BUFFER, geometry->vertexBuffer);
    glVertexAttribPointer(VERTEX_INDEX, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(VERTEX_INDEX);

    // assocaite the colour array with the vertex array object
    glBindBuffer(GL_ARRAY_BUFFER, geometry->colourBuffer);
    glVertexAttribPointer(COLOUR_INDEX, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(COLOUR_INDEX);

    // unbind our buffers, resetting to default state
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // check for OpenGL errors and return false if error occurred
    return !CheckGLErrors();
}

// deallocate geometry-related objects
void DestroyGeometry(MyGeometry *geometry)
{
    // unbind and destroy our vertex array object and associated buffers
    glBindVertexArray(0);
    glDeleteVertexArrays(1, &geometry->vertexArray);
    glDeleteBuffers(1, &geometry->vertexBuffer);
    glDeleteBuffers(1, &geometry->colourBuffer);
}

// --------------------------------------------------------------------------
// GLFW callback functions

// reports GLFW errors
void ErrorCallback(int error, const char* description)
{
    cout << "GLFW ERROR " << error << ":" << endl;
    cout << description << endl;
}

// handles keyboard input events
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
	if (key == GLFW_KEY_1 && action == GLFW_PRESS)
		scene = 1;
	if (key == GLFW_KEY_2 && action == GLFW_PRESS)
		scene = 2;
	if (key == GLFW_KEY_3 && action == GLFW_PRESS)
		scene = 3;
}

// ==========================================================================
// PROGRAM ENTRY POINT

//This takes a file and files vector arrays with objects
bool anyIntersect(ray r);

void loadAllObjects(string filename)
{
	ifstream file(filename);
	string line;
	ifstream line2;
	string data;
	vector<string> words;
	int i = 0;

	while (getline(file, line, ' '))
	{
		line.erase(std::remove(line.begin(), line.end(), '\n'), line.end());
		line.erase(std::remove(line.begin(), line.end(), '}'), line.end());
		line.erase(std::remove(line.begin(), line.end(), '{'), line.end());
		line.erase(std::remove(line.begin(), line.end(), '#'), line.end());
		words.push_back(line);
	}
	
	while (i < words.size())	
	{
		if (!words.at(i).find("light"))
		{
			light l;
			l.position = vec3(stof(words.at(i + 3)), stof(words.at(i + 4)), stof(words.at(i + 5)));
			l.intensity = stof(words.at(i + 7));
			lights.push_back(l);
		}

		if (!words.at(i).find("sphere"))
		{
			sphere s;
			s.center = vec3(stof(words.at(i + 3)), stof(words.at(i + 4)), stof(words.at(i + 5)));
			s.radius = stof(words.at(i + 7));
			s.color = vec3(stof(words.at(i + 9)), stof(words.at(i + 10)), stof(words.at(i + 11)));
			spheres.push_back(s);
		}

		if (!words.at(i).find("triangle"))
		{
			triangle t;
			t.P0 = vec3(stof(words.at(i + 3)), stof(words.at(i + 4)), stof(words.at(i + 5)));
			t.P1 = vec3(stof(words.at(i + 7)), stof(words.at(i + 8)), stof(words.at(i + 9)));
			t.P2 = vec3(stof(words.at(i + 11)), stof(words.at(i + 12)), stof(words.at(i + 13)));
			t.color = vec3(stof(words.at(i + 15)), stof(words.at(i + 16)), stof(words.at(i + 17)));
			triangles.push_back(t);
		}

		if (!words.at(i).find("plane"))
		{
			plane p;
			p.normal = vec3(stof(words.at(i + 3)), stof(words.at(i + 4)), stof(words.at(i + 5)));
			p.position = vec3(stof(words.at(i + 7)), stof(words.at(i + 8)), stof(words.at(i + 9)));
			p.color = vec3(stof(words.at(i + 11)), stof(words.at(i + 12)), stof(words.at(i + 13)));
			planes.push_back(p);
		}
		i++;
	}
}

float max(float a, float b)
{
	if (a >= b)
		return a;
	else
		return b;
}

vec3 Phong(light light, vec3 point, vec3 normal, vec3 color, ray r, bool draw)
{
	vec3 l = normalize(light.position - point);
	vec3 v = normalize(r.origin - point);
	vec3 n = normalize(normal);

	vec3 h = normalize(v + l);

	vec3 ka = color;
	vec3 kd = ka;
	vec3 ks = vec3(0.7,0.7,0.7);
	float Ia = 0.2;
	float I = light.intensity;
	int exp = 16;

	vec3 ambient = ka * Ia;
	vec3 diffuse = kd * I * max(0, dot(n, l));
	vec3 specular = ks * I * pow(max(0, dot(n, h)), exp);
	vec3 L = ambient + diffuse + specular;

	if (draw == false)
		return ambient + diffuse;
	else
		return L;
}

vec4 intersectSphere(ray r, sphere sphere, light light)
{
	//calculate quadratic variables
	float a = dot(r.direction, r.direction);
	float b = (2 * r.direction.x * (r.origin.x - sphere.center.x)) +
		(2 * r.direction.y * (r.origin.y - sphere.center.y)) +
		(2 * r.direction.z * (r.origin.z - sphere.center.z));
	float c = dot(sphere.center, sphere.center) + dot(r.origin, r.origin) + (-2 * dot(sphere.center, r.origin)) - pow(sphere.radius, 2);

	float determ = pow(b, 2) - 4 * a*c;	//calculates determinant

	if (determ < 0)						//if determ < 0 then no intersection
		return vec4(NULL, NULL, NULL, NULL);
	else
	{
		float t1 = (-b + determ) / (2 * a);
		float t2 = (-b - determ) / (2 * a);
		float t;

		if (t1 <= t2){ t = t1; }
		else{ t = t2; }

		vec3 x = r.origin + (t*r.direction);
		vec3 n = x - sphere.center;
		vec3 color;

		ray newRay;
		newRay.origin = x;
		newRay.direction = light.position - x;

		color = Phong(light, x, n, sphere.color, r, true);

		return vec4(color, t);
	}

}

vec4 intersectPlane(ray ray, plane plane, light light)
{
	float para = dot(plane.normal, ray.direction);
	if (para != 0)
	{
		vec3 ppco = plane.position - ray.origin; //planePosition -cameraOrigin
		float t = dot(ppco, plane.normal) / para;

		if (t < 0)
			return vec4(NULL, NULL, NULL, NULL);

		vec3 x = ray.origin + (t*ray.direction);
		vec3 color = Phong(light, x, plane.normal, plane.color, ray, false);

		return vec4(color, t);
	}
	else
	{
		return vec4(NULL, NULL, NULL, NULL);
	}
}

vec4 intersectTriangle(ray ray, triangle tri, light light)
{
	//compute normal vector of plane which triangle resides
	vec3 P1P0 = tri.P1 - tri.P0;
	vec3 P2P0 = tri.P2 - tri.P0;
	vec3 P2P1 = tri.P2 - tri.P1;
	vec3 P0P2 = tri.P0 - tri.P2;
	vec3 normal = normalize(cross(P1P0, P2P0));

	plane p;
	p.normal = normal;
	p.position = tri.P0;

	vec4 plane = intersectPlane(ray, p, light);

	vec3 x = ray.origin + (plane.w*ray.direction); //plane.w is t

	float a = dot(cross(P1P0, (x - tri.P0)), normal);
	float b = dot(cross(P2P1, (x - tri.P1)), normal);
	float c = dot(cross(P0P2, (x - tri.P2)), normal);

		if (a >= -0.001 && b >= -0.001 && c >= -0.001)
		{
			vec3 color = Phong(light, x, normal, tri.color, ray, false);
			return vec4(color, plane.w);
		}
		else
		{
			return vec4(NULL, NULL, NULL, NULL);
		}
}

int main(int argc, char *argv[])
{   
    // initialize the GLFW windowing system
    if (!glfwInit()) {
        cout << "ERROR: GLFW failed to initilize, TERMINATING" << endl;
        return -1;
    }
    glfwSetErrorCallback(ErrorCallback);

    // attempt to create a window with an OpenGL 4.1 core profile context
    GLFWwindow *window = 0;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    window = glfwCreateWindow(windowX, windowY, "CPSC 453 OpenGL Boilerplate", 0, 0);
    if (!window) {
        cout << "Program failed to create GLFW window, TERMINATING" << endl;
        glfwTerminate();
        return -1;
    }

    // set keyboard callback function and make our context current (active)
    glfwSetKeyCallback(window, KeyCallback);
    glfwMakeContextCurrent(window);

    // query and print out information about our OpenGL environment
	gladLoadGL();

    QueryGLVersion();

	ImageBuffer imageBuffer;
	imageBuffer.Initialize();

    // run an event-triggered main loop
    while (!glfwWindowShouldClose(window))
    {
		//Load objects from file
		if (scene == 1)
			loadAllObjects("scene1.txt");
		if (scene == 2)
			loadAllObjects("scene2.txt");
		if (scene == 3)
			loadAllObjects("scene3.txt");
		cout << triangles.at(0).color.x << endl;
		vec3 cameraOrigin(0, 0, 0);			//place camera origin

		int l, r, t, b;						//init and set 
		r = t = 1;
		l = b = -r;

        // call function to draw our scene
		for (int i = 0; i < imageBuffer.Width(); i++)
		{
			for (int j = 0; j < imageBuffer.Height(); j++) 
			{
				ray newRay; //init ray to be shot out of camera
				vec4 intersect; //init data vector for all intersects
				vec4 closestInteresectAndColor(1.0, 1.0, 1.0, numeric_limits<float>::max()); //return color if there exists an intersection
				bool doesIntersect = false; //start every ray as non-intersect

				//Calculates camera ray direction vector
				float u = l + ((r - l) * (i + 0.5)) / (windowX);
				float v = b + ((t - b) * (j + 0.5)) / (windowY);
				float w = -(r / tan(FoV/2)); //dynamic Field of View
				
				//Ray data assignment
				newRay.origin = cameraOrigin;
				newRay.direction = normalize(vec3(u, v, w) - cameraOrigin);

				//check intersect with all spheres
				for (int i = 0; i < spheres.size(); i++)
				{
					intersect = intersectSphere(newRay, spheres.at(i),lights.at(0));
					if (intersect.w != NULL)
						doesIntersect = true;
					if ((intersect.w < closestInteresectAndColor.w) && (intersect.w != NULL))
						closestInteresectAndColor = intersect;

				}

				//check intersect with all planes
				for (int i = 0; i < planes.size(); i++)
				{
					intersect = intersectPlane(newRay, planes.at(i), lights.at(0));
					if (intersect.w != NULL)
						doesIntersect = true;
					if ((intersect.w < closestInteresectAndColor.w) && (intersect.w != NULL))
						closestInteresectAndColor = intersect;
				}
				
				//check intersect with all triangles
				for (int i = 0; i < triangles.size(); i++)
				{
					intersect = intersectTriangle(newRay, triangles.at(i), lights.at(0));
					if (intersect.w != NULL)
						doesIntersect = true;
					if (intersect.w < closestInteresectAndColor.w && (intersect.w != NULL))
						closestInteresectAndColor = intersect;
				}
				
				if (doesIntersect == true) //only if there was an intersect this ray, draw pixel
				{
					vec3 color = vec3(closestInteresectAndColor);
					imageBuffer.SetPixel(i, j, color);
				}
			}
		}

		imageBuffer.Render();

        // scene is rendered to the back buffer, so swap to front for display
        glfwSwapBuffers(window);

        // sleep until next event before drawing again
        glfwWaitEvents();
    }

    // clean up allocated resources before exit
    glfwDestroyWindow(window);
    glfwTerminate();
	return 0;
}

// ==========================================================================
// SUPPORT FUNCTION DEFINITIONS

// --------------------------------------------------------------------------
// OpenGL utility functions


void QueryGLVersion()
{
    // query opengl version and renderer information
    string version  = reinterpret_cast<const char *>(glGetString(GL_VERSION));
    string glslver  = reinterpret_cast<const char *>(glGetString(GL_SHADING_LANGUAGE_VERSION));
    string renderer = reinterpret_cast<const char *>(glGetString(GL_RENDERER));

    cout << "OpenGL [ " << version << " ] "
         << "with GLSL [ " << glslver << " ] "
         << "on renderer [ " << renderer << " ]" << endl;
}

bool CheckGLErrors()
{
    bool error = false;
    for (GLenum flag = glGetError(); flag != GL_NO_ERROR; flag = glGetError())
    {
        cout << "OpenGL ERROR:  ";
        switch (flag) {
        case GL_INVALID_ENUM:
            cout << "GL_INVALID_ENUM" << endl; break;
        case GL_INVALID_VALUE:
            cout << "GL_INVALID_VALUE" << endl; break;
        case GL_INVALID_OPERATION:
            cout << "GL_INVALID_OPERATION" << endl; break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            cout << "GL_INVALID_FRAMEBUFFER_OPERATION" << endl; break;
        case GL_OUT_OF_MEMORY:
            cout << "GL_OUT_OF_MEMORY" << endl; break;
        default:
            cout << "[unknown error code]" << endl;
        }
        error = true;
    }
    return error;
}

// --------------------------------------------------------------------------
// OpenGL shader support functions

// reads a text file with the given name into a string
string LoadSource(const string &filename)
{
    string source;

    ifstream input(filename);
    if (input) {
        copy(istreambuf_iterator<char>(input),
             istreambuf_iterator<char>(),
             back_inserter(source));
        input.close();
    }
    else {
        cout << "ERROR: Could not load shader source from file "
             << filename << endl;
    }

    return source;
}

// creates and returns a shader object compiled from the given source
GLuint CompileShader(GLenum shaderType, const string &source)
{
    // allocate shader object name
    GLuint shaderObject = glCreateShader(shaderType);

    // try compiling the source as a shader of the given type
    const GLchar *source_ptr = source.c_str();
    glShaderSource(shaderObject, 1, &source_ptr, 0);
    glCompileShader(shaderObject);

    // retrieve compile status
    GLint status;
    glGetShaderiv(shaderObject, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE)
    {
        GLint length;
        glGetShaderiv(shaderObject, GL_INFO_LOG_LENGTH, &length);
        string info(length, ' ');
        glGetShaderInfoLog(shaderObject, info.length(), &length, &info[0]);
        cout << "ERROR compiling shader:" << endl << endl;
        cout << source << endl;
        cout << info << endl;
    }

    return shaderObject;
}

// creates and returns a program object linked from vertex and fragment shaders
GLuint LinkProgram(GLuint vertexShader, GLuint fragmentShader)
{
    // allocate program object name
    GLuint programObject = glCreateProgram();

    // attach provided shader objects to this program
    if (vertexShader)   glAttachShader(programObject, vertexShader);
    if (fragmentShader) glAttachShader(programObject, fragmentShader);

    // try linking the program with given attachments
    glLinkProgram(programObject);

    // retrieve link status
    GLint status;
    glGetProgramiv(programObject, GL_LINK_STATUS, &status);
    if (status == GL_FALSE)
    {
        GLint length;
        glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &length);
        string info(length, ' ');
        glGetProgramInfoLog(programObject, info.length(), &length, &info[0]);
        cout << "ERROR linking shader program:" << endl;
        cout << info << endl;
    }

    return programObject;
}


// ==========================================================================
