#define STB_IMAGE_IMPLEMENTATION
extern "C"
{
	#include <psp2/kernel/modulemgr.h>
	#include <psp2/kernel/processmgr.h>
	#include <psp2/kernel/clib.h>
	#include <PVR_PSP2/EGL/eglplatform.h>
	#include <PVR_PSP2/EGL/egl.h>
	#include <PVR_PSP2/gpu_es4/psp2_pvr_hint.h>

	#include <PVR_PSP2/GLES2/gl2.h>
	#include <PVR_PSP2/GLES2/gl2ext.h>

	#include <psp2/ctrl.h>
    #include <psp2/io/fcntl.h>
}

#include <cmath>
#include <fstream>
#include <string>
#include <cstring>
#include "stb_image.h"

using namespace std;

//SCE
int _newlib_heap_size_user   = 16 * 1024 * 1024;
unsigned int sceLibcHeapSize = 3 * 1024 * 1024;
unsigned int VBO, EBO;

//EGL
EGLDisplay Display;
EGLConfig Config;
EGLSurface Surface;
EGLContext Context;
EGLint NumConfigs, MajorVersion, MinorVersion;
EGLint ConfigAttr[] =
{
	EGL_BUFFER_SIZE, EGL_DONT_CARE,
	EGL_DEPTH_SIZE, 16,
	EGL_RED_SIZE, 8,
	EGL_GREEN_SIZE, 8,
	EGL_BLUE_SIZE, 8,
	EGL_ALPHA_SIZE, 8,
	EGL_STENCIL_SIZE, 8,
	EGL_SURFACE_TYPE, 5,
	EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
	EGL_NONE
};
EGLint ContextAttributeList[] = 
{
	EGL_CONTEXT_CLIENT_VERSION, 2,
	EGL_NONE
};

//PIB cube egl stuff

static GLuint program;
static GLuint positionLoc;
static GLuint colorLoc;
static GLuint texLoc;
//static GLuint sampleLoc;

//Texture data
int width, height, nrChannels;
unsigned char *data;
unsigned int texture;
const char* textpath = "app0:images/wall.jpg";


static EGLint surface_width, surface_height;

float vertices[] = {
     0.5f,  -0.5f, 0.0f,     1.0f, 0.0f, 0.0f,   1.0f, 0.0f, 
     0.5f, 0.5f, 0.0f,       0.0f, 1.0f, 0.0f,   1.0f, 1.0f, 
    -0.5f, 0.5f, 0.0f,       0.0f, 0.0f, 1.0f,   0.0f, 1.0f,
    -0.5f,  -0.5f, 0.0f,     1.0f, 0.0f, 0.5f,   0.0f, 0.0f 
};
unsigned int indices[] = {  // note that we start from 0!
    0, 1, 2,   // first triangle
    2, 3, 0    // second triangle
}; 

/* Define our GLSL shader here. They will be compiled at runtime */
const GLchar vShaderStr[] =
// Vertex Shader
    "#version 100\n"
    "attribute vec3 aPos;\n"
    "attribute vec3 aColor;\n"
    "attribute vec2 aTex;\n"
    "varying vec3 oColor;\n"
    "varying vec2 oTex;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
        "oColor = aColor;\n"
        "oTex = aTex;\n"
    "}\n";

const GLchar fShaderStr[] = 
// Fragment Shader
        "#version 100\n"
        "precision mediump float;\n"
        "varying vec3 oColor;\n"
        "varying vec2 oTex;\n"
        "uniform sampler2D ourTexture;\n"
        "void main()\n"
        "{\n"
        //"    gl_FragColor = vec4(oColor, 1.0);\n"
            "gl_FragColor = texture2D(ourTexture, oTex);\n"
        "}\n";


GLuint LoadShader(const GLchar *shaderSrc, GLenum type, GLint *size)
{
    GLuint shader;
    GLint compiled;

    sceClibPrintf("Creating Shader...\n");

    shader = glCreateShader(type);

    if(shader == 0)
    {
        sceClibPrintf("Failed to Create Shader\n");
        return 0;
    }

    glShaderSource(shader, 1, &shaderSrc, size);

    sceClibPrintf("Compiling Shader: %s...\n", shaderSrc);
    glCompileShader(shader);

    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

    if(!compiled)
    {
        GLint infoLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);

        if(infoLen > 1)
        {
            char* infoLog = (char*)malloc(sizeof(char) * infoLen);
            glGetShaderInfoLog(shader, infoLen, NULL, infoLog);  // Shader Logs through GLES functions work :D
            sceClibPrintf("Error compiling shader:\n%s\n", infoLog);
            free(infoLog);
        }
        glDeleteShader(shader);
        return 0;
    }
    sceClibPrintf("Shader Compiled!\n");
    return shader;
}

void render(){
    glViewport(0, 0, surface_width, surface_height);
 
    /* Typical render pass */
    glClear(GL_COLOR_BUFFER_BIT); 
    //glBindTexture(GL_TEXTURE_2D, texture);
 
    /* Enable and bind the vertex information */
    glEnableVertexAttribArray(positionLoc);    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(positionLoc, 3, GL_FLOAT, GL_FALSE,
                          8 * sizeof(GLfloat), (void*)0);      

    /* Enable and bind the color information */
    glEnableVertexAttribArray(colorLoc);    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(colorLoc, 3, GL_FLOAT, GL_FALSE,
                          8 * sizeof(GLfloat), (void*)(3* sizeof(float)));    

    /* Enable and bind the texture coord information */
    glEnableVertexAttribArray(texLoc);    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(texLoc, 3, GL_FLOAT, GL_FALSE,
                          8 * sizeof(GLfloat), (void*)(6* sizeof(float)));                                                                       

    glUseProgram(program);

    /* Same draw call as in GLES1 */    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO); 
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    /* Disable attribute arrays */
    glDisableVertexAttribArray(positionLoc);
    glDisableVertexAttribArray(colorLoc);
    glDisableVertexAttribArray(texLoc);
 
    eglSwapBuffers(Display, Surface);
}

/* How we load shaders */
int initShaders(void)
{
    GLint status;

    GLuint vshader = LoadShader(vShaderStr, GL_VERTEX_SHADER, NULL);
    GLuint fshader = LoadShader(fShaderStr, GL_FRAGMENT_SHADER, NULL);
    program = glCreateProgram();
    if (program)
    {
        glAttachShader(program, vshader);
        glAttachShader(program, fshader);
        glLinkProgram(program);
    
        glGetProgramiv(program, GL_LINK_STATUS, &status);
	sceClibPrintf("Status: %d\n", status);
        if (status == GL_FALSE)    {
            GLchar log[256];
            glGetProgramInfoLog(fshader, 256, NULL, log);
    
            sceClibPrintf("Failed to link shader program: %s\n", log);
    
            glDeleteProgram(program);
            program = 0;
    
            return -1;
        }
    } else {
        sceClibPrintf("Failed to create a shader program\n");
    
        glDeleteShader(vshader);
        glDeleteShader(fshader);
        return -1;
    }
 
    /* Shaders are now in the programs */
    glDeleteShader(fshader);
    glDeleteShader(vshader);
    return 0;
}

/* This handles creating a view matrix for the Vita */
int initvbos(void)
{
    positionLoc = glGetAttribLocation(program, "aPos");
    colorLoc = glGetAttribLocation(program, "aColor");
    texLoc = glGetAttribLocation(program, "aTex");

    /* Generate vertex and color buffers and fill with data */
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices,
                GL_STATIC_DRAW);  

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW); 
    return 0;
}

int ReadFile(const char* path, unsigned char **buffer, SceOff &size) {
    int ret = 0;
    SceUID file = 0;
    file = sceIoOpen(path, SCE_O_RDONLY, 0777);
    ret = file;
    if (ret < 0) {
        sceClibPrintf("sceIoOpen(%s) failed: 0x%lx\n", path, ret);        
        return ret;
    }
        
    size = sceIoLseek(file, 0, SEEK_END);
    *buffer = new unsigned char[size];

    ret = sceIoPread(file, *buffer, size, SCE_SEEK_SET);
    if (ret < 0) {
        sceClibPrintf("sceIoPread(%s) failed: 0x%lx\n", path, ret);
        return ret;
    }
    ret = sceIoClose(file);
    if (ret < 0) {
        sceClibPrintf("sceIoClose(%s) failed: 0x%lx\n", path, ret);
        return ret;
    }

    return 0;
}

void loadTextures(void){
    //load image into memory
    //SceOff size = 0;
    //unsigned char *data = nullptr;
    unsigned char *image = nullptr;
    /*int res = ReadFile(textpath, &data, size);
    if (res != 0) {
        sceClibPrintf("Error loading raw file to memory.\n");
    }

    image = stbi_load_from_memory(data, size, &width, &height, nullptr, 4);*/
    image = stbi_load(textpath, &width, &height, nullptr, 0);
    sceClibPrintf("Image dimensions: %d x %d. Code: %s.\n", width, height, stbi_failure_reason());
        
    //initialize texture    
    glGenTextures(1, &texture);    
    /* Activate a texture. */
    glActiveTexture(GL_TEXTURE0);      
    glBindTexture(GL_TEXTURE_2D, texture); // all upcoming GL_TEXTURE_2D operations now have effect on this texture object
     // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (image)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        glGenerateMipmap(GL_TEXTURE_2D);
        sceClibPrintf("Texture load OK.\n");
    } else {
        sceClibPrintf("Failed to load texture: %s.\n", stbi_failure_reason());
    }
    //free memory
    stbi_image_free(image);
    free(data);
}

void ModuleInit()
{
	sceKernelLoadStartModule("vs0:sys/external/libfios2.suprx", 0, NULL, 0, NULL, NULL);
	sceKernelLoadStartModule("vs0:sys/external/libc.suprx", 0, NULL, 0, NULL, NULL);
	sceKernelLoadStartModule("app0:module/libgpu_es4_ext.suprx", 0, NULL, 0, NULL, NULL);
  	sceKernelLoadStartModule("app0:module/libIMGEGL.suprx", 0, NULL, 0, NULL, NULL);
	sceClibPrintf("Module init OK\n");
}

void PVR_PSP2Init()
{
	PVRSRV_PSP2_APPHINT hint;
  	PVRSRVInitializeAppHint(&hint);
  	PVRSRVCreateVirtualAppHint(&hint);
	sceClibPrintf("PVE_PSP2 init OK.\n");
}

void EGLInit()
{
	EGLBoolean Res;
	Display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	if(!Display)
	{
		sceClibPrintf("EGL display get failed.\n");
		return;
	}

	Res = eglInitialize(Display, &MajorVersion, &MinorVersion);
	if (Res == EGL_FALSE)
	{
		sceClibPrintf("EGL initialize failed.\n");
		return;
	} else {
        sceClibPrintf("EGL version: %d.%d\n", MajorVersion, MinorVersion);
    }

	//PIB cube demo
	eglBindAPI(EGL_OPENGL_ES_API);

	Res = eglChooseConfig(Display, ConfigAttr, &Config, 1, &NumConfigs);
	if (Res == EGL_FALSE)
	{
		sceClibPrintf("EGL config initialize failed.\n");
		return;
	}

	Surface = eglCreateWindowSurface(Display, Config, (EGLNativeWindowType)0, nullptr);
	if(!Surface)
	{
		sceClibPrintf("EGL surface create failed.\n");
		return;
	}

	Context = eglCreateContext(Display, Config, EGL_NO_CONTEXT, ContextAttributeList);
	if(!Context)
	{
		sceClibPrintf("EGL content create failed.\n");
		return;
	}

	eglMakeCurrent(Display, Surface, Surface, Context);

	// PIB cube demo
	eglQuerySurface(Display, Surface, EGL_WIDTH, &surface_width);
	eglQuerySurface(Display, Surface, EGL_HEIGHT, &surface_height);
	sceClibPrintf("Surface Width: %d, Surface Height: %d\n", surface_width, surface_height);
	glClearDepthf(1.0f);
	glClearColor(0.0f,0.0f,0.0f,1.0f); // You can change the clear color to whatever
	 
	glEnable(GL_CULL_FACE);

	sceClibPrintf("EGL init OK.\n");

    const GLubyte *renderer = glGetString( GL_RENDERER ); 
    const GLubyte *vendor = glGetString( GL_VENDOR ); 
    const GLubyte *version = glGetString( GL_VERSION ); 
    const GLubyte *glslVersion = glGetString( GL_SHADING_LANGUAGE_VERSION ); 
    
    sceClibPrintf("GL Vendor            : %s\n", vendor); 
    sceClibPrintf("GL Renderer          : %s\n", renderer); 
    sceClibPrintf("GL Version (string)  : %s\n", version); 
    sceClibPrintf("GLSL Version         : %s\n", glslVersion);    
}

void EGLEnd()
{
	eglDestroySurface(Display, Surface);
  	eglDestroyContext(Display, Context);
  	eglTerminate(Display);
	sceClibPrintf("EGL terminated.\n");
}

void SCEInit()
{
	sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
	sceClibPrintf("SCE init OK\n");
}

int main()
{
	ModuleInit();
	PVR_PSP2Init();
	EGLInit();

	SCEInit();
	sceClibPrintf("All init OK.\n");

	initShaders();
	initvbos();
	loadTextures();
	while(1)
	{
	    render();
	}

	EGLEnd();

	sceKernelExitProcess(0);
	return 0;
}