all:
	gcc stickAnimator.c -L./Linux -lglfw3 -ldl -lm -lX11 -lglad -lGL -lGLU -lpthread -DOS_LINUX -DDEBUGGING_FLAG -Wall -o stickAnimator.o
rel:
	gcc stickAnimator.c -L./Linux -lglfw3 -ldl -lm -lX11 -lglad -lGL -lGLU -lpthread -DOS_LINUX -O3 -o stickAnimator.o
win:
	gcc stickAnimator.c -L./Windows -lglfw3 -lopengl32 -lgdi32 -lglad -lole32 -luuid -lwsock32 -lWs2_32 -DOS_WINDOWS -DDEBUGGING_FLAG -Wall -o stickAnimator.exe
winrel:
	gcc stickAnimator.c -L./Windows -lglfw3 -lopengl32 -lgdi32 -lglad -lole32 -luuid -lwsock32 -lWs2_32 -DOS_WINDOWS -O3 -o stickAnimator.exe
