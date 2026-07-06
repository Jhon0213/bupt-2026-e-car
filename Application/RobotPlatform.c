#include "Application/RobotPlatform.h"

#include "Hardware/Bluetooth.h"
#include "Hardware/CONTROL/GrayTrack.h"
#include "Hardware/CONTROL/HeadingControl.h"
#include "Hardware/CONTROL/SpeedPI.h"
#include "Hardware/Encoder.h"
#include "Hardware/IMU.h"
#include "Hardware/Motor.h"
#include "Public/Board/board.h"

void RobotPlatform_Init(void)
{
    board_init();
    Bluetooth_Init();
    Motor_Init();
    Encoder_Init();
    GrayTrack_Init();
    SpeedPI_Init();
    HeadingControl_Init();
    IMU_Init();
    (void)IMU_SetHostBaudRate(115200U);
    Motor_Coast();
}
