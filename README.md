# ShaderDemo
Runs simple ShaderToy games/demos that use buffer A &amp; keyboard.
This was just a quick project to run webgl games offline and fullscreen.
There are two seperate versions for Windows (32 bit api) and Linux (ShaderDemoX).
It accepts a file on the command line with the following format...
Start the file in NOTEPAD for Windows or gedit for Linux (line endings must be correct)!
Type
[bufA]
Start a new line after the ] and paste in the code from Buffer A.
Then type
[image]
and paste in the code from the Image tab.
In the code for buffer A and Image iChannel0 = buffer A and iChannel1 = keyboard. 
No other inputs are available. If it compiles it will run if not it will display the OpenGL compile error. 
"Escape" ends the program and "backspace" resets iFrame to 0.
