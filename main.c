/*
 */

#include <project_cc.h>
#include "ti_drivers_config.h"
#include "ti_board_open_close.h"
#include "ti_drivers_open_close.h"

int main(void)
{
    System_init();
    Board_init();
    //    int32_t status = SystemP_SUCCESS;
    Drivers_open();
    // status = Board_driversOpen();
    // DebugP_assert(status == SystemP_SUCCESS);
    appMain(NULL);
    Board_deinit();
    System_deinit();
    return 0;
}
