#include "Application/RobotPlatform.h"
#include "Application/StatusSignal.h"

#include "Hardware/StarFlash.h"
#include "Hardware/Buzzer.h"
#include "Hardware/CONTROL/GrayTrack.h"
#include "Hardware/CONTROL/HeadingControl.h"
#include "Hardware/CONTROL/SpeedPI.h"
#include "Hardware/Encoder.h"
#include "Hardware/IMU.h"
#include "Hardware/LaserRelay.h"
#include "Hardware/Motor.h"
#include "Hardware/TrackZone.h"
#include "Public/Board/board.h"

void RobotPlatform_Init(void)
{
    board_init();
    Buzzer_Init();
    LaserRelay_Init();
    TrackZone_Init();
    StatusSignal_Init();
    StarFlash_Init();
    Motor_Init();
    Encoder_Init();
    GrayTrack_Init();
    SpeedPI_Init();
    HeadingControl_Init();
    IMU_Init();
    Motor_Coast();
}
