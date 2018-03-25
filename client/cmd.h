#ifndef CMD_H
#define CMD_H

#define CMD_SIZE 5

static const char CMD_ROLL_STOP[]     = {0xff,0,0,0,0xff};
static const char CMD_ROLL_FORWARD[]  = {0xff,0,1,0,0xff};
static const char CMD_ROLL_BACKWARD[] = {0xff,0,2,0,0xff};
static const char CMD_ROLL_LEFT[]     = {0xff,0,3,0,0xff};
static const char CMD_ROLL_RIGHT[]    = {0xff,0,4,0,0xff};

#define CAMERA_STEP 3
static char CMD_CAMERA_V[]     = {0xff,1,8,0,0xff};
static char CMD_CAMERA_H[]   = {0xff,1,7,90,0xff};

#endif // CMD_H
