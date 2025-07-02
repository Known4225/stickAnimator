#include "include/turtleTools.h"
#include "include/osTools.h"
#include <time.h>

/*
TODO:
- sync currentAnimation and currentFrame names so that it makes more sense
- face the music and admit that you cannot support multiple sticks
- delete frames
- delete animations
- reorder frames?
- rotate entire stick?
- fix bug with loading causing an extra frame (or erasing the last frame?)
- binding keys to animations?
*/

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
        [filepath, name, number of frames, startingX, startingY, reserved, reserved, reserved,
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
    int32_t mouseHoverFrame;
    int32_t mouseHoverAnimation;

    int8_t mode; // 0 - normal mode, 1 - editing mode
    tt_switch_t *modeSwitch;
    double onionNumber;
    tt_slider_t *onionSlider;
    int8_t frameButtonPressed;
    tt_button_t *frameButton;
    int32_t currentFrame; // number of frame being edited
    double frameBarX;
    double frameBarY;
    double frameScroll;
    tt_scrollbar_t *frameScrollbar;

    /* animation */
    int32_t animationSaveIndex;
    double animationBarX;
    double animationBarY;
    double animationScroll;
    tt_scrollbar_t *animationScrollbar;

    /* playing animation */
    int32_t playingAnimationFrame;
    clock_t timeOfLastFrame;
    int8_t playButtonPressed;
    int8_t play;
    tt_button_t *playButton;
    int8_t loop;
    tt_switch_t *loopSwitch;
    double framesPerSecond;
    tt_slider_t *framesPerSecondSlider;
} stickAnimator_t;

stickAnimator_t self;

void createStick(list_t *stick);
void insertFrame(int32_t stickIndex, int32_t frameIndex);
void generateAnimation(char *filename, int32_t animationIndex);

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
    insertFrame(0, 0);
    self.mouseHoverDot = -1;
    self.mouseDraggingDot = -1;
    self.mouseAnchorX = 0;
    self.mouseAnchorY = 0;
    self.mouseHoverFrame = -1;
    self.mouseHoverAnimation = -1;

    /* UI elements */
    self.mode = 1;
    self.modeSwitch = switchInit("Mode", &self.mode, -305, 152, 6);
    self.onionNumber = 0;
    self.onionSlider = sliderInit("Onion", &self.onionNumber, TT_SLIDER_HORIZONTAL, TT_SLIDER_ALIGN_CENTER, -290, 113, 6, 40, 0, 3, 1);
    self.frameButtonPressed = 0;
    self.frameButton = buttonInit("Add Frame", &self.frameButtonPressed, -290, 135, 6);
    self.currentFrame = 0;
    self.frameBarX = -180;
    self.frameBarY = 160;
    self.frameScroll = 0;
    self.frameScrollbar = scrollbarInit(&self.frameScroll, TT_SCROLLBAR_HORIZONTAL, (self.frameBarX + 315) / 2, self.frameBarY - 41, 6, 492, 90);
    self.playButtonPressed = 0;
    self.play = 0;
    self.playButton = buttonInit("Play", &self.playButtonPressed, -280, 152, 6);
    self.loop = 1;
    self.loopSwitch = switchInit("Loop", &self.loop, -260, 152, 6);
    self.loopSwitch -> style = TT_SWITCH_STYLE_CHECKBOX;
    self.framesPerSecond = 12;
    self.framesPerSecondSlider = sliderInit("Frames/s", &self.framesPerSecond, TT_SLIDER_HORIZONTAL, TT_SLIDER_ALIGN_CENTER, -210, 152, 6, 40, 1, 30, 1);

    /* animations */
    self.animations = list_init();
    self.animationSaveIndex = 0;
    generateAnimation("null", self.animationSaveIndex);
    self.animationBarX = 250;
    self.animationBarY = 113;
    self.animationScroll = 0;
    self.animationScrollbar = scrollbarInit(&self.animationScroll, TT_SCROLLBAR_VERTICAL, (self.animationBarX) - 5, (self.animationBarY - 175) / 2, 6, 286, 90);
}

/* create stick in default position */
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

/* insert frame to currentAnimation (from stick data) */
void insertFrame(int32_t stickIndex, int32_t frameIndex) {
    list_t *stick = self.sticks -> data[stickIndex].r;
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
    list_append(constructedList, stick -> data[STICK_RIGHT_UPPER_LEG], 'd');
    list_append(constructedList, stick -> data[STICK_RIGHT_LOWER_LEG], 'd');
    list_insert(self.currentAnimation, frameIndex, (unitype) constructedList, 'r');
}

/* update currentAnimation with data from stick */
void updateCurrentFrame(int32_t stickIndex, int32_t frameIndex) {
    list_t *stick = self.sticks -> data[stickIndex].r;
    self.currentAnimation -> data[frameIndex].r -> data[0] = stick -> data[STICK_X];
    self.currentAnimation -> data[frameIndex].r -> data[1] = stick -> data[STICK_Y];
    self.currentAnimation -> data[frameIndex].r -> data[2] = stick -> data[STICK_LOWER_BODY];
    self.currentAnimation -> data[frameIndex].r -> data[3] = stick -> data[STICK_UPPER_BODY];
    self.currentAnimation -> data[frameIndex].r -> data[4] = stick -> data[STICK_HEAD];
    self.currentAnimation -> data[frameIndex].r -> data[5] = stick -> data[STICK_LEFT_UPPER_ARM];
    self.currentAnimation -> data[frameIndex].r -> data[6] = stick -> data[STICK_LEFT_LOWER_ARM];
    self.currentAnimation -> data[frameIndex].r -> data[7] = stick -> data[STICK_RIGHT_UPPER_ARM];
    self.currentAnimation -> data[frameIndex].r -> data[8] = stick -> data[STICK_RIGHT_LOWER_ARM];
    self.currentAnimation -> data[frameIndex].r -> data[9] = stick -> data[STICK_LEFT_UPPER_LEG];
    self.currentAnimation -> data[frameIndex].r -> data[10] = stick -> data[STICK_LEFT_LOWER_LEG];
    self.currentAnimation -> data[frameIndex].r -> data[11] = stick -> data[STICK_RIGHT_UPPER_LEG];
    self.currentAnimation -> data[frameIndex].r -> data[12] = stick -> data[STICK_RIGHT_LOWER_LEG];
}

/* update stick with data from the current frame */
void loadCurrentFrame(int32_t stickIndex) {
    list_t *stick = self.sticks -> data[stickIndex].r;
    stick -> data[STICK_X] = self.currentAnimation -> data[self.currentFrame].r -> data[0];
    stick -> data[STICK_Y] = self.currentAnimation -> data[self.currentFrame].r -> data[1];
    stick -> data[STICK_LOWER_BODY] = self.currentAnimation -> data[self.currentFrame].r -> data[2];
    stick -> data[STICK_UPPER_BODY] = self.currentAnimation -> data[self.currentFrame].r -> data[3];
    stick -> data[STICK_HEAD] = self.currentAnimation -> data[self.currentFrame].r -> data[4];
    stick -> data[STICK_LEFT_UPPER_ARM] = self.currentAnimation -> data[self.currentFrame].r -> data[5];
    stick -> data[STICK_LEFT_LOWER_ARM] = self.currentAnimation -> data[self.currentFrame].r -> data[6];
    stick -> data[STICK_RIGHT_UPPER_ARM] = self.currentAnimation -> data[self.currentFrame].r -> data[7];
    stick -> data[STICK_RIGHT_LOWER_ARM] = self.currentAnimation -> data[self.currentFrame].r -> data[8];
    stick -> data[STICK_LEFT_UPPER_LEG] = self.currentAnimation -> data[self.currentFrame].r -> data[9];
    stick -> data[STICK_LEFT_LOWER_LEG] = self.currentAnimation -> data[self.currentFrame].r -> data[10];
    stick -> data[STICK_RIGHT_UPPER_LEG] = self.currentAnimation -> data[self.currentFrame].r -> data[11];
    stick -> data[STICK_RIGHT_LOWER_LEG] = self.currentAnimation -> data[self.currentFrame].r -> data[12];
}

/* update currentAnimation with data from animations */
void loadCurrentAnimation(int32_t animationIndex) {
    list_clear(self.currentAnimation);
    double xpos = self.animations -> data[animationIndex].r -> data[3].d;
    double ypos = self.animations -> data[animationIndex].r -> data[4].d;
    self.framesPerSecond = self.animations -> data[animationIndex].r -> data[5].i;
    for (int32_t i = 0; i < self.animations -> data[animationIndex].r -> length - 8; i++) {
        list_t *constructedList = list_init();
        xpos += self.animations -> data[animationIndex].r -> data[8 + i].r -> data[0].d;
        ypos += self.animations -> data[animationIndex].r -> data[8 + i].r -> data[1].d;
        list_append(constructedList, (unitype) xpos, 'd');
        list_append(constructedList, (unitype) ypos, 'd');
        list_append(constructedList, self.animations -> data[animationIndex].r -> data[8 + i].r -> data[2], 'd');
        list_append(constructedList, self.animations -> data[animationIndex].r -> data[8 + i].r -> data[3], 'd');
        list_append(constructedList, self.animations -> data[animationIndex].r -> data[8 + i].r -> data[4], 'd');
        list_append(constructedList, self.animations -> data[animationIndex].r -> data[8 + i].r -> data[5], 'd');
        list_append(constructedList, self.animations -> data[animationIndex].r -> data[8 + i].r -> data[6], 'd');
        list_append(constructedList, self.animations -> data[animationIndex].r -> data[8 + i].r -> data[7], 'd');
        list_append(constructedList, self.animations -> data[animationIndex].r -> data[8 + i].r -> data[8], 'd');
        list_append(constructedList, self.animations -> data[animationIndex].r -> data[8 + i].r -> data[9], 'd');
        list_append(constructedList, self.animations -> data[animationIndex].r -> data[8 + i].r -> data[10], 'd');
        list_append(constructedList, self.animations -> data[animationIndex].r -> data[8 + i].r -> data[11], 'd');
        list_append(constructedList, self.animations -> data[animationIndex].r -> data[8 + i].r -> data[12], 'd');
        list_append(self.currentAnimation, (unitype) constructedList, 'r');
    }
}

/* update animations with currentAnimation */
void generateAnimation(char *filename, int32_t animationIndex) {
    list_t *newAnimation = list_init();
    list_append(newAnimation, (unitype) filename, 's'); // filepath
    int32_t nameIndex = strlen(filename) - 1;
    int32_t dotIndex = -1;
    while (nameIndex >= 0) {
        if (filename[nameIndex] == '.') {
            dotIndex = nameIndex;
            filename[nameIndex] = '\0';
        }
        if (filename[nameIndex] == '\\' || filename[nameIndex] == '/') {
            break;
        }
        nameIndex--;
    }
    list_append(newAnimation, (unitype) (filename + nameIndex + 1), 's'); // name
    if (dotIndex != -1) {
        filename[dotIndex] = '.';
    }
    list_append(newAnimation, (unitype) self.currentAnimation -> length, 'i'); // number of frames
    if (self.currentAnimation -> length > 0) {
        list_append(newAnimation, self.currentAnimation -> data[0].r -> data[0], 'd'); // startingX
        list_append(newAnimation, self.currentAnimation -> data[0].r -> data[1], 'd'); // startingY
    } else {
        list_append(newAnimation, (unitype) 0, 'd');
        list_append(newAnimation, (unitype) 0, 'd');
    }
    list_append(newAnimation, (unitype) (int32_t) round(self.framesPerSecond), 'i');
    for (int32_t i = 0; i < 2; i++) {
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
    if (animationIndex < self.animations -> length) {
        /* update existing animation */
        list_delete(self.animations, animationIndex);
        list_insert(self.animations, animationIndex, (unitype) newAnimation, 'r');
    } else {
        /* create animation */
        list_append(self.animations, (unitype) newAnimation, 'r');
    }
    if (strcmp(filename, "null") != 0) {
        FILE *fp = fopen(filename, "w");
        list_fprint_emb(fp, self.animations -> data[animationIndex].r);
        fclose(fp);
    }
}

/* import animation from file */
int32_t importAnimation(char *filename) {
    uint32_t fileSize;
    char *fileData = (char *) osToolsMapFile(filename, &fileSize);
    if (fileData == NULL) {
        return -1;
    }
    list_t *outputList = list_init();
    int32_t left = 1;
    int32_t right = 1;
    int32_t extractionIndex = 0;
    int8_t inList = 0;
    while (right < fileSize) {
        if (fileData[right] == '[') {
            list_append(outputList, (unitype) list_init(), 'r');
            left = right + 1;
            inList = 1;
        }
        if (fileData[right] == ']' && right < fileSize) {
            fileData[right] = '\0';
            /* double */
            double readDouble;
            sscanf(fileData + left, "%lf", &readDouble);
            if (inList) {
                list_append(outputList -> data[outputList -> length - 1].r, (unitype) readDouble, 'd');
            } else {
                list_append(outputList, (unitype) readDouble, 'd');
            }
            fileData[right] = ']';
            right += 2;
            left = right + 1;
            extractionIndex++;
            inList = 0;
        }
        if (fileData[right] == ',') {
            fileData[right] = '\0';
            if (extractionIndex == 0) {
                /* update filepath */
                list_append(outputList, (unitype) osToolsFileDialog.selectedFilename, 's');
            } else if (extractionIndex == 1) {
                /* string */
                list_append(outputList, (unitype) (fileData + left), 's');
            } else if (extractionIndex == 2 || extractionIndex == 5) {
                /* int */
                int32_t readInt;
                sscanf(fileData + left, "%d", &readInt);
                list_append(outputList, (unitype) readInt, 'i');
            } else {
                /* double */
                double readDouble;
                sscanf(fileData + left, "%lf", &readDouble);
                if (inList) {
                    list_append(outputList -> data[outputList -> length - 1].r, (unitype) readDouble, 'd');
                } else {
                    list_append(outputList, (unitype) readDouble, 'd');
                }
            }
            fileData[right] = ',';
            right++;
            left = right + 1;
            extractionIndex++;
        }
        right++;
    }
    osToolsUnmapFile((uint8_t *) fileData);
    list_append(self.animations, (unitype) outputList, 'r');
    return 0;
}

/* show, hide, and process UI elements */
void handleUI() {
    turtleRectangleColor(-320, 180, self.frameBarX - 1, 100, turtle.bgr, turtle.bgg, turtle.bgb, 0);
    if (self.mode) {
        self.onionSlider -> enabled = TT_ELEMENT_ENABLED;
        self.frameButton -> enabled = TT_ELEMENT_ENABLED;
        if (self.currentAnimation -> length >= 8) {
            self.frameScrollbar -> enabled = TT_ELEMENT_ENABLED;
            self.frameScrollbar -> barPercentage = 100 / (self.currentAnimation -> length / 7.8);
        } else {
            self.frameScrollbar -> enabled = TT_ELEMENT_HIDE;
        }
        if (self.frameButtonPressed) {
            insertFrame(0, self.currentFrame);
            self.currentFrame++;
            if (self.currentFrame < self.currentAnimation -> length) {
                loadCurrentFrame(0);
            }
        }
    } else {
        self.onionSlider -> enabled = TT_ELEMENT_HIDE;
        self.frameButton -> enabled = TT_ELEMENT_HIDE;
        self.frameScrollbar -> enabled = TT_ELEMENT_HIDE;
    }
    if (self.animations -> length >= 8) {
        self.animationScrollbar -> enabled = TT_ELEMENT_ENABLED;
        self.frameScrollbar -> barPercentage = 100 / (self.currentAnimation -> length / 6.8);
    } else {
        self.animationScrollbar -> enabled = TT_ELEMENT_HIDE;
    }
    if (self.playButtonPressed) {
        self.play = !self.play;
        self.currentFrame = 0;
        self.timeOfLastFrame = clock();
    }
    if (self.play) {
        strcpy(self.playButton -> label, "Stop");
        clock_t timeNow = clock();
        if ((double) (timeNow - self.timeOfLastFrame) / CLOCKS_PER_SEC >= (1.0 / self.framesPerSecond)) {
            self.timeOfLastFrame = timeNow;
            self.currentFrame++;
            if (self.currentFrame == self.currentAnimation -> length) {
                if (self.loop) {
                    self.currentFrame = 0;
                } else {
                    self.play = 0;
                    self.currentFrame = self.currentAnimation -> length - 1;
                }
            }
            loadCurrentFrame(0);
            /* extra catch to end animation early */
            if (self.currentFrame == self.currentAnimation -> length - 1 && self.loop == 0) {
                self.play = 0;
            }
        }
    } else {
        strcpy(self.playButton -> label, "Play");
    }
}

/* render the ground */
void renderGround() {
    turtlePenShape("square");
    turtlePenColor(0, 0, 0);
    turtlePenSize(4);
    turtleGoto(-320, -86);
    turtlePenDown();
    turtleGoto(240, turtle.y);
    turtlePenUp();
    turtlePenShape("circle");
}

/* render a stick */
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
        turtlePenColor(turtle.bgr, turtle.bgg, turtle.bgb);
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

/* check if mouse hovers over a dot */
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

/* render dots on a stick */
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

/* render frame topbar */
void renderFrames() {
    self.mouseHoverFrame = -1;
    for (int32_t i = 0; i < self.currentAnimation -> length; i++) {
        double frameXLeft = self.frameBarX + i * 66 - self.frameScroll * (self.currentAnimation -> length - 7.5) * 0.66;
        double frameYUp = self.frameBarY;
        double frameXRight = frameXLeft + 64; 
        double frameYDown = frameYUp - 36;
        if (frameXRight < self.frameBarX) {
            continue;
        }
        if (frameXLeft > 320) {
            return;
        }
        /* detect mouse */
        if (turtle.mouseX >= frameXLeft && turtle.mouseX <= frameXRight && turtle.mouseY >= frameYDown && turtle.mouseY <= frameYUp) {
            self.mouseHoverFrame = i;
        }
        /* draw box */
        if (self.currentFrame == i) {
            turtlePenColor(16, 180, 190);
            turtlePenSize(1.5);
        } else if (self.mouseHoverFrame == i) {
            turtlePenColor(150, 150, 150);
            turtlePenSize(1);
        } else {
            turtlePenColor(0, 0, 0);
            turtlePenSize(1);
        }
        turtleGoto(frameXLeft, frameYUp);
        turtlePenDown();
        turtleGoto(frameXRight, frameYUp);
        turtleGoto(frameXRight, frameYDown);
        turtleGoto(frameXLeft, frameYDown);
        turtleGoto(frameXLeft, frameYUp);
        turtlePenUp();
        turtleTextWriteStringf(frameXLeft + 2, frameYUp - 5, 5, 0, "%d", i + 1);
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
        tempStick -> data[STICK_RIGHT_UPPER_LEG].d = self.currentAnimation -> data[i].r -> data[11].d;
        tempStick -> data[STICK_RIGHT_LOWER_LEG].d = self.currentAnimation -> data[i].r -> data[12].d;
        renderStick(tempStick);
        list_free(tempStick);
    }
}

/* render animation sidebar */
void renderAnimations() {
    self.mouseHoverAnimation = -1;
    for (int32_t i = 0; i < self.animations -> length; i++) {
        double animationXLeft = self.animationBarX;
        double animationYUp = self.animationBarY - i * 38 + self.animationScroll * (self.animations -> length - 7.5) * 0.38;
        double animationXRight = animationXLeft + 64;
        double animationYDown = animationYUp - 36;
        if (animationYDown > self.animationBarY) {
            continue;
        }
        if (animationYUp < -180) {
            break;
        }
        /* detect mouse */
        if (turtle.mouseX >= animationXLeft && turtle.mouseX <= animationXRight && turtle.mouseY >= animationYDown && turtle.mouseY <= animationYUp) {
            self.mouseHoverAnimation = i;
        }
        /* draw box */
        if (self.animationSaveIndex == i) {
            turtlePenColor(16, 180, 190);
            turtlePenSize(1.5);
        } else if (self.mouseHoverAnimation == i) {
            turtlePenColor(150, 150, 150);
            turtlePenSize(1);
        } else {
            turtlePenColor(0, 0, 0);
            turtlePenSize(1);
        }
        turtleGoto(animationXLeft, animationYUp);
        turtlePenDown();
        turtleGoto(animationXRight, animationYUp);
        turtleGoto(animationXRight, animationYDown);
        turtleGoto(animationXLeft, animationYDown);
        turtleGoto(animationXLeft, animationYUp);
        turtlePenUp();
        turtleTextWriteStringf(animationXLeft + 2, animationYUp - 5, 5, 0, "%s", self.animations -> data[i].r -> data[1].s);
        /* draw thumbnail */
        list_t *tempStick = list_init();
        createStick(tempStick);
        tempStick -> data[STICK_SIZE].d = 0.05;
        tempStick -> data[STICK_X].d = animationXLeft + ((self.animations -> data[i].r -> data[3].d + 330) / 660) * (animationXRight - animationXLeft);
        tempStick -> data[STICK_Y].d = animationYDown + ((self.animations -> data[i].r -> data[4].d + 190) / 380) * (animationYUp - animationYDown);
        tempStick -> data[STICK_LOWER_BODY].d = self.animations -> data[i].r -> data[8].r -> data[2].d;
        tempStick -> data[STICK_UPPER_BODY].d = self.animations -> data[i].r -> data[8].r -> data[3].d;
        tempStick -> data[STICK_HEAD].d = self.animations -> data[i].r -> data[8].r -> data[4].d;
        tempStick -> data[STICK_LEFT_UPPER_ARM].d = self.animations -> data[i].r -> data[8].r -> data[5].d;
        tempStick -> data[STICK_LEFT_LOWER_ARM].d = self.animations -> data[i].r -> data[8].r -> data[6].d;
        tempStick -> data[STICK_RIGHT_UPPER_ARM].d = self.animations -> data[i].r -> data[8].r -> data[7].d;
        tempStick -> data[STICK_RIGHT_LOWER_ARM].d = self.animations -> data[i].r -> data[8].r -> data[8].d;
        tempStick -> data[STICK_LEFT_UPPER_LEG].d = self.animations -> data[i].r -> data[8].r -> data[9].d;
        tempStick -> data[STICK_LEFT_LOWER_LEG].d = self.animations -> data[i].r -> data[8].r -> data[10].d;
        tempStick -> data[STICK_RIGHT_UPPER_LEG].d = self.animations -> data[i].r -> data[8].r -> data[11].d;
        tempStick -> data[STICK_RIGHT_LOWER_LEG].d = self.animations -> data[i].r -> data[8].r -> data[12].d;
        renderStick(tempStick);
        list_free(tempStick);
    }
    turtleRectangleColor(self.animationBarX - 10, self.animationBarY + 1, 320, 180, turtle.bgr, turtle.bgg, turtle.bgb, 0);
}

void mouseTick() {
    if (turtleMouseDown()) {
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
            if (self.mode && self.mouseHoverFrame != -1) {
                self.currentFrame = self.mouseHoverFrame;
                loadCurrentFrame(0);
            }
            if (self.mouseHoverAnimation != -1) {
                /* save this animation */
                generateAnimation(self.animations -> data[self.animationSaveIndex].r -> data[0].s, self.animationSaveIndex);
                /* load new animation */
                self.animationSaveIndex = self.mouseHoverAnimation;
                if (strcmp(self.animations -> data[self.animationSaveIndex].r -> data[0].s, "null") == 0) {
                    strcpy(osToolsFileDialog.selectedFilename, "null");
                } else {
                    strcpy(osToolsFileDialog.selectedFilename, self.animations -> data[self.animationSaveIndex].r -> data[0].s);
                }
                loadCurrentAnimation(self.animationSaveIndex);
                self.currentFrame = 0;
                loadCurrentFrame(0);
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
        if (self.mouseDraggingDot != -1) {
            self.mouseDraggingDot = -1;
            /* update current frame */
            if (self.currentFrame < self.currentAnimation -> length) {
                updateCurrentFrame(0, self.currentFrame);
            }
        }
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
                /* save this animation */
                generateAnimation(self.animations -> data[self.animationSaveIndex].r -> data[0].s, self.animationSaveIndex);
                /* attempt to save all other animations */
                for (int32_t i = 0; i < self.animations -> length; i++) {
                    if (strcmp(self.animations -> data[i].r -> data[0].s, "null") != 0 && i != self.animationSaveIndex) {
                        loadCurrentAnimation(i);
                        generateAnimation(self.animations -> data[i].r -> data[0].s, i);
                        printf("Saved to: %s\n", self.animations -> data[i].r -> data[0].s);
                    }
                }
                loadCurrentAnimation(self.animationSaveIndex);
                /* save currentAnimation to file */
                if (strcmp(osToolsFileDialog.selectedFilename, "null") == 0) {
                    if (osToolsFileDialogPrompt(1, "") != -1) {
                        generateAnimation(osToolsFileDialog.selectedFilename, self.animationSaveIndex);
                        printf("Saved to: %s\n", osToolsFileDialog.selectedFilename);
                    }
                    /* tick window to notice keys are no longer pressed */
                    glfwHideWindow(turtle.window);
                    glfwShowWindow(turtle.window);
                } else {
                    generateAnimation(osToolsFileDialog.selectedFilename, self.animationSaveIndex);
                    printf("Saved to: %s\n", osToolsFileDialog.selectedFilename);
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
                strcpy(osToolsFileDialog.selectedFilename, "null");
                self.animationSaveIndex++;
                list_clear(self.currentAnimation);
                insertFrame(0, 0);
                generateAnimation("null", self.animationSaveIndex);
            }
            if (ribbonRender.output[2] == 2) { // Save
                if (strcmp(osToolsFileDialog.selectedFilename, "null") == 0) {
                    if (osToolsFileDialogPrompt(1, "") != -1) {
                        generateAnimation(osToolsFileDialog.selectedFilename, self.animationSaveIndex);
                        printf("Saved to: %s\n", osToolsFileDialog.selectedFilename);
                    }
                } else {
                    generateAnimation(osToolsFileDialog.selectedFilename, self.animationSaveIndex);
                    printf("Saved to: %s\n", osToolsFileDialog.selectedFilename);
                }
            }
            if (ribbonRender.output[2] == 3) { // Save As...
                if (osToolsFileDialogPrompt(1, "") != -1) {
                    generateAnimation(osToolsFileDialog.selectedFilename, self.animationSaveIndex);
                    printf("Saved to: %s\n", osToolsFileDialog.selectedFilename);
                }
            }
            if (ribbonRender.output[2] == 4) { // Open
                if (osToolsFileDialogPrompt(0, "") != -1) {
                    /* save this animation */
                    generateAnimation(self.animations -> data[self.animationSaveIndex].r -> data[0].s, self.animationSaveIndex);
                    /* import animation */
                    if (importAnimation(osToolsFileDialog.selectedFilename) != -1) {
                        self.animationSaveIndex = self.animations -> length - 1;
                        loadCurrentAnimation(self.animationSaveIndex);
                        self.currentFrame = 0;
                        loadCurrentFrame(0);
                        printf("Loaded data from: %s\n", osToolsFileDialog.selectedFilename);
                    }
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

    if (argc > 1) {
        list_delete(self.animations, 0);
        if (importAnimation(argv[1]) != -1) {
            strcpy(osToolsFileDialog.selectedFilename, argv[1]);
            self.animationSaveIndex = self.animations -> length - 1;
            loadCurrentAnimation(self.animationSaveIndex);
            self.currentFrame = 0;
            loadCurrentFrame(0);
            printf("Loaded data from: %s\n", osToolsFileDialog.selectedFilename);
        }
    }

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
        renderAnimations();
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