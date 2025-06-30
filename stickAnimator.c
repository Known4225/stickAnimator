#include "include/turtleTools.h"
#include "include/osTools.h"
#include <time.h>

typedef enum {
    STICK_STYLE_OPEN_HEAD = 0,
    STICK_STYLE_CLOSED_HEAD = 1,
} stick_style_t;

typedef enum {
    STICK_X = 0,
    STICK_Y = 1,
    STICK_SIZE = 2,
    STICK_STYLE = 3,
    STICK_RED = 4,
    STICK_GREEN = 5,
    STICK_BLUE = 6,
    STICK_ALPHA = 7,
    STICK_LOWER_BODY = 16,
    STICK_UPPER_BODY = 17,
    STICK_HEAD = 18,
    STICK_LEFT_UPPER_ARM = 19,
    STICK_LEFT_LOWER_ARM = 20,
    STICK_RIGHT_UPPER_ARM = 21,
    STICK_RIGHT_LOWER_ARM = 22,
    STICK_LEFT_UPPER_LEG = 23,
    STICK_LEFT_LOWER_LEG = 24,
    STICK_RIGHT_UPPER_LEG = 25,
    STICK_RIGHT_LOWER_LEG = 26,
} stick_index_t;

typedef enum {
    STICK_SIZE_LOWER_BODY = 22,
    STICK_SIZE_UPPER_BODY = 22,
    STICK_SIZE_HEAD_RADIUS = 17,
    STICK_SIZE_HEAD = 15,
    STICK_SIZE_UPPER_ARM = 28,
    STICK_SIZE_LOWER_ARM = 28,
    STICK_SIZE_UPPER_LEG = 35,
    STICK_SIZE_LOWER_LEG = 35,
} stick_relative_sizes_t;

typedef struct {
    /* list of sticks */
    list_t *sticks;
    /* stick format 
    [
        [positionX, positionY, size, style, red, green, blue, alpha, reserved, reserved, reserved, reserved, reserved, reserved, reserved, reserved
        lower body, upper body, head, left upper arm, left lower arm, right upper arm, right lower arm, left upper leg, left lower leg, right upper leg, right lower leg]
    ]
    */
    list_t *currentAnimation;
    /* currentAnimation format 
    [
        [positionX, positionY, lower body, upper body, head, left upper arm, left lower arm, right upper arm, right lower arm, left upper leg, left lower leg, right upper leg, right lower leg]
    ]
    */
    list_t *animations;
    /* animations format
    [
        [name, reserved, reserved, reserved, reserved, reserved, reserved, reserved,
            [changeX, changeY, lower body, upper body, head, left upper arm, left lower arm, right upper arm, right lower arm, left upper leg, left lower leg, right upper leg, right lower leg
            changeX, changeY, lower body, upper body, head, left upper arm, left lower arm, right upper arm, right lower arm, left upper leg, left lower leg, right upper leg, right lower leg
            changeX, changeY, lower body, upper body, head, left upper arm, left lower arm, right upper arm, right lower arm, left upper leg, left lower leg, right upper leg, right lower leg
            changeX, changeY, lower body, upper body, head, left upper arm, left lower arm, right upper arm, right lower arm, left upper leg, left lower leg, right upper leg, right lower leg
            ]
        ]
    ]
    */
    list_t *dotPositions;
    list_t *limbParents;
    list_t *limbChildren;
    int8_t keys[16];

    int32_t mouseStickIndex;
    int32_t mouseHoverDot;
    int32_t mouseDraggingDot;
    double mouseHoverDistance;
    double mouseAnchorX;
    double mouseAnchorY;
    double positionAnchorX;
    double positionAnchorY;

    int8_t mode; // 0 - normal mode, 1 - editing mode
    tt_switch_t *modeSwitch;
    double onionNumber;
    tt_slider_t *onionSlider;
    int8_t frame;
    tt_button_t *frameButton;
    int32_t frameNumber; // number of frame being edited
    double frameBarX;
    double frameBarY;
    double frameScroll;
    tt_scrollbar_t *frameScrollbar;
    int32_t animationSaveIndex;

} stickAnimator_t;

stickAnimator_t self;

void createStick(list_t *stick) {
    list_append(stick, (unitype) -280.0, 'd'); // position X
    list_append(stick, (unitype) -50.0, 'd'); // position Y
    list_append(stick, (unitype) 0.5, 'd'); // size
    list_append(stick, (unitype) STICK_STYLE_OPEN_HEAD, 'i'); // style
    list_append(stick, (unitype) 110.0, 'd'); // red
    list_append(stick, (unitype) 16.0, 'd'); // green
    list_append(stick, (unitype) 190.0, 'd'); // blue
    list_append(stick, (unitype) 0.0, 'd'); // alpha
    for (int32_t i = 0; i < 8; i++) {
        list_append(stick, (unitype) 0.0, 'd'); // reserved
    }
    list_append(stick, (unitype) 5.0, 'd'); // lower body
    list_append(stick, (unitype) 5.0, 'd'); // upper body
    list_append(stick, (unitype) 5.0, 'd'); // head
    list_append(stick, (unitype) -150.0, 'd'); // upper left arm
    list_append(stick, (unitype) 170.0, 'd'); // lower left arm
    list_append(stick, (unitype) 170.0, 'd'); // upper right arm
    list_append(stick, (unitype) 150.0, 'd'); // lower right arm
    list_append(stick, (unitype) -175.0, 'd'); // upper left leg
    list_append(stick, (unitype) -170.0, 'd'); // lower left leg
    list_append(stick, (unitype) 160.0, 'd'); // upper right leg
    list_append(stick, (unitype) 170.0, 'd'); // lower right leg
}

void init() {
    self.sticks = list_init();
    /* create default stick */
    list_t *defaultStick = list_init();
    createStick(defaultStick);
    list_append(self.sticks, (unitype) defaultStick, 'r');
    /* create limb tree */
    self.limbParents = list_init();
    self.dotPositions = list_init();
    self.limbChildren = list_init();
    for (int32_t i = 0; i < 64; i++) {
        list_append(self.dotPositions, (unitype) 0.0, 'd');
        list_append(self.limbParents, (unitype) 0, 'i');
        list_append(self.limbChildren, (unitype) list_init(), 'r');
    }
    self.limbParents -> data[STICK_LOWER_BODY].i = 0;
    self.limbParents -> data[STICK_UPPER_BODY].i = STICK_LOWER_BODY;
    self.limbParents -> data[STICK_HEAD].i = STICK_UPPER_BODY;
    self.limbParents -> data[STICK_LEFT_UPPER_ARM].i = STICK_UPPER_BODY;
    self.limbParents -> data[STICK_RIGHT_UPPER_ARM].i = STICK_UPPER_BODY;
    self.limbParents -> data[STICK_LEFT_LOWER_ARM].i = STICK_LEFT_UPPER_ARM;
    self.limbParents -> data[STICK_RIGHT_LOWER_ARM].i = STICK_RIGHT_UPPER_ARM;
    self.limbParents -> data[STICK_LEFT_UPPER_LEG].i = 0;
    self.limbParents -> data[STICK_RIGHT_UPPER_LEG].i = 0;
    self.limbParents -> data[STICK_LEFT_LOWER_LEG].i = STICK_LEFT_UPPER_LEG;
    self.limbParents -> data[STICK_RIGHT_LOWER_LEG].i = STICK_RIGHT_UPPER_LEG;

    list_append(self.limbChildren -> data[STICK_LOWER_BODY].r, (unitype) STICK_UPPER_BODY, 'i');
    list_append(self.limbChildren -> data[STICK_LOWER_BODY].r, (unitype) STICK_HEAD, 'i');
    list_append(self.limbChildren -> data[STICK_LOWER_BODY].r, (unitype) STICK_LEFT_UPPER_ARM, 'i');
    list_append(self.limbChildren -> data[STICK_LOWER_BODY].r, (unitype) STICK_RIGHT_UPPER_ARM, 'i');
    list_append(self.limbChildren -> data[STICK_LOWER_BODY].r, (unitype) STICK_LEFT_LOWER_ARM, 'i');
    list_append(self.limbChildren -> data[STICK_LOWER_BODY].r, (unitype) STICK_RIGHT_LOWER_ARM, 'i');

    list_append(self.limbChildren -> data[STICK_UPPER_BODY].r, (unitype) STICK_HEAD, 'i');
    list_append(self.limbChildren -> data[STICK_UPPER_BODY].r, (unitype) STICK_LEFT_UPPER_ARM, 'i');
    list_append(self.limbChildren -> data[STICK_UPPER_BODY].r, (unitype) STICK_RIGHT_UPPER_ARM, 'i');
    list_append(self.limbChildren -> data[STICK_UPPER_BODY].r, (unitype) STICK_LEFT_LOWER_ARM, 'i');
    list_append(self.limbChildren -> data[STICK_UPPER_BODY].r, (unitype) STICK_RIGHT_LOWER_ARM, 'i');

    list_append(self.limbChildren -> data[STICK_LEFT_UPPER_ARM].r, (unitype) STICK_LEFT_LOWER_ARM, 'i');
    list_append(self.limbChildren -> data[STICK_RIGHT_UPPER_ARM].r, (unitype) STICK_RIGHT_LOWER_ARM, 'i');
    list_append(self.limbChildren -> data[STICK_LEFT_UPPER_LEG].r, (unitype) STICK_LEFT_LOWER_LEG, 'i');
    list_append(self.limbChildren -> data[STICK_RIGHT_UPPER_LEG].r, (unitype) STICK_RIGHT_LOWER_LEG, 'i');

    self.currentAnimation = list_init();
    self.mouseHoverDot = -1;
    self.mouseDraggingDot = -1;
    self.mouseAnchorX = 0;
    self.mouseAnchorY = 0;

    /* UI elements */
    self.mode = 0;
    self.modeSwitch = switchInit("Mode", &self.mode, -305, 152, 6);
    self.onionNumber = 0;
    self.onionSlider = sliderInit("Onion", &self.onionNumber, TT_SLIDER_HORIZONTAL, TT_SLIDER_ALIGN_CENTER, -220, 152, 6, 40, 0, 3, 1);
    self.frame = 0;
    self.frameButton = buttonInit("Frame", &self.frame, -270, 152, 6);
    self.frameNumber = 0;
    self.frameBarX = -180;
    self.frameBarY = 160;
    self.frameScroll = 0;
    self.frameScrollbar = scrollbarInit(&self.frameScroll, TT_SCROLLBAR_HORIZONTAL, (self.frameBarX + 315) / 2, self.frameBarY - 41, 6, 492, 90);

    /* animations */
    self.animations = list_init();
    self.animationSaveIndex = 0;
}

void insertFrame(int32_t index, int32_t frameIndex) {
    list_t *stick = self.sticks -> data[index].r;
    list_t *constructedList = list_init();
    list_append(constructedList, stick -> data[STICK_X], 'd');
    list_append(constructedList, stick -> data[STICK_Y], 'd');
    list_append(constructedList, stick -> data[STICK_LOWER_BODY], 'd');
    list_append(constructedList, stick -> data[STICK_UPPER_BODY], 'd');
    list_append(constructedList, stick -> data[STICK_HEAD], 'd');
    list_append(constructedList, stick -> data[STICK_LEFT_UPPER_ARM], 'd');
    list_append(constructedList, stick -> data[STICK_LEFT_LOWER_ARM], 'd');
    list_append(constructedList, stick -> data[STICK_RIGHT_UPPER_ARM], 'd');
    list_append(constructedList, stick -> data[STICK_RIGHT_LOWER_ARM], 'd');
    list_append(constructedList, stick -> data[STICK_LEFT_UPPER_LEG], 'd');
    list_append(constructedList, stick -> data[STICK_LEFT_LOWER_LEG], 'd');
    list_append(constructedList, stick -> data[STICK_RIGHT_LOWER_LEG], 'd');
    list_append(constructedList, stick -> data[STICK_RIGHT_UPPER_LEG], 'd');
    list_insert(self.currentAnimation, frameIndex, (unitype) constructedList, 'r');
}

void handleUI() {
    // printf("%d\n", self.frameNumber);
    turtleRectangleColor(-320, 180, -181, 100, 255, 255, 255, 0);
    if (self.mode) {
        self.onionSlider -> enabled = TT_ELEMENT_ENABLED;
        self.frameButton -> enabled = TT_ELEMENT_ENABLED;
        if (self.currentAnimation -> length >= 8) {
            self.frameScrollbar -> enabled = TT_ELEMENT_ENABLED;
            self.frameScrollbar -> barPercentage = 100 / (self.currentAnimation -> length / 7.8);
        }
        if (self.frame) {
            insertFrame(0, self.frameNumber);
            self.frameNumber++;
        }
    } else {
        self.onionSlider -> enabled = TT_ELEMENT_HIDE;
        self.frameButton -> enabled = TT_ELEMENT_HIDE;
        self.frameScrollbar -> enabled = TT_ELEMENT_HIDE;
    }
}

void renderStick(list_t *stick) {
    double xpos = stick -> data[STICK_X].d;
    double ypos = stick -> data[STICK_Y].d;
    /* draw body */
    turtlePenColorAlpha(stick -> data[STICK_RED].d, stick -> data[STICK_GREEN].d, stick -> data[STICK_BLUE].d, stick -> data[STICK_ALPHA].d);
    turtlePenSize(stick -> data[STICK_SIZE].d * 9);
    turtleGoto(xpos, ypos);
    turtlePenDown();
    turtleGoto(turtle.x + sin(stick -> data[STICK_LOWER_BODY].d / 57.2958) * STICK_SIZE_LOWER_BODY * stick -> data[STICK_SIZE].d, turtle.y + cos(stick -> data[STICK_LOWER_BODY].d / 57.2958) * STICK_SIZE_LOWER_BODY * stick -> data[STICK_SIZE].d);
    turtleGoto(turtle.x + sin(stick -> data[STICK_UPPER_BODY].d / 57.2958) * STICK_SIZE_UPPER_BODY * stick -> data[STICK_SIZE].d, turtle.y + cos(stick -> data[STICK_UPPER_BODY].d / 57.2958) * STICK_SIZE_UPPER_BODY * stick -> data[STICK_SIZE].d);
    double savedX = turtle.x;
    double savedY = turtle.y;
    /* draw arms */
    turtleGoto(turtle.x + sin(stick -> data[STICK_LEFT_UPPER_ARM].d / 57.2958) * STICK_SIZE_UPPER_ARM * stick -> data[STICK_SIZE].d, turtle.y + cos(stick -> data[STICK_LEFT_UPPER_ARM].d / 57.2958) * STICK_SIZE_UPPER_ARM * stick -> data[STICK_SIZE].d);
    turtleGoto(turtle.x + sin(stick -> data[STICK_LEFT_LOWER_ARM].d / 57.2958) * STICK_SIZE_LOWER_ARM * stick -> data[STICK_SIZE].d, turtle.y + cos(stick -> data[STICK_LEFT_LOWER_ARM].d / 57.2958) * STICK_SIZE_LOWER_ARM * stick -> data[STICK_SIZE].d);
    turtlePenUp();
    turtleGoto(savedX, savedY);
    turtlePenDown();
    turtleGoto(turtle.x + sin(stick -> data[STICK_RIGHT_UPPER_ARM].d / 57.2958) * STICK_SIZE_UPPER_ARM * stick -> data[STICK_SIZE].d, turtle.y + cos(stick -> data[STICK_RIGHT_UPPER_ARM].d / 57.2958) * STICK_SIZE_UPPER_ARM * stick -> data[STICK_SIZE].d);
    turtleGoto(turtle.x + sin(stick -> data[STICK_RIGHT_LOWER_ARM].d / 57.2958) * STICK_SIZE_LOWER_ARM * stick -> data[STICK_SIZE].d, turtle.y + cos(stick -> data[STICK_RIGHT_LOWER_ARM].d / 57.2958) * STICK_SIZE_LOWER_ARM * stick -> data[STICK_SIZE].d);
    turtlePenUp();

    /* draw legs */
    turtleGoto(xpos, ypos);
    turtlePenDown();
    turtleGoto(turtle.x + sin(stick -> data[STICK_LEFT_UPPER_LEG].d / 57.2958) * STICK_SIZE_UPPER_LEG * stick -> data[STICK_SIZE].d, turtle.y + cos(stick -> data[STICK_LEFT_UPPER_LEG].d / 57.2958) * STICK_SIZE_UPPER_LEG * stick -> data[STICK_SIZE].d);
    turtleGoto(turtle.x + sin(stick -> data[STICK_LEFT_LOWER_LEG].d / 57.2958) * STICK_SIZE_LOWER_LEG * stick -> data[STICK_SIZE].d, turtle.y + cos(stick -> data[STICK_LEFT_LOWER_LEG].d / 57.2958) * STICK_SIZE_LOWER_LEG * stick -> data[STICK_SIZE].d);
    turtlePenUp();
    turtleGoto(xpos, ypos);
    turtlePenDown();
    turtleGoto(turtle.x + sin(stick -> data[STICK_RIGHT_UPPER_LEG].d / 57.2958) * STICK_SIZE_UPPER_LEG * stick -> data[STICK_SIZE].d, turtle.y + cos(stick -> data[STICK_RIGHT_UPPER_LEG].d / 57.2958) * STICK_SIZE_UPPER_LEG * stick -> data[STICK_SIZE].d);
    turtleGoto(turtle.x + sin(stick -> data[STICK_RIGHT_LOWER_LEG].d / 57.2958) * STICK_SIZE_LOWER_LEG * stick -> data[STICK_SIZE].d, turtle.y + cos(stick -> data[STICK_RIGHT_LOWER_LEG].d / 57.2958) * STICK_SIZE_LOWER_LEG * stick -> data[STICK_SIZE].d);
    turtlePenUp();

    /* draw head */
    if (stick -> data[STICK_STYLE].i == STICK_STYLE_OPEN_HEAD) {
        // turtlePenSize(STICK_SIZE_HEAD_RADIUS / 4 * stick -> data[STICK_SIZE].d);
        // savedX += sin(stick -> data[STICK_HEAD].d / 57.2958) * STICK_SIZE_HEAD * stick -> data[STICK_SIZE].d;
        // savedY += cos(stick -> data[STICK_HEAD].d / 57.2958) * STICK_SIZE_HEAD * stick -> data[STICK_SIZE].d;
        // turtleGoto(savedX, savedY + STICK_SIZE_HEAD_RADIUS * stick -> data[STICK_SIZE].d);
        // turtlePenDown();
        // double prez = turtle.circleprez * 4 * log(2.71 + turtle.pensize);
        // for (double i = 0; i < prez + 1; i++) {
        //     turtleGoto(savedX + sin((360 / prez * i) / 57.2958) * STICK_SIZE_HEAD_RADIUS * stick -> data[STICK_SIZE].d, savedY + cos((360 / prez * i) / 57.2958) * STICK_SIZE_HEAD_RADIUS * stick -> data[STICK_SIZE].d);
        // }
        // turtlePenUp();
        turtlePenSize(STICK_SIZE_HEAD_RADIUS * 2 * stick -> data[STICK_SIZE].d);
        turtleGoto(savedX + sin(stick -> data[STICK_HEAD].d / 57.2958) * STICK_SIZE_HEAD * stick -> data[STICK_SIZE].d, savedY + cos(stick -> data[STICK_HEAD].d / 57.2958) * STICK_SIZE_HEAD * stick -> data[STICK_SIZE].d);
        turtlePenDown();
        turtlePenUp();
        turtlePenColor(255, 255, 255);
        turtlePenSize(turtle.pensize * 2 * 0.8);
        turtlePenDown();
        turtlePenUp();
    } else if (stick -> data[STICK_STYLE].i == STICK_STYLE_CLOSED_HEAD) {
        turtlePenSize(STICK_SIZE_HEAD_RADIUS * 2 * stick -> data[STICK_SIZE].d);
        turtleGoto(savedX + sin(stick -> data[STICK_HEAD].d / 57.2958) * STICK_SIZE_HEAD * stick -> data[STICK_SIZE].d, savedY + cos(stick -> data[STICK_HEAD].d / 57.2958) * STICK_SIZE_HEAD * stick -> data[STICK_SIZE].d);
        turtlePenDown();
        turtlePenUp();
    }
}

void checkMouse(list_t *stick, int32_t index, int32_t dotIndex) {
    self.dotPositions -> data[dotIndex * 2].d = turtle.x;
    self.dotPositions -> data[dotIndex * 2 + 1].d = turtle.y;
    if (turtle.mouseX > turtle.x - stick -> data[STICK_SIZE].d * 4 && turtle.mouseX < turtle.x + stick -> data[STICK_SIZE].d * 4 && turtle.mouseY > turtle.y - stick -> data[STICK_SIZE].d * 4 && turtle.mouseY < turtle.y + stick -> data[STICK_SIZE].d * 4) {
        if (self.mouseHoverDot == -1 || self.mouseHoverDistance > (turtle.mouseX - turtle.x) * (turtle.mouseX - turtle.x) + (turtle.mouseY - turtle.y) * (turtle.mouseY - turtle.y)) {
            self.mouseHoverDot = dotIndex;
            self.mouseHoverDistance = (turtle.mouseX - turtle.x) * (turtle.mouseX - turtle.x) + (turtle.mouseY - turtle.y) * (turtle.mouseY - turtle.y);
            self.mouseStickIndex = index;
        }
    }
}

void renderDots(int32_t index) {
    self.mouseHoverDot = -1;
    list_t *stick = self.sticks -> data[index].r;
    double xpos = stick -> data[STICK_X].d;
    double ypos = stick -> data[STICK_Y].d;
    /* draw body */
    turtlePenColor(225, 70, 0);
    turtlePenSize(stick -> data[STICK_SIZE].d * 6);
    turtleGoto(xpos, ypos);
    turtlePenDown();
    turtlePenUp();
    checkMouse(stick, index, 0);
    turtlePenColor(0, 0, 0);
    turtleGoto(turtle.x + sin(stick -> data[STICK_LOWER_BODY].d / 57.2958) * STICK_SIZE_LOWER_BODY * stick -> data[STICK_SIZE].d, turtle.y + cos(stick -> data[STICK_LOWER_BODY].d / 57.2958) * STICK_SIZE_LOWER_BODY * stick -> data[STICK_SIZE].d);
    turtlePenDown();
    turtlePenUp();
    checkMouse(stick, index, STICK_LOWER_BODY);
    turtleGoto(turtle.x + sin(stick -> data[STICK_UPPER_BODY].d / 57.2958) * STICK_SIZE_UPPER_BODY * stick -> data[STICK_SIZE].d, turtle.y + cos(stick -> data[STICK_UPPER_BODY].d / 57.2958) * STICK_SIZE_UPPER_BODY * stick -> data[STICK_SIZE].d);
    turtlePenDown();
    turtlePenUp();
    checkMouse(stick, index, STICK_UPPER_BODY);
    double savedX = turtle.x;
    double savedY = turtle.y;
    /* draw arms */
    turtleGoto(turtle.x + sin(stick -> data[STICK_LEFT_UPPER_ARM].d / 57.2958) * STICK_SIZE_UPPER_ARM * stick -> data[STICK_SIZE].d, turtle.y + cos(stick -> data[STICK_LEFT_UPPER_ARM].d / 57.2958) * STICK_SIZE_UPPER_ARM * stick -> data[STICK_SIZE].d);
    turtlePenDown();
    turtlePenUp();
    checkMouse(stick, index, STICK_LEFT_UPPER_ARM);
    turtleGoto(turtle.x + sin(stick -> data[STICK_LEFT_LOWER_ARM].d / 57.2958) * STICK_SIZE_LOWER_ARM * stick -> data[STICK_SIZE].d, turtle.y + cos(stick -> data[STICK_LEFT_LOWER_ARM].d / 57.2958) * STICK_SIZE_LOWER_ARM * stick -> data[STICK_SIZE].d);
    turtlePenDown();
    turtlePenUp();
    checkMouse(stick, index, STICK_LEFT_LOWER_ARM);
    turtleGoto(savedX, savedY);
    turtleGoto(turtle.x + sin(stick -> data[STICK_RIGHT_UPPER_ARM].d / 57.2958) * STICK_SIZE_UPPER_ARM * stick -> data[STICK_SIZE].d, turtle.y + cos(stick -> data[STICK_RIGHT_UPPER_ARM].d / 57.2958) * STICK_SIZE_UPPER_ARM * stick -> data[STICK_SIZE].d);
    turtlePenDown();
    turtlePenUp();
    checkMouse(stick, index, STICK_RIGHT_UPPER_ARM);
    turtleGoto(turtle.x + sin(stick -> data[STICK_RIGHT_LOWER_ARM].d / 57.2958) * STICK_SIZE_LOWER_ARM * stick -> data[STICK_SIZE].d, turtle.y + cos(stick -> data[STICK_RIGHT_LOWER_ARM].d / 57.2958) * STICK_SIZE_LOWER_ARM * stick -> data[STICK_SIZE].d);
    turtlePenDown();
    turtlePenUp();
    checkMouse(stick, index, STICK_RIGHT_LOWER_ARM);

    /* draw legs */
    turtleGoto(xpos, ypos);
    turtleGoto(turtle.x + sin(stick -> data[STICK_LEFT_UPPER_LEG].d / 57.2958) * STICK_SIZE_UPPER_LEG * stick -> data[STICK_SIZE].d, turtle.y + cos(stick -> data[STICK_LEFT_UPPER_LEG].d / 57.2958) * STICK_SIZE_UPPER_LEG * stick -> data[STICK_SIZE].d);
    turtlePenDown();
    turtlePenUp();
    checkMouse(stick, index, STICK_LEFT_UPPER_LEG);
    turtleGoto(turtle.x + sin(stick -> data[STICK_LEFT_LOWER_LEG].d / 57.2958) * STICK_SIZE_LOWER_LEG * stick -> data[STICK_SIZE].d, turtle.y + cos(stick -> data[STICK_LEFT_LOWER_LEG].d / 57.2958) * STICK_SIZE_LOWER_LEG * stick -> data[STICK_SIZE].d);
    turtlePenDown();
    turtlePenUp();
    checkMouse(stick, index, STICK_LEFT_LOWER_LEG);
    turtleGoto(xpos, ypos);
    turtleGoto(turtle.x + sin(stick -> data[STICK_RIGHT_UPPER_LEG].d / 57.2958) * STICK_SIZE_UPPER_LEG * stick -> data[STICK_SIZE].d, turtle.y + cos(stick -> data[STICK_RIGHT_UPPER_LEG].d / 57.2958) * STICK_SIZE_UPPER_LEG * stick -> data[STICK_SIZE].d);
    turtlePenDown();
    turtlePenUp();
    checkMouse(stick, index, STICK_RIGHT_UPPER_LEG);
    turtleGoto(turtle.x + sin(stick -> data[STICK_RIGHT_LOWER_LEG].d / 57.2958) * STICK_SIZE_LOWER_LEG * stick -> data[STICK_SIZE].d, turtle.y + cos(stick -> data[STICK_RIGHT_LOWER_LEG].d / 57.2958) * STICK_SIZE_LOWER_LEG * stick -> data[STICK_SIZE].d);
    turtlePenDown();
    turtlePenUp();
    checkMouse(stick, index, STICK_RIGHT_LOWER_LEG);

    /* draw head */
    turtleGoto(savedX + sin(stick -> data[STICK_HEAD].d / 57.2958) * STICK_SIZE_HEAD * stick -> data[STICK_SIZE].d, savedY + cos(stick -> data[STICK_HEAD].d / 57.2958) * STICK_SIZE_HEAD * stick -> data[STICK_SIZE].d);
    turtlePenDown();
    turtlePenUp();
    checkMouse(stick, index, STICK_HEAD);
}

void renderFrames() {
    for (int32_t i = 0; i < self.currentAnimation -> length; i++) {
        double frameXLeft = self.frameBarX + i * 66 - self.frameScroll * (self.currentAnimation -> length - 7.5) * 0.66;
        double frameYUp = self.frameBarY;
        double frameXRight = frameXLeft + 64; 
        double frameYDown = self.frameBarY - 36;
        if (frameXRight < self.frameBarX) {
            continue;
        }
        if (frameXLeft > 320) {
            return;
        }
        /* draw box */
        turtleGoto(frameXLeft, frameYUp);
        turtlePenColor(0, 0, 0);
        turtlePenSize(1);
        turtlePenDown();
        turtleGoto(frameXRight, frameYUp);
        turtleGoto(frameXRight, frameYDown);
        turtleGoto(frameXLeft, frameYDown);
        turtleGoto(frameXLeft, frameYUp);
        turtlePenUp();
        turtleTextWriteStringf(frameXLeft + 2, frameYUp - 5, 5, 0, "%d", i);
        /* draw stick */
        list_t *tempStick = list_init();
        createStick(tempStick);
        tempStick -> data[STICK_SIZE].d = 0.05;
        tempStick -> data[STICK_X].d = frameXLeft + ((self.currentAnimation -> data[i].r -> data[0].d + 330) / 660) * (frameXRight - frameXLeft);
        tempStick -> data[STICK_Y].d = frameYDown + ((self.currentAnimation -> data[i].r -> data[1].d + 190) / 380) * (frameYUp - frameYDown);
        tempStick -> data[STICK_LOWER_BODY].d = self.currentAnimation -> data[i].r -> data[2].d;
        tempStick -> data[STICK_UPPER_BODY].d = self.currentAnimation -> data[i].r -> data[3].d;
        tempStick -> data[STICK_HEAD].d = self.currentAnimation -> data[i].r -> data[4].d;
        tempStick -> data[STICK_LEFT_UPPER_ARM].d = self.currentAnimation -> data[i].r -> data[5].d;
        tempStick -> data[STICK_LEFT_LOWER_ARM].d = self.currentAnimation -> data[i].r -> data[6].d;
        tempStick -> data[STICK_RIGHT_UPPER_ARM].d = self.currentAnimation -> data[i].r -> data[7].d;
        tempStick -> data[STICK_RIGHT_LOWER_ARM].d = self.currentAnimation -> data[i].r -> data[8].d;
        tempStick -> data[STICK_LEFT_UPPER_LEG].d = self.currentAnimation -> data[i].r -> data[9].d;
        tempStick -> data[STICK_LEFT_LOWER_LEG].d = self.currentAnimation -> data[i].r -> data[10].d;
        tempStick -> data[STICK_RIGHT_LOWER_LEG].d = self.currentAnimation -> data[i].r -> data[11].d;
        tempStick -> data[STICK_RIGHT_UPPER_LEG].d = self.currentAnimation -> data[i].r -> data[12].d;
        renderStick(tempStick);
        list_free(tempStick);
    }
}

void renderGround() {
    turtlePenColor(0, 0, 0);
    turtlePenSize(4);
    turtleGoto(-320, -86);
    turtlePenDown();
    turtleGoto(320, turtle.y);
    turtlePenUp();
}

/* generate or edit animation from currentAnimation */
void generateAnimation(char *name) {
    if (self.animationSaveIndex < self.animations -> length) {
        /* update existing animation */
    } else {
        /* create animation */
        list_t *newAnimation = list_init();
        list_append(newAnimation, (unitype) name, 's');
        for (int32_t i = 0; i < 7; i++) {
            list_append(newAnimation, (unitype) 0, 'd'); // reserved
        }
        list_t *animationContents = list_init();
        list_copy(animationContents, self.currentAnimation);
        if (self.currentAnimation -> length > 0) {
            animationContents -> data[0].r -> data[0].d = 0;
            animationContents -> data[0].r -> data[1].d = 0;
        }
        for (int32_t i = 0; i < animationContents -> length; i++) {
            if (i > 0) {
                animationContents -> data[i].r -> data[0].d = self.currentAnimation -> data[i].r -> data[0].d - self.currentAnimation -> data[i - 1].r -> data[0].d;
                animationContents -> data[i].r -> data[1].d = self.currentAnimation -> data[i].r -> data[1].d - self.currentAnimation -> data[i - 1].r -> data[1].d;
            }
            list_append(newAnimation, animationContents -> data[i], 'r');
        }
        list_append(self.animations, (unitype) newAnimation, 'r');
    }
    // FILE *fp = fopen(name, "w");
    // list_fprint_emb(fp, self.animations -> data[self.animationSaveIndex].r);
    // fclose(fp);
    list_print(self.animations -> data[self.animationSaveIndex].r);
}

void mouseTick() {
    if (turtleMouseDown() && self.mode) {
        if (self.keys[0] == 0) {
            /* first tick */
            self.keys[0] = 1;
            if (self.mode && self.mouseHoverDot != -1) {
                self.mouseDraggingDot = self.mouseHoverDot;
                self.mouseAnchorX = turtle.mouseX;
                self.mouseAnchorY = turtle.mouseY;
                if (self.mouseDraggingDot == 0) {
                    self.positionAnchorX = self.sticks -> data[self.mouseStickIndex].r -> data[0].d;
                    self.positionAnchorY = self.sticks -> data[self.mouseStickIndex].r -> data[1].d;
                } else {
                    self.positionAnchorX = self.dotPositions -> data[self.limbParents -> data[self.mouseDraggingDot].i * 2].d;
                    self.positionAnchorY = self.dotPositions -> data[self.limbParents -> data[self.mouseDraggingDot].i * 2 + 1].d;
                }
            }
        } else {
            /* mouse held */
            if (self.mouseDraggingDot != -1) {
                if (self.mouseDraggingDot == 0) {
                    /* move position */
                    self.sticks -> data[self.mouseStickIndex].r -> data[0].d = (turtle.mouseX - self.mouseAnchorX) + self.positionAnchorX;
                    self.sticks -> data[self.mouseStickIndex].r -> data[1].d = (turtle.mouseY - self.mouseAnchorY) + self.positionAnchorY;
                } else {
                    /* change angle */
                    double lastAngle = self.sticks -> data[self.mouseStickIndex].r -> data[self.mouseDraggingDot].d;
                    self.sticks -> data[self.mouseStickIndex].r -> data[self.mouseDraggingDot].d = 57.2958 * atan2(turtle.mouseX - self.positionAnchorX, turtle.mouseY - self.positionAnchorY);
                    double angleChange = self.sticks -> data[self.mouseStickIndex].r -> data[self.mouseDraggingDot].d - lastAngle;
                    for (int32_t i = 0; i < self.limbChildren -> data[self.mouseDraggingDot].r -> length; i++) {
                        self.sticks -> data[self.mouseStickIndex].r -> data[self.limbChildren -> data[self.mouseDraggingDot].r -> data[i].i].d += angleChange;
                    }
                }
            }
        }
    } else {
        self.keys[0] = 0;
        self.mouseDraggingDot = -1;
    }
    if (turtleKeyPressed(GLFW_KEY_LEFT_CONTROL)) {
        if (self.keys[4] == 0) {
            self.keys[4] = 1;
        }
    } else {
        self.keys[4] = 0;
    }
    if (turtleKeyPressed(GLFW_KEY_S)) {
        if (self.keys[5] == 0) {
            self.keys[5] = 1;
            if (self.keys[4]) {
                /* save */
                if (strcmp(osToolsFileDialog.selectedFilename, "null") == 0) {
                    if (osToolsFileDialogPrompt(1, "") != -1) {
                        printf("Saved to: %s\n", osToolsFileDialog.selectedFilename);
                        generateAnimation(osToolsFileDialog.selectedFilename);
                    }
                } else {
                    printf("Saved to: %s\n", osToolsFileDialog.selectedFilename);
                    generateAnimation(osToolsFileDialog.selectedFilename);
                }
            }
        }
    } else {
        self.keys[5] = 0;
    }
}

void parseRibbonOutput() {
    if (ribbonRender.output[0] == 1) {
        ribbonRender.output[0] = 0;
        if (ribbonRender.output[1] == 0) { // File
            if (ribbonRender.output[2] == 1) { // New
                printf("New\n");
                strcpy(osToolsFileDialog.selectedFilename, "null");
                self.animationSaveIndex++;
            }
            if (ribbonRender.output[2] == 2) { // Save
                if (strcmp(osToolsFileDialog.selectedFilename, "null") == 0) {
                    if (osToolsFileDialogPrompt(1, "") != -1) {
                        printf("Saved to: %s\n", osToolsFileDialog.selectedFilename);
                        generateAnimation(osToolsFileDialog.selectedFilename);
                    }
                } else {
                    printf("Saved to: %s\n", osToolsFileDialog.selectedFilename);
                    generateAnimation(osToolsFileDialog.selectedFilename);
                }
            }
            if (ribbonRender.output[2] == 3) { // Save As...
                if (osToolsFileDialogPrompt(1, "") != -1) {
                    printf("Saved to: %s\n", osToolsFileDialog.selectedFilename);
                    generateAnimation(osToolsFileDialog.selectedFilename);
                }
            }
            if (ribbonRender.output[2] == 4) { // Open
                if (osToolsFileDialogPrompt(0, "") != -1) {
                    printf("Loaded data from: %s\n", osToolsFileDialog.selectedFilename);
                }
            }
        }
        if (ribbonRender.output[1] == 1) { // Edit
            if (ribbonRender.output[2] == 1) { // Undo
                printf("Undo\n");
            }
            if (ribbonRender.output[2] == 2) { // Redo
                printf("Redo\n");
            }
            if (ribbonRender.output[2] == 3) { // Cut
                osToolsClipboardSetText("test123");
                printf("Cut \"test123\" to clipboard!\n");
            }
            if (ribbonRender.output[2] == 4) { // Copy
                osToolsClipboardSetText("test345");
                printf("Copied \"test345\" to clipboard!\n");
            }
            if (ribbonRender.output[2] == 5) { // Paste
                osToolsClipboardGetText();
                printf("Pasted \"%s\" from clipboard!\n", osToolsClipboard.text);
            }
        }
        if (ribbonRender.output[1] == 2) { // View
            if (ribbonRender.output[2] == 1) { // Change theme
                printf("Change theme\n");
                if (tt_theme == TT_THEME_DARK) {
                    turtleBgColor(36, 30, 32);
                    turtleToolsSetTheme(TT_THEME_COLT);
                } else if (tt_theme == TT_THEME_COLT) {
                    turtleBgColor(212, 201, 190);
                    turtleToolsSetTheme(TT_THEME_NAVY);
                } else if (tt_theme == TT_THEME_NAVY) {
                    turtleBgColor(255, 255, 255);
                    turtleToolsSetTheme(TT_THEME_LIGHT);
                } else if (tt_theme == TT_THEME_LIGHT) {
                    turtleBgColor(30, 30, 30);
                    turtleToolsSetTheme(TT_THEME_DARK);
                }
            } 
            if (ribbonRender.output[2] == 2) { // GLFW
                printf("GLFW settings\n");
            } 
        }
    }
}

void parsePopupOutput(GLFWwindow *window) {
    if (popup.output[0] == 1) {
        popup.output[0] = 0; // untoggle
        if (popup.output[1] == 0) { // cancel
            turtle.close = 0;
            glfwSetWindowShouldClose(window, 0);
        }
        if (popup.output[1] == 1) { // close
            turtle.shouldClose = 1;
        }
    }
}

int main(int argc, char *argv[]) {
    /* Initialize glfw */
    if (!glfwInit()) {
        return -1;
    }
    glfwWindowHint(GLFW_SAMPLES, 4); // MSAA (Anti-Aliasing) with 4 samples (must be done before window is created (?))

    /* Create a windowed mode window and its OpenGL context */
    const GLFWvidmode *monitorSize = glfwGetVideoMode(glfwGetPrimaryMonitor());
    int32_t windowHeight = monitorSize -> height * 0.85;
    GLFWwindow *window = glfwCreateWindow(windowHeight * 16 / 9, windowHeight, "stickAnimator", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetWindowSizeLimits(window, windowHeight * 16 / 9, windowHeight, windowHeight * 16 / 9, windowHeight);

    /* initialize turtle */
    turtleInit(window, -320, -180, 320, 180);
    /* initialise turtleText */
    turtleTextInit("include/roberto.tgl");
    /* initialise turtleTools ribbon */
    turtleBgColor(255, 255, 255);
    turtleToolsSetTheme(TT_THEME_LIGHT); // light theme preset
    ribbonInit("include/ribbonConfig.txt");
    /* initialise turtleTools popup */
    popupInit("include/popupConfig.txt", -60, -20, 60, 20);
    /* initialise osTools */
    osToolsInit(argv[0], window); // must include argv[0] to get executableFilepath, must include GLFW window
    osToolsFileDialogAddExtension("sta"); // add sta to extension restrictions

    init();

    uint32_t tps = 120; // ticks per second (locked to fps in this case)
    uint64_t tick = 0; // count number of ticks since application started
    clock_t start, end;

    while (turtle.shouldClose == 0) {
        start = clock();
        turtleGetMouseCoords();
        turtleClear();
        tt_setColor(TT_COLOR_TEXT);
        turtleTextWriteStringf(-310, -170, 5, 0, "%.2lf, %.2lf", turtle.mouseX, turtle.mouseY);
        renderGround();
        // renderStick(self.sticks -> data[0].r);
        if (self.mode) {
            renderDots(0);
            renderFrames();
        }
        handleUI();
        mouseTick();
        turtleToolsUpdate(); // update turtleTools
        parseRibbonOutput(); // user defined function to use ribbon
        parsePopupOutput(window); // user defined function to use popup
        turtleUpdate(); // update the screen
        end = clock();
        while ((double) (end - start) / CLOCKS_PER_SEC < (1.0 / tps)) {
            end = clock();
        }
        tick++;
    }
    turtleFree();
    glfwTerminate();
    return 0;
}