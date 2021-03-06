//LIBS:-lX11 -lGL -lasound -pthread
//ARGS:pinball.glsl
// ShaderDemoX.c : This is it. Created by eiffie GPLv3
// What is it? Runs simple webgl scripts from ShaderToy under XServer.
// Why? I wanted to run the games offline/native at fullscreen/speed (and learn XServer).
// Usage: Create a textfile and type [bufA] then newline. Copy in code for buffer A
// type [image] on a line by itself then copy in the code from the Image tab.
// Optionally add the [sound] tab as well.
// Save it and run ./ShaderDemoX whateveryounamedit.
// Buffer A=iChannel0 and keyboard=iChannel1
// Press "escape" to quit and "backspace" to reset iFrame.
// You need these standard headers plus a few gl and X11 headers 
// to compile: gcc -o ShaderDemoX ShaderDemoX.c -lX11 -lGL -lasound -pthread

//lots of code taken from Mihael Vrbanec's nehe linux port. Thank you. It still works!
//also ALSA code from Dr Matthias Nagorni

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <GL/glx.h>
#include <GL/gl.h>
//#include <X11/extensions/xf86vmode.h> //for switching video resolution
#include <X11/keysym.h>

#define min(a,b) ((a)<(b)?(a):(b))
#define BYTE unsigned char

Bool bQuit=False; //stops vid & snd
#define ADD_SOUND
#ifdef ADD_SOUND 
#include <pthread.h>
#include <alsa/asoundlib.h>
#define BUFSAMPS 2048
int rate=44100, iSndFrames=0, iDatSamps;
snd_pcm_t *pcm_handle;
short *sndbuf,*snddat=NULL;

snd_pcm_t *open_pcm(char *pcm_name) {
    snd_pcm_t *playback_handle;
    snd_pcm_hw_params_t *hw_params;
    snd_pcm_sw_params_t *sw_params;
    if (snd_pcm_open (&playback_handle, pcm_name, SND_PCM_STREAM_PLAYBACK, 0) < 0) {
        fprintf (stderr, "cannot open audio device %s\n", pcm_name);
      return NULL;//it seems greedy and wants exclusive control?!
    }
    snd_pcm_hw_params_alloca(&hw_params);
    snd_pcm_hw_params_any(playback_handle, hw_params);
    snd_pcm_hw_params_set_access(playback_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(playback_handle, hw_params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_rate_near(playback_handle, hw_params, &rate, 0);
    snd_pcm_hw_params_set_channels(playback_handle, hw_params, 2);
    snd_pcm_hw_params_set_periods(playback_handle, hw_params, 2, 0);
    snd_pcm_hw_params_set_period_size(playback_handle, hw_params, BUFSAMPS, 0);
    snd_pcm_hw_params(playback_handle, hw_params);
    snd_pcm_sw_params_alloca(&sw_params);
    snd_pcm_sw_params_current(playback_handle, sw_params);
    snd_pcm_sw_params_set_avail_min(playback_handle, sw_params, BUFSAMPS);
    snd_pcm_sw_params(playback_handle, sw_params);
    return(playback_handle);
}

int playback_callback (snd_pcm_sframes_t samps) {
   int idx=samps*2*iSndFrames;//shorts
   if(idx+samps*2>iDatSamps){iSndFrames=0;idx=0;}//2 chan 
   memcpy(sndbuf,snddat+idx,samps*4);//2 chan x 2 bytes
   iSndFrames++;
    return snd_pcm_writei(pcm_handle, sndbuf, samps); 
}

void* startSnd(void *arg){
   sndbuf = (short *) malloc (2 * sizeof (short) * BUFSAMPS); //each sample is 2 chans, 2 byte shorts
   if(!sndbuf){free(snddat);return NULL;}
   pcm_handle = open_pcm("hw:0,0");
   if(!pcm_handle){free(sndbuf);free(snddat);return NULL;}
   memset(sndbuf, 0, 2 * sizeof (short) * BUFSAMPS);
   int nfds = snd_pcm_poll_descriptors_count (pcm_handle);
   struct pollfd *pfds = (struct pollfd *)alloca(sizeof(struct pollfd) * nfds);
   snd_pcm_poll_descriptors(pcm_handle, pfds, nfds);
   while(!bQuit){
      if (poll (pfds, nfds, 1000) > 0) {
         int i;
         for (i = 0; i < nfds; i++) {    
            if (pfds[i].revents > 0) { 
               if (playback_callback(BUFSAMPS) < BUFSAMPS) snd_pcm_prepare(pcm_handle);
            }
         } 
      }
   }
   snd_pcm_close(pcm_handle);
   free(sndbuf);free(snddat);
   return NULL;
}
float sgn(float t){return (t<0.0f?-1.0f:1.0f);}
#endif

char *bufferA=NULL,*image=NULL,*sound=NULL;
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
   if(memcmp(buff,"[bufA]",6)){fclose(fp);return False;}
   bufferA=(char *)malloc(65536);
   image=(char *)malloc(65536);
   sound=(char *)malloc(65536);
   bufferA[0]=0;image[0]=0;sound[0]=0;
   txt=bufferA;
   int i=0;
   while (!feof(fp)){//load [bufA] and [image]
      buf=fgets (buff, sizeof (buff), fp); //Read whole line
      if(buf){
         if(!memcmp(buff,"[image]",7)){
            txt=image;i=0;
         }else if(!memcmp(buff,"[sound]",7)){
            txt=sound;i=0;
         }else strcat(txt,buff);
      }
   }
   fclose(fp);
   return (image[0]!=0);
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
   glActiveTexture(GL_TEXTURE0+1);                     // Texture #1
   glBindTexture(GL_TEXTURE_2D, ztexture);               // Tell GL we are using it
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

int main(int argc, char **argv)
{
   if(!argv[1])return -1;
   if(argv[1][0]==0)return -1;
   if(!loadShaders(argv[1]))return -1;
   Display *dpy;
   int screen;
   Window win;
   GLXContext ctx;
   XSetWindowAttributes attr;
   XVisualInfo *vi;
   Colormap cmap;
   Atom wmDelete;
   dpy = XOpenDisplay(0);
   screen = DefaultScreen(dpy);
#ifdef _DEBUG
   unsigned int width=512,height=320;
   Bool bOverride=False;
#else
   unsigned int width,height;
   width=XDisplayWidth(dpy, screen);
   height=XDisplayHeight(dpy, screen);
   Bool bOverride=True;
#endif
   vi = glXChooseVisual(dpy, screen, attrlist);
   if(!vi){printf("No doublebuffering.\n");return -1;}
   ctx = glXCreateContext(dpy, vi, 0, GL_TRUE);
   cmap = XCreateColormap(dpy, RootWindow(dpy, vi->screen), vi->visual, AllocNone);
   attr.colormap = cmap; attr.border_pixel = 0;
   attr.event_mask = KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask;
   attr.override_redirect = bOverride;
   win = XCreateWindow(dpy, RootWindow(dpy, vi->screen),
      0, 0, width, height, 0, vi->depth, InputOutput, vi->visual,
      CWBorderPixel | CWColormap | CWEventMask | CWOverrideRedirect, &attr);
   if(bOverride){
      XWarpPointer(dpy, None, win, 0, 0, 0, 0, 0, 0);
      XMapRaised(dpy, win);
      XGrabKeyboard(dpy, win, True, GrabModeAsync, GrabModeAsync, CurrentTime);
      XGrabPointer(dpy, win, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, win, None, CurrentTime);
   }else{
      XMapRaised(dpy, win);
      XSetStandardProperties(dpy, win, "ShaderDemoX", "ShaderDemoX", None, NULL, 0, NULL);
   }   
   glXMakeCurrent(dpy, win, ctx);// connect the glx-context to the window and get real size
   Window winDummy;
   unsigned int borderDummy,x,y,depth;
   XGetGeometry(dpy, win, &winDummy, &x, &y, &width, &height, &borderDummy, &depth); 
   if (!glXIsDirect(dpy, ctx)){printf("No Direct Rendering\n");return -1;}
   
   //setup textures & buffers
   GLuint texture,fbo[2],rttex[2];
   glGenFramebuffers(2, fbo);                        // Rendering to buffers
   glGenTextures(2, rttex);                        // Create named textures to render to
   glGenTextures(1, &texture);                        // For keyboard buffer
   glActiveTexture(GL_TEXTURE0);                     // Render to texture 0
   int i;
   for(i=0;i<2;i++){
      glBindTexture(GL_TEXTURE_2D, rttex[i]);               // Bind to the render texture
      SetTexParams(width,height);                        // Create it and set params
      glBindTexture(GL_TEXTURE_2D, 0);
      glBindFramebuffer(GL_FRAMEBUFFER, fbo[i]);            //Bind to the frame buffer
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
#ifdef ADD_SOUND
   pthread_t tid;
   if(sound[0]!=0){
      char fssh[]="uniform float Zuni[2];\nvec2 mainSound(in float);\n\
         void main(){float t=floor(gl_FragCoord.y)*Zuni[0]+floor(gl_FragCoord.x);\n\
         vec2 v1=mainSound(t*2.0/Zuni[1]),v2=mainSound((t*2.0+1.0)/Zuni[1]);\n\
         gl_FragColor=clamp(vec4(v1,v2),-1.0,1.0);}\n%s";
      GLuint P_S, VS_S, FS_S;//create the program and shaders
      sprintf(FSscript,fssh,sound);
      if(createprogram(VSscript, FSscript, &P_S, &VS_S, &FS_S)){      
         iDatSamps=width*height*4; //2 samples per pixel x 2 channels
         snddat=(short *)malloc(iDatSamps*sizeof(short));
         if(snddat){
            float *dat=(float *)malloc(iDatSamps*sizeof(float));
            if(dat){
               glBindFramebuffer(GL_FRAMEBUFFER, fbo[1]); 
               glUseProgram(P_S);
               float u[2];u[0]=(float)width;u[1]=(float)rate;//sample rate
               GLint zUniS=glGetUniformLocation(P_S,"Zuni");//get uniform location
               glUniform1fv(zUniS,2,u);
               glRects(-1,-1,1,1);
               glReadPixels(0,0,width,height,GL_RGBA,GL_FLOAT,(void *)dat);
               for(i=0;i<iDatSamps;i++){
                  float f=dat[i]*32766.0f;
                  snddat[i]=(short)(f+sgn(f)*0.5f);
               }
               free(dat);
            }
         }
         glDetachShader(P_S,FS_S);glDeleteShader(FS_S);
         glDetachShader(P_S,VS_S);glDeleteShader(VS_S);
         glDeleteProgram(P_S);
      }
      if(snddat)pthread_create(&tid, NULL, &startSnd, NULL);
   }
#endif
   free(FSscript);free(bufferA);free(image);free(sound);
 
   //now the demo loop
#define ARRAY_SIZE 16
   BYTE bytKeys[256];for(i=0;i<256;i++)bytKeys[i]=0;
   int iMouse=0,iFrame=0;
   Bool bKeys=True;
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
            int len = XLookupString(&event.xkey, buf, 1, &key, NULL);
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
   XCloseDisplay(dpy);
#ifdef ADD_SOUND
   if(snddat)pthread_join(tid,NULL);
#endif
   return 0;
}

