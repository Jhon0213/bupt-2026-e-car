#include "ti_msp_dl_config.h"

#include "Public/Board/board.h"

int main(void)
{
    board_init();

    while (1)
    {
        delay_ms(1000);
    }
}
