#include "flash.h"
#include <string.h>

// 外部变量声明
extern unsigned short white[8];  // 白色校准值数组
extern unsigned short black[8];  // 黑色校准值数组
/**
 * @brief 将校准数据写入Flash
 * @note 写入过程会禁用中断确保操作原子性
 *       先擦除对应扇区，再分两次写入8字节数据(每次64位)
 */
void writeToFlash(void)
{
    __disable_irq();  // 禁用中断确保操作原子性
    DL_FlashCTL_unprotectSector(FLASHCTL, FLASH_WHITE_DATA_ADDR, DL_FLASHCTL_REGION_SELECT_MAIN);
    DL_FlashCTL_eraseMemoryFromRAM(FLASHCTL, FLASH_WHITE_DATA_ADDR, DL_FLASHCTL_COMMAND_SIZE_SECTOR);
    /* 白色校准数据写入 */
    
    // 分两次写入白色数据(每次64位)
    DL_FlashCTL_unprotectSector(FLASHCTL, FLASH_WHITE_DATA_ADDR, DL_FLASHCTL_REGION_SELECT_MAIN);
    DL_FlashCTL_programMemoryFromRAM64(FLASHCTL, FLASH_WHITE_DATA_ADDR, (uint32_t*)&white[0]);
    DL_FlashCTL_waitForCmdDone(FLASHCTL);
    
    DL_FlashCTL_unprotectSector(FLASHCTL, FLASH_WHITE_DATA_ADDR, DL_FLASHCTL_REGION_SELECT_MAIN);
    DL_FlashCTL_programMemoryFromRAM64(FLASHCTL, FLASH_WHITE_DATA_ADDR + 8, (uint32_t*)&white[4]);
    DL_FlashCTL_waitForCmdDone(FLASHCTL);
    
    /* 黑色校准数据写入 */
    
    // 分两次写入黑色数据(每次64位)
    DL_FlashCTL_unprotectSector(FLASHCTL, FLASH_BLACK_DATA_ADDR, DL_FLASHCTL_REGION_SELECT_MAIN);
    DL_FlashCTL_programMemoryFromRAM64(FLASHCTL, FLASH_BLACK_DATA_ADDR, (uint32_t*)&black[0]);
    DL_FlashCTL_waitForCmdDone(FLASHCTL);
    
    DL_FlashCTL_unprotectSector(FLASHCTL, FLASH_BLACK_DATA_ADDR, DL_FLASHCTL_REGION_SELECT_MAIN);
    DL_FlashCTL_programMemoryFromRAM64(FLASHCTL, FLASH_BLACK_DATA_ADDR + 8, (uint32_t*)&black[4]);
    DL_FlashCTL_waitForCmdDone(FLASHCTL);
    
    __enable_irq();  // 重新启用中断
}

/**
 * @brief 从Flash读取白色校准数据
 * @note 读取过程会禁用中断确保数据一致性
 */
void readWhiteFromFlash(void)
{
    __disable_irq();  // 禁用中断
    
    // 直接从Flash地址拷贝数据到白色校准数组
    memcpy(&white[0], (void*)FLASH_WHITE_DATA_ADDR, 8);    // 前4个元素(8字节)
    memcpy(&white[4], (void*)(FLASH_WHITE_DATA_ADDR+8), 8); // 后4个元素(8字节)
    
    __enable_irq();  // 重新启用中断
}

/**
 * @brief 从Flash读取黑色校准数据
 * @note 读取过程会禁用中断确保数据一致性
 */
void readBlackFromFlash(void)
{
    __disable_irq();  // 禁用中断
    
    // 直接从Flash地址拷贝数据到黑色校准数组
    memcpy(&black[0], (void*)FLASH_BLACK_DATA_ADDR, 8);    // 前4个元素(8字节)
    memcpy(&black[4], (void*)(FLASH_BLACK_DATA_ADDR+8), 8); // 后4个元素(8字节)
    
    __enable_irq();  // 重新启用中断
}