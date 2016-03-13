// ShaderDemoX.c : This is it. Created by eiffie GPLv3
// What is it? Runs simple webgl scripts from ShaderToy under XServer.
// Why? I wanted to run the games offline/native at fullscreen/speed (and learn XServer).
// Usage: Create a textfile in GEDIT and type [bufA] then newline. Copy in code for buffer A
// type [image] on a line by itself then copy in the code from the Image tab.
// Save it and run ./ShaderDemoX whateveryounamedit.
// Buffer A=iChannel0 and keyboard=iChannel1
// Press "escape" to quit and "backspace" to reset iFrame.
// You need these standard headers plus a few gl and X11 headers 
// to compile: gcc -o ShaderDemoX ShaderDemoX.c -lX11 -lXxf86vm -lGL

//lots of code taken from Mihael Vrbanec's nehe linux port. Thank you. It still works!

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <GL/glx.h>
#include <GL/gl.h>
#include <X11/extensions/xf86vmode.h>
#include <X11/keysym.h>

//#define _DEBUG

char *bufferA=NULL,*image=NULL;
char VSscript[]="void main() {gl_Position = gl_Vertex;}";
char fsh[]="uniform sampler2D Zbuf,Ztex;\n\
		   uniform float Zuni[16];\n\
		   #define iResolution vec4(Zuni[0],Zuni[1],0.0,0.0)\n\
		   #define iGlobalTime Zuni[2]\n\
		   #define iMouse vec4(Zuni[3],Zuni[4],Zuni[5],Zuni[6])\n\
		   #define iFrame int(Zuni[7])\n\
		   #define iTimeDelta Zuni[8]\n\
		   #define iDate vec4(Zuni[9],Zuni[10],Zuni[11],Zuni[12])\n\
		   #define iChannel0 Zbuf\n\
		   #define iChannel1 Ztex\n\
		   vec2 iChannelResolution[2];\n\
		   void mainImage(out vec4, in vec2);\n\
		   void main(){iChannelResolution[0]=iResolution.xy;\n\
		   iChannelResolution[1]=vec2(256.0,2.0);\n\
		   mainImage(gl_FragColor,gl_FragCoord.xy);}\n%s";

Bool loadShaders(char *fname){
	FILE *fp;
	char buff[512],*txt,*buf;
	fp = fopen (fname, "rb");
	if (!fp) return False;
	fgets (buff, sizeof (buff), fp); //Read whole line
	if(strcmp(buff,"[bufA]\n")){fclose(fp);return False;}
	bufferA=(char *)malloc(65536);
	image=(char *)malloc(65536);
	bufferA[0]=0;image[0]=0;
	txt=bufferA;
	int i=0;
	while (!feof(fp)){//load [bufA] and [image]
		buf=fgets (buff, sizeof (buff), fp); //Read whole line
		if(buf){
			if(!strcmp(buff,"[image]\n")){
				txt=image;i=0;
			}else strcat(txt,buff);
		}
	}
	fclose(fp);
	return (txt==image);
}
//typedef void (__stdcall * PFNGLACTIVETEXTUREPROC) (GLenum texunit);
//typedef void (__stdcall * PFNGLTEXIMAGE3DEXTPROC) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void* pixels);
typedef void (GLAPIENTRY * PFNGLATTACHSHADERPROC) (GLuint program, GLuint shader);
typedef void (GLAPIENTRY * PFNGLCOMPILESHADERPROC) (GLuint shader);
typedef GLuint (GLAPIENTRY * PFNGLCREATEPROGRAMPROC) (void);
typedef GLuint (GLAPIENTRY * PFNGLCREATESHADERPROC) (GLenum type);
typedef void (GLAPIENTRY * PFNGLDELETEPROGRAMPROC) (GLuint program);
typedef void (GLAPIENTRY * PFNGLDELETESHADERPROC) (GLuint shader);
typedef void (GLAPIENTRY * PFNGLDETACHSHADERPROC) (GLuint program, GLuint shader);
typedef void (GLAPIENTRY * PFNGLLINKPROGRAMPROC) (GLuint program);
typedef void (GLAPIENTRY * PFNGLUSEPROGRAMPROC) (GLuint program);
//typedef void (GLAPIENTRY * PFNGLSHADERSOURCEPROC) (GLuint shader, GLsizei count, const char** strings, const GLint* lengths);
typedef void (GLAPIENTRY * PFNGLGETSHADERIVPROC) (GLuint shader, GLenum pname, GLint* param);
typedef void (GLAPIENTRY * PFNGLGETPROGRAMIVPROC) (GLuint program, GLenum pname, GLint* param);
typedef void (GLAPIENTRY * PFNGLGETPROGRAMINFOLOGPROC) (GLuint program, GLsizei bufSize, GLsizei* length, char* infoLog);
typedef void (GLAPIENTRY * PFNGLGETSHADERINFOLOGPROC) (GLuint shader, GLsizei bufSize, GLsizei* length, char* infoLog);
typedef GLint (GLAPIENTRY * PFNGLGETUNIFORMLOCATION) (GLuint program, const char *name);
typedef void (GLAPIENTRY * PFNGLUNIFORMFVPROC) (GLint location, GLsizei count, const GLfloat* value);
typedef void (GLAPIENTRY * PFNGLUNIFORMIVPROC) (GLint location, GLsizei count, const GLint* value);
typedef void (GLAPIENTRY * PFNGLDELETEFRAMEBUFFERSPROC) (GLsizei n, const GLuint* framebuffers);
typedef void (GLAPIENTRY * PFNGLBINDFRAMEBUFFERPROC) (GLenum target, GLuint framebuffer);
typedef void (GLAPIENTRY * PFNGLFRAMEBUFFERTEXTURE2DPROC) (GLenum target, GLenum attachment, GLenum textarget, GLuint textur, GLint level);
typedef void (GLAPIENTRY * PFNGLGENFRAMEBUFFERSPROC) (GLsizei n, GLuint* framebuffers);
typedef GLenum (GLAPIENTRY * PFNGLCHECKFRAMEBUFFERSTATUSPROC) (GLenum target);
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER 0x8B31
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_INFO_LOG_LENGTH 0x8B84
#define GL_TEXTURE_3D_EXT 0x806F
#define GL_FRAMEBUFFER 0x8D40
#define GL_TEXTURE0 0x84C0
#define GL_COLOR_ATTACHMENT0 0x8CE0
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#define GL_RGBA32F 0x8814
#define GL_RGB32F 0x8815

//PFNGLACTIVETEXTUREPROC glActiveTexture=NULL;
//PFNGLTEXIMAGE3DEXTPROC glTexImage3D=NULL;
PFNGLDELETESHADERPROC glDeleteShader=NULL; 
PFNGLDELETEPROGRAMPROC glDeleteProgram=NULL; 
PFNGLCREATESHADERPROC glCreateShader=NULL;
//PFNGLSHADERSOURCEPROC glShaderSource=NULL;
PFNGLCOMPILESHADERPROC glCompileShader=NULL;
PFNGLATTACHSHADERPROC glAttachShader=NULL;
PFNGLCREATEPROGRAMPROC glCreateProgram=NULL;
PFNGLLINKPROGRAMPROC glLinkProgram=NULL;
PFNGLUSEPROGRAMPROC glUseProgram=NULL;
PFNGLDETACHSHADERPROC glDetachShader=NULL;
PFNGLGETSHADERIVPROC glGetShaderiv=NULL;
PFNGLGETPROGRAMIVPROC glGetProgramiv=NULL;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog=NULL;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog=NULL;
PFNGLGETUNIFORMLOCATION glGetUniformLocation=NULL;
PFNGLUNIFORMFVPROC glUniform1fv=NULL;
PFNGLUNIFORMIVPROC glUniform1iv=NULL;
PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers=NULL;
PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers=NULL;
PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer=NULL;
PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D=NULL;
PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus=NULL;
#define LPCSTR const GLubyte *
Bool LoadShaderProcs(){//call after starting OpenGL
	//glActiveTexture=(PFNGLACTIVETEXTUREPROC)glXGetProcAddress((LPCSTR)"glActiveTexture");
	//glTexImage3D=(PFNGLTEXIMAGE3DEXTPROC)glXGetProcAddress((LPCSTR)"glTexImage3DEXT");
	glDeleteShader=(PFNGLDELETESHADERPROC)glXGetProcAddress((LPCSTR)"glDeleteShader"); 
	glDeleteProgram=(PFNGLDELETEPROGRAMPROC)glXGetProcAddress((LPCSTR)"glDeleteProgram"); 
	glCreateShader=(PFNGLCREATESHADERPROC)glXGetProcAddress((LPCSTR)"glCreateShader");
	//glShaderSource=(PFNGLSHADERSOURCEPROC)glXGetProcAddress((LPCSTR)"glShaderSource");
	glCompileShader=(PFNGLCOMPILESHADERPROC)glXGetProcAddress((LPCSTR)"glCompileShader");
	glCreateProgram=(PFNGLCREATEPROGRAMPROC)glXGetProcAddress((LPCSTR)"glCreateProgram");
	glAttachShader=(PFNGLATTACHSHADERPROC)glXGetProcAddress((LPCSTR)"glAttachShader");
	glLinkProgram=(PFNGLLINKPROGRAMPROC)glXGetProcAddress((LPCSTR)"glLinkProgram");
	glUseProgram=(PFNGLUSEPROGRAMPROC)glXGetProcAddress((LPCSTR)"glUseProgram");
	glDetachShader=(PFNGLDETACHSHADERPROC)glXGetProcAddress((LPCSTR)"glDetachShader");
	glGetShaderiv=(PFNGLGETSHADERIVPROC)glXGetProcAddress((LPCSTR)"glGetShaderiv");
	glGetProgramiv=(PFNGLGETPROGRAMIVPROC)glXGetProcAddress((LPCSTR)"glGetProgramiv");
	glGetProgramInfoLog=(PFNGLGETPROGRAMINFOLOGPROC)glXGetProcAddress((LPCSTR)"glGetProgramInfoLog");
	glGetShaderInfoLog=(PFNGLGETSHADERINFOLOGPROC)glXGetProcAddress((LPCSTR)"glGetShaderInfoLog");
	glGetUniformLocation=(PFNGLGETUNIFORMLOCATION)glXGetProcAddress((LPCSTR)"glGetUniformLocation");
	glUniform1fv=(PFNGLUNIFORMFVPROC)glXGetProcAddress((LPCSTR)"glUniform1fv");
	glUniform1iv=(PFNGLUNIFORMIVPROC)glXGetProcAddress((LPCSTR)"glUniform1iv");
	glDeleteFramebuffers=(PFNGLDELETEFRAMEBUFFERSPROC)glXGetProcAddress((LPCSTR)"glDeleteFramebuffers");
	glGenFramebuffers=(PFNGLGENFRAMEBUFFERSPROC)glXGetProcAddress((LPCSTR)"glGenFramebuffers");
	glBindFramebuffer=(PFNGLBINDFRAMEBUFFERPROC)glXGetProcAddress((LPCSTR)"glBindFramebuffer");
	glFramebufferTexture2D=(PFNGLFRAMEBUFFERTEXTURE2DPROC)glXGetProcAddress((LPCSTR)"glFramebufferTexture2D");
	glCheckFramebufferStatus=(PFNGLCHECKFRAMEBUFFERSTATUSPROC)glXGetProcAddress((LPCSTR)"glCheckFramebufferStatus");
	Bool bFrameBuffers=(glDeleteFramebuffers&&glGenFramebuffers&&glBindFramebuffer&&glFramebufferTexture2D&&glCheckFramebufferStatus);
	if(!(bFrameBuffers&&glCreateShader&&glCompileShader&&glCreateProgram&&glAttachShader&&glLinkProgram&&
		glUseProgram&&glDetachShader&&glGetShaderiv&&glGetProgramiv&&glGetProgramInfoLog&&glGetShaderInfoLog)){
		printf("Could not get OpenGL process addresses. Expecting 2.0 functionality.\n");
		return False;
	}
	return True;
}
Bool createprogram(char *VSscript, char *FSscript, GLuint *P, GLuint *VS, GLuint *FS){
	if(!(VSscript && FSscript))return False;
	if(!(VSscript[0] && FSscript[0]))return False;
	*P=glCreateProgram();
	*VS=glCreateShader(GL_VERTEX_SHADER);
	*FS=glCreateShader(GL_FRAGMENT_SHADER);
	GLint iStatus=0;
	const char *fs=VSscript;
	glShaderSource(*VS, 1, &fs, NULL);
	glCompileShader(*VS);
	glGetShaderiv(*VS,GL_COMPILE_STATUS,&iStatus);
	if(!iStatus){
		char *log=(char *)malloc(1024);
		if(!log)return False;
		glGetShaderInfoLog(*VS, 1024, &iStatus, log);
		printf("vertex: %s",log);
		free(log);
		return False;
	}
	fs=FSscript;
	glShaderSource(*FS, 1, &fs, NULL);
	glCompileShader(*FS);
	glGetShaderiv(*FS,GL_COMPILE_STATUS,&iStatus);
	if(!iStatus){
		char *log=(char *)malloc(1024);
		if(!log)return False;
		glGetShaderInfoLog(*FS, 1024, &iStatus, log);
		printf("fragment: %s",log);
		free(log);
		return False;
	}
	glAttachShader(*P,*VS);
	glAttachShader(*P,*FS);
	glLinkProgram(*P);
	glGetProgramiv(*P,GL_LINK_STATUS,&iStatus);
	if(!iStatus){
		char *log=(char *)malloc(1024);
		if(!log)return False;
		glGetProgramInfoLog(*P, 1024, &iStatus, log);
		glDetachShader(*P,*VS);
		glDetachShader(*P,*FS);
		printf("linking: %s",log);
		free(log);
		return False;
	}
	return True;
}

void setkbtexture(void *bits, GLuint ztexture){//keyboard texture
	glActiveTexture(GL_TEXTURE0+1);							// Texture #1
	glBindTexture(GL_TEXTURE_2D, ztexture);					// Tell GL we are using it
	glTexImage2D(GL_TEXTURE_2D,0,GL_RED,256,1,0,GL_RED,GL_UNSIGNED_BYTE,bits);//should be 256x2
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glActiveTexture(GL_TEXTURE0);
}
void SetTexParams(int width, int height){//render textures
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL); 
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
}

long getTickCount (void){
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
	return spec.tv_sec*1000 + spec.tv_nsec / 1.0e6; 
}

static int attrlist[] = { GLX_RGBA, GLX_DOUBLEBUFFER, None };

#define min(a,b) ((a)<(b)?(a):(b))
int main(int argc, char **argv)
{
	if(!argv[1])return -1;
	if(argv[1][0]==0)return -1;
	if(!loadShaders(argv[1]))return -1;
	//these describe the window
	Display *dpy;
    int screen,modeNum;
    Window win;
    GLXContext ctx;
    XSetWindowAttributes attr;
    XF86VidModeModeInfo deskMode;
	XVisualInfo *vi;
    Colormap cmap;
	Atom wmDelete;
#ifdef _DEBUG
	unsigned int maxx=512,maxy=320;
#else
	unsigned int maxx=0,maxy=0;
#endif
	Bool MODE_FULLSCREEN=(maxx==0);
	dpy = XOpenDisplay(0);
    screen = DefaultScreen(dpy);
	XF86VidModeModeInfo **modes;
    XF86VidModeGetAllModeLines(dpy, screen, &modeNum, &modes);
    deskMode = *modes[0];//save desktop resolution
	int width=modes[0]->hdisplay;
	int height=modes[0]->vdisplay;
	width=min((maxx>0?maxx:width),width);
	height=min((maxy>0?maxy:height),height);
	vi = glXChooseVisual(dpy, screen, attrlist);
    if(!vi){printf("No doublebuffering.\n");return -1;}
	ctx = glXCreateContext(dpy, vi, 0, GL_TRUE);
	cmap = XCreateColormap(dpy, RootWindow(dpy, vi->screen), vi->visual, AllocNone);
    attr.colormap = cmap; attr.border_pixel = 0;
	attr.event_mask = KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask;
    if (MODE_FULLSCREEN){/* create a fullscreen window */
        XF86VidModeSwitchToMode(dpy, screen, modes[0]);//actually not switching modes!
        XF86VidModeSetViewPort(dpy, screen, 0, 0);
        attr.override_redirect = True;
        win = XCreateWindow(dpy, RootWindow(dpy, vi->screen),
            0, 0, width, height, 0, vi->depth, InputOutput, vi->visual,
            CWBorderPixel | CWColormap | CWEventMask | CWOverrideRedirect, &attr);
        XWarpPointer(dpy, None, win, 0, 0, 0, 0, 0, 0);
		XMapRaised(dpy, win);
        XGrabKeyboard(dpy, win, True, GrabModeAsync, GrabModeAsync, CurrentTime);
        XGrabPointer(dpy, win, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, win, None, CurrentTime);
    }else{/* create a small window for debugging*/
        win = XCreateWindow(dpy, RootWindow(dpy, vi->screen),
            0, 0, width, height, 0, vi->depth, InputOutput, vi->visual,
            CWBorderPixel | CWColormap | CWEventMask, &attr);
        /* only set window title and handle wm_delete_events if in windowed mode */
        wmDelete = XInternAtom(dpy, "WM_DELETE_WINDOW", True);
        XSetWMProtocols(dpy, win, &wmDelete, 1);
		char title[]="Debugging";
        XSetStandardProperties(dpy, win, title, title, None, NULL, 0, NULL);
        XMapRaised(dpy, win);
    }   
	XFree(modes);    
    glXMakeCurrent(dpy, win, ctx);// connect the glx-context to the window and get real size
	Window winDummy;
    unsigned int borderDummy,x,y,depth;
    XGetGeometry(dpy, win, &winDummy, &x, &y, &width, &height, &borderDummy, &depth); 
printf("Actual size %i, %i\n",width, height); 
    if (!glXIsDirect(dpy, ctx)){printf("No Direct Rendering\n");return -1;}
	
	//get OpenGL2 procs & buffers
	if(!LoadShaderProcs())return -1;						// Get Addresses to 2.0 functionality
	GLuint texture,fbo[2],rttex[2];
	glGenFramebuffers(2, fbo);								// Rendering to buffers
	glGenTextures(2, rttex);								// Create named textures to render to
	glGenTextures(1, &texture);								// For keyboard buffer
	glActiveTexture(GL_TEXTURE0);							// Render to texture 0
	int i;
	for(i=0;i<2;i++){
		glBindTexture(GL_TEXTURE_2D, rttex[i]);					// Bind to the render texture
		SetTexParams(width,height);								// Create it and set params
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo[i]);				//Bind to the frame buffer
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rttex[i], 0);  //Attach texture  
		GLuint status = glCheckFramebufferStatus(GL_FRAMEBUFFER); 
		if (status != GL_FRAMEBUFFER_COMPLETE) {glBindFramebuffer(GL_FRAMEBUFFER, 0);return -1;}//Bail
	}
	glViewport(0,0,width,height);// Set the Viewport
	glClear(GL_COLOR_BUFFER_BIT);
	//create the programs bufferA & image
	GLuint P_A, VS_A, FS_A, P_I, VS_I, FS_I;
	GLint zBufA, zTexA, zUniA, zBufI, zTexI, zUniI;
	char *FSscript=(char *)malloc(65536); //max text for scripts
	sprintf(FSscript,fsh,bufferA);
	if(!createprogram(VSscript, FSscript, &P_A, &VS_A, &FS_A)){free(FSscript);return -1;}
	glUseProgram(P_A);
	zUniA=glGetUniformLocation(P_A,"Zuni");//set uniforms
	zBufA=glGetUniformLocation(P_A,"Zbuf");
	zTexA=glGetUniformLocation(P_A,"Ztex");
	sprintf(FSscript,fsh,image);
	if(!createprogram(VSscript, FSscript, &P_I, &VS_I, &FS_I)){free(FSscript);return -1;}
	glUseProgram(P_I);
	zUniI=glGetUniformLocation(P_I,"Zuni");
	zBufI=glGetUniformLocation(P_I,"Zbuf");
	zTexI=glGetUniformLocation(P_I,"Ztex");
	free(FSscript);free(bufferA);free(image);
 
	//now the demo loop
#define ARRAY_SIZE 16
	BYTE bytKeys[256];for(i=0;i<256;i++)bytKeys[i]=0;
	int iMouse=0,iFrame=0;
	Bool bQuit=False,bKeys=True;
	int lastX=0,lastY=0,iBuf=0;
	const int ZERO=0,ONE=1;
	float u[ARRAY_SIZE]; for(i=0;i<ARRAY_SIZE;i++)u[i]=0.0;
	u[0]=(float)width;//setting up uniforms
	u[1]=(float)height;
	long iStartTime=getTickCount(),lastTime=iStartTime;
	XEvent event;
    KeySym key;
	while(!bQuit){//press esc to quit
        while (XPending(dpy) > 0){/* handle the events in the queue */
            XNextEvent(dpy, &event);
            switch (event.type){
                case ButtonPress:{
					XButtonPressedEvent *pe=(XButtonPressedEvent *)&event;
					u[3]=(float)pe->x;
					u[4]=(float)(height-pe->y-1);
					if(u[5]<=0){u[5]=u[3];u[6]=u[4];}
					break;}
				case ButtonRelease: 
					if(u[5]>0.0){u[5]=-u[5];u[6]=-u[6];}
					break;
				case MotionNotify:
					if(u[5]>0.0){//only record mouse movement when mouse pressed
						XPointerMovedEvent *pe=(XPointerMovedEvent *)&event;
						u[3]=(float)pe->x;
						u[4]=(float)(height-pe->y-1);
					}
					break;
				case KeyRelease:
                case KeyPress:{
					bKeys=True;
					char buf[2];
                    int len;
                    len = XLookupString(&event.xkey, buf, 1, &key, NULL);
					if(key==XK_Escape)bQuit=True;
					else if(key==XK_BackSpace)iFrame=0;
					else if(key==XK_Left)bytKeys[37]=(event.type==KeyPress?255:0);
					else if(key==XK_Right)bytKeys[39]=(event.type==KeyPress?255:0);
					else if(key==XK_Up)bytKeys[38]=(event.type==KeyPress?255:0);
					else if(key==XK_Down)bytKeys[40]=(event.type==KeyPress?255:0);
					else {bytKeys[buf[0]]=(event.type==KeyPress?255:0);}
                    break;}
                case ClientMessage:
                    if (*XGetAtomName(dpy, event.xclient.message_type) == *"WM_PROTOCOLS")
                    	bQuit = True;
                    break;
                default:
                    break;
            }
        }

		long now=getTickCount();
		u[2]=(float)(now-iStartTime)/1000.0;
		u[7]=(float)iFrame;
		u[8]=(float)(now-lastTime)/1000.0;
		u[12]=now/1000.0;
		
		if(bKeys){setkbtexture(bytKeys, texture);bKeys=False;} //if key change rebind texture
		glBindTexture(GL_TEXTURE_2D, rttex[iBuf]); //ping pong buffers to eliminate feedback
		iBuf=(iBuf+1)%2;
		glBindFramebuffer(GL_FRAMEBUFFER, fbo[iBuf]); //buffer A
		glUseProgram(P_A);	
		glUniform1fv(zUniA,ARRAY_SIZE,u);
		glUniform1iv(zBufA,1,&ZERO);
		glUniform1iv(zTexA,1,&ONE);
		glRects(-1,-1,1,1);
		glBindFramebuffer(GL_FRAMEBUFFER, 0); //image
		glUseProgram(P_I); 
		glUniform1fv(zUniI,ARRAY_SIZE,u);
		glUniform1iv(zBufI,1,&ZERO);
		glUniform1iv(zTexI,1,&ONE);
		glRects(-1,-1,1,1);
		iFrame+=1;
		lastTime=now;
		glXSwapBuffers(dpy, win);
	}

	//clean this mess up
	glDetachShader(P_A,FS_A);glDeleteShader(FS_A);
	glDetachShader(P_A,VS_A);glDeleteShader(VS_A);
	glDeleteProgram(P_A);
	glDetachShader(P_I,FS_I);glDeleteShader(FS_I);
	glDetachShader(P_I,VS_I);glDeleteShader(VS_I);
	glDeleteProgram(P_I);
	glDeleteTextures(1,&texture);
	glDeleteTextures(2,rttex);
	glDeleteFramebuffers(2,fbo);
	glXMakeCurrent(dpy, None, NULL);
	glXDestroyContext(dpy, ctx);
	if(MODE_FULLSCREEN){
        XF86VidModeSwitchToMode(dpy, screen, &deskMode);
        XF86VidModeSetViewPort(dpy, screen, 0, 0);
    }
    XCloseDisplay(dpy);
	return 0;
}


