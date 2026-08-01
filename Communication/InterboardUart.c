#include "Communication/InterboardUart.h"

#include "Application/BuildConfig.h"
#include "ti_msp_dl_config.h"

#if INTERBOARD_LINK_ENABLE

typedef struct
{
    uint8_t active_frame[INTERBOARD_FRAME_SIZE];
    uint8_t pending_frame[INTERBOARD_FRAME_SIZE];
    volatile uint8_t active_index;
    volatile uint8_t active_valid;
    volatile uint8_t pending_valid;
    volatile uint32_t tx_frame_count;
    volatile uint32_t overwrite_count;
} InterboardTxState;

static InterboardTxState g_interboard_tx;

static uint32_t InterboardUart_EnterCritical(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

static void InterboardUart_ExitCritical(uint32_t primask)
{
    if (primask == 0U)
    {
        __enable_irq();
    }
}

static void InterboardUart_CopyFrame(uint8_t *destination,
                                     const uint8_t *source)
{
    uint8_t i;

    for (i = 0U; i < INTERBOARD_FRAME_SIZE; i++)
    {
        destination[i] = source[i];
    }
}

static void InterboardUart_EnableTxInterrupt(void)
{
    DL_UART_Main_enableInterrupt(UART_2_INST, DL_UART_MAIN_INTERRUPT_TX);
    NVIC_ClearPendingIRQ(UART_2_INST_INT_IRQN);
    NVIC_EnableIRQ(UART_2_INST_INT_IRQN);
}

static void InterboardUart_DisableTxInterrupt(void)
{
    DL_UART_Main_disableInterrupt(UART_2_INST, DL_UART_MAIN_INTERRUPT_TX);
}

static void InterboardUart_KickTx(void)
{
    uint32_t primask = InterboardUart_EnterCritical();

    if ((g_interboard_tx.active_valid != 0U) &&
        (g_interboard_tx.active_index < INTERBOARD_FRAME_SIZE))
    {
        uint8_t index = g_interboard_tx.active_index;

        if (DL_UART_Main_transmitDataCheck(UART_2_INST,
                                           g_interboard_tx.active_frame[index]))
        {
            g_interboard_tx.active_index = (uint8_t)(index + 1U);
        }
    }
    InterboardUart_EnableTxInterrupt();
    InterboardUart_ExitCritical(primask);
}

static void InterboardUart_LoadPendingToActive(void)
{
    InterboardUart_CopyFrame(g_interboard_tx.active_frame,
                             g_interboard_tx.pending_frame);
    g_interboard_tx.active_index = 0U;
    g_interboard_tx.active_valid = 1U;
    g_interboard_tx.pending_valid = 0U;
}

static void InterboardUart_TxService(void)
{
    while ((g_interboard_tx.active_valid != 0U) &&
           (g_interboard_tx.active_index < INTERBOARD_FRAME_SIZE))
    {
        uint8_t index = g_interboard_tx.active_index;

        if (!DL_UART_Main_transmitDataCheck(UART_2_INST,
                                            g_interboard_tx.active_frame[index]))
        {
            return;
        }
        g_interboard_tx.active_index = (uint8_t)(index + 1U);
    }

    if ((g_interboard_tx.active_valid != 0U) &&
        (g_interboard_tx.active_index >= INTERBOARD_FRAME_SIZE))
    {
        g_interboard_tx.tx_frame_count++;
        if (g_interboard_tx.pending_valid != 0U)
        {
            InterboardUart_LoadPendingToActive();
            InterboardUart_TxService();
        }
        else
        {
            g_interboard_tx.active_valid = 0U;
            g_interboard_tx.active_index = 0U;
            InterboardUart_DisableTxInterrupt();
        }
    }
}

void InterboardUart_Init(void)
{
    uint32_t primask;

    primask = InterboardUart_EnterCritical();
    g_interboard_tx.active_index = 0U;
    g_interboard_tx.active_valid = 0U;
    g_interboard_tx.pending_valid = 0U;
    g_interboard_tx.tx_frame_count = 0U;
    g_interboard_tx.overwrite_count = 0U;
    InterboardUart_DisableTxInterrupt();
    DL_UART_Main_disableInterrupt(UART_2_INST, DL_UART_MAIN_INTERRUPT_RX);
    DL_UART_Main_clearInterruptStatus(UART_2_INST,
                                      DL_UART_MAIN_INTERRUPT_RX |
                                      DL_UART_MAIN_INTERRUPT_TX);
    NVIC_ClearPendingIRQ(UART_2_INST_INT_IRQN);
    NVIC_EnableIRQ(UART_2_INST_INT_IRQN);
    InterboardUart_ExitCritical(primask);
}

void InterboardUart_SubmitLatest(const uint8_t frame[INTERBOARD_FRAME_SIZE])
{
    uint32_t primask;
    uint8_t start_tx = 0U;

    if (frame == 0)
    {
        return;
    }

    primask = InterboardUart_EnterCritical();
    if (g_interboard_tx.active_valid == 0U)
    {
        InterboardUart_CopyFrame(g_interboard_tx.active_frame, frame);
        g_interboard_tx.active_index = 0U;
        g_interboard_tx.active_valid = 1U;
        start_tx = 1U;
    }
    else
    {
        if (g_interboard_tx.pending_valid != 0U)
        {
            g_interboard_tx.overwrite_count++;
        }
        InterboardUart_CopyFrame(g_interboard_tx.pending_frame, frame);
        g_interboard_tx.pending_valid = 1U;
    }
    InterboardUart_ExitCritical(primask);

    if (start_tx != 0U)
    {
        InterboardUart_KickTx();
    }
}

uint32_t InterboardUart_GetTxFrameCount(void)
{
    return g_interboard_tx.tx_frame_count;
}

uint32_t InterboardUart_GetOverwriteCount(void)
{
    return g_interboard_tx.overwrite_count;
}

uint8_t InterboardUart_IsBusy(void)
{
    return ((g_interboard_tx.active_valid != 0U) ||
            (g_interboard_tx.pending_valid != 0U)) ? 1U : 0U;
}

void UART_2_INST_IRQHandler(void)
{
    switch (DL_UART_getPendingInterrupt(UART_2_INST))
    {
        case DL_UART_IIDX_TX:
            InterboardUart_TxService();
            break;

        default:
            break;
    }
}

#else

void InterboardUart_Init(void)
{
}

void InterboardUart_SubmitLatest(const uint8_t frame[INTERBOARD_FRAME_SIZE])
{
    (void)frame;
}

uint32_t InterboardUart_GetTxFrameCount(void)
{
    return 0U;
}

uint32_t InterboardUart_GetOverwriteCount(void)
{
    return 0U;
}

uint8_t InterboardUart_IsBusy(void)
{
    return 0U;
}

#endif