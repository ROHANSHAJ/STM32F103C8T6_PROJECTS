/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define LCD_ADDR (0x27 << 1) // Default I2C LCD address shifted left
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;
TIM_HandleTypeDef htim1;
UART_HandleTypeDef huart1;

osThreadId defaultTaskHandle;

/* USER CODE BEGIN PV */
// Task Handles
osThreadId keypadTaskHandle;
osThreadId coinTaskHandle;
osThreadId controlTaskHandle;
osThreadId motorTaskHandle;
osThreadId ultrasonicTaskHandle;
osThreadId displayTaskHandle;
osThreadId buzzerTaskHandle;

// Queue Handles
osMessageQId coinQueueHandle;
osMessageQId keypadQueueHandle;

// Global State Variable
typedef enum {
  IDLE,
  COIN_INSERTED,
  SELECT_ITEM,
  SELECT_QUANTITY,
  CHECK_BALANCE,
  DISPENSE,
  VERIFY_DROP,
  SYS_SUCCESS,
  SYS_ERROR
} VendingState_t;

VendingState_t currentState = IDLE;

// System Data
volatile uint16_t currentBalance = 0;
volatile char selectedItem = '\0';
volatile uint8_t dropVerified = 0;

// Per-item configuration (1 unit = 10 Rs coin)
const uint16_t defaultItemCost[4] = {1, 1, 1, 1};
volatile uint16_t itemCost[4] = {1, 1, 1, 1};
volatile uint8_t itemStock[4] = {5, 5, 5, 5};
volatile uint32_t totalRevenue = 0;

// Restock mode variables
volatile uint8_t restockMode = 0;
volatile uint8_t restockStep = 0;          // 99=password, 0=select item, 1=enter qty, 2=enter price
volatile char restockItem = 0;
volatile uint8_t tempQty = 0;
volatile uint16_t tempPrice = 0;
volatile uint8_t tempDigitIndex = 0;
volatile uint16_t tempInputValue = 0;
volatile char pwdBuffer[5] = {0};          // Store 4-digit password

// Multi‑quantity purchase
volatile uint8_t requestedQty = 0;

// Display toggle for stock/price menu
volatile uint8_t displayMenuState = 0;     // 0 = prompt, 1 = stock, 2 = price
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM1_Init(void);
static void MX_USART1_UART_Init(void);
void StartDefaultTask(void const * argument);

/* USER CODE BEGIN PFP */
void StartKeypadTask(void const * argument);
void StartCoinTask(void const * argument);
void StartControlTask(void const * argument);
void StartMotorTask(void const * argument);
void StartUltrasonicTask(void const * argument);
void StartDisplayTask(void const * argument);
void StartBuzzerTask(void const * argument);

// Helper Prototypes
void Debug_Print(char *msg);
void LCD_SendCommand(uint8_t cmd);
void LCD_SendData(uint8_t data);
void LCD_String(char *str);
void LCD_Clear(void);
void LCD_SetCursor(int row, int col);
void LCD_Init(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_TIM1_Init();
  MX_USART1_UART_Init();

  /* USER CODE BEGIN 2 */
  // Initial Debug Output
  Debug_Print("\r\n=================================\r\n");
  Debug_Print("  RTOS VENDING MACHINE BOOTING   \r\n");
  Debug_Print("=================================\r\n");

  // --- BARE METAL LCD INITIALIZATION ---
  LCD_Init();

  LCD_SetCursor(0, 0);
  LCD_String("  RTOS VENDING  ");
  LCD_SetCursor(1, 0);
  LCD_String(" SYSTEM BOOTING ");

  HAL_Delay(2000);
  LCD_Clear();
  /* USER CODE END 2 */

  /* USER CODE BEGIN RTOS_QUEUES */
  osMessageQDef(coinQueue, 16, uint8_t);
  coinQueueHandle = osMessageCreate(osMessageQ(coinQueue), NULL);

  osMessageQDef(keypadQueue, 16, uint8_t);
  keypadQueueHandle = osMessageCreate(osMessageQ(keypadQueue), NULL);
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 128);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  osThreadDef(controlTask, StartControlTask, osPriorityAboveNormal, 0, 512);
  controlTaskHandle = osThreadCreate(osThread(controlTask), NULL);

  osThreadDef(keypadTask, StartKeypadTask, osPriorityNormal, 0, 256);
  keypadTaskHandle = osThreadCreate(osThread(keypadTask), NULL);

  osThreadDef(coinTask, StartCoinTask, osPriorityNormal, 0, 128);
  coinTaskHandle = osThreadCreate(osThread(coinTask), NULL);

  osThreadDef(motorTask, StartMotorTask, osPriorityNormal, 0, 128);
  motorTaskHandle = osThreadCreate(osThread(motorTask), NULL);

  osThreadDef(displayTask, StartDisplayTask, osPriorityNormal, 0, 256);
  displayTaskHandle = osThreadCreate(osThread(displayTask), NULL);

  osThreadDef(buzzerTask, StartBuzzerTask, osPriorityNormal, 0, 128);
  buzzerTaskHandle = osThreadCreate(osThread(buzzerTask), NULL);

  osThreadDef(ultrasonicTask, StartUltrasonicTask, osPriorityNormal, 0, 256);
  ultrasonicTaskHandle = osThreadCreate(osThread(ultrasonicTask), NULL);
  /* USER CODE END RTOS_THREADS */

  /* Start scheduler */
  osKernelStart();

  while (1)
  {
  }
}

/**
  * @brief System Clock Configuration
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  */
static void MX_I2C1_Init(void)
{
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM1 Initialization Function
  */
static void MX_TIM1_Init(void)
{
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 71;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 1000;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_TIM_MspPostInit(&htim1);
}

/**
  * @brief USART1 Initialization Function
  */
static void MX_USART1_UART_Init(void)
{
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3
                          |MC1_Pin|MC2_Pin|MD1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, TRIG_Pin|MB1_Pin|MB2_Pin|BUZZER_Pin
                          |MD2_Pin|MA1_Pin|MA2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : PA0 PA1 PA2 PA3 MC1_Pin MC2_Pin MD1_Pin */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3
                          |MC1_Pin|MC2_Pin|MD1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PA4 PA5 PA6 PA7 */
  GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : TRIG_Pin MB1_Pin MB2_Pin BUZZER_Pin MD2_Pin MA1_Pin MA2_Pin */
  GPIO_InitStruct.Pin = TRIG_Pin|MB1_Pin|MB2_Pin|BUZZER_Pin
                          |MD2_Pin|MA1_Pin|MA2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : ECHO_Pin */
  GPIO_InitStruct.Pin = ECHO_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(ECHO_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : IR_Pin */
  GPIO_InitStruct.Pin = IR_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(IR_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

/* USER CODE BEGIN 4 */

// --- Helper: UART Debug Print ---
void Debug_Print(char *msg) {
    HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
}

// --- Helper: Robust I2C LCD Driver ---
void LCD_SendCommand(uint8_t cmd) {
    uint8_t data_u = (cmd & 0xF0);
    uint8_t data_l = ((cmd << 4) & 0xF0);
    uint8_t data_t[4];

    data_t[0] = data_u | 0x0C;  // EN=1, RS=0
    data_t[1] = data_u | 0x08;  // EN=0, RS=0
    data_t[2] = data_l | 0x0C;  // EN=1, RS=0
    data_t[3] = data_l | 0x08;  // EN=0, RS=0

    HAL_I2C_Master_Transmit(&hi2c1, LCD_ADDR, data_t, 4, 100);
}

void LCD_SendData(uint8_t data) {
    uint8_t data_u = (data & 0xF0);
    uint8_t data_l = ((data << 4) & 0xF0);
    uint8_t data_t[4];

    data_t[0] = data_u | 0x0D;  // EN=1, RS=1
    data_t[1] = data_u | 0x09;  // EN=0, RS=1
    data_t[2] = data_l | 0x0D;  // EN=1, RS=1
    data_t[3] = data_l | 0x09;  // EN=0, RS=1

    HAL_I2C_Master_Transmit(&hi2c1, LCD_ADDR, data_t, 4, 100);
}

void LCD_String(char *str) {
    while (*str) LCD_SendData(*str++);
}

void LCD_Clear(void) {
    LCD_SendCommand(0x01);
    HAL_Delay(2);
}

void LCD_SetCursor(int row, int col) {
    uint8_t addr = (row == 0) ? 0x80 + col : 0xC0 + col;
    LCD_SendCommand(addr);
}

void LCD_Init(void) {
    HAL_Delay(50);
    LCD_SendCommand(0x30);
    HAL_Delay(5);
    LCD_SendCommand(0x30);
    HAL_Delay(1);
    LCD_SendCommand(0x30);
    HAL_Delay(10);
    LCD_SendCommand(0x20); // 4-bit mode
    HAL_Delay(10);

    LCD_SendCommand(0x28); // 2 line
    LCD_SendCommand(0x08); // display off
    LCD_SendCommand(0x01); // clear
    HAL_Delay(2);
    LCD_SendCommand(0x06); // entry mode
    LCD_SendCommand(0x0C); // display on
    HAL_Delay(10);
}

// --- EXTI Callback for IR Coin Sensor ---
volatile uint32_t lastCoinTick = 0; // Prevent coin bouncing
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == IR_Pin) {
        uint32_t currentTick = HAL_GetTick();
        if (currentTick - lastCoinTick > 300) { // 300ms hardware debounce
            lastCoinTick = currentTick;
            uint8_t coinDrop = 1;
            osMessagePut(coinQueueHandle, coinDrop, 0); // Non-blocking in ISR
        }
    }
}

// --- TASK IMPLEMENTATIONS ---

void StartCoinTask(void const * argument) {
    osEvent event;
    char uartBuf[64];
    for(;;) {
        event = osMessageGet(coinQueueHandle, osWaitForever);
        if (event.status == osEventMessage) {
            currentBalance += 1; // 1 unit = 10 Rs
            sprintf(uartBuf, "[COIN] Valid Coin! Balance After Insert: %d\r\n", currentBalance);
            Debug_Print(uartBuf);

            if (currentState == IDLE) {
                currentState = COIN_INSERTED;
                Debug_Print("[STATE] Transition: IDLE -> COIN_INSERTED\r\n");
            }
            osSignalSet(buzzerTaskHandle, 0x01);
        }
    }
}

void StartKeypadTask(void const * argument) {
    char uartBuf[64];
    const char keymap[4][4] = {
        {'1','2','3','A'},
        {'4','5','6','B'},
        {'7','8','9','C'},
        {'*','0','#','D'}
    };
    uint16_t rowPins[4] = {GPIO_PIN_0, GPIO_PIN_1, GPIO_PIN_2, GPIO_PIN_3}; // PA0-PA3
    uint16_t colPins[4] = {GPIO_PIN_4, GPIO_PIN_5, GPIO_PIN_6, GPIO_PIN_7}; // PA4-PA7

    for(;;) {
        for (int r = 0; r < 4; r++) {
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, GPIO_PIN_SET);
            HAL_GPIO_WritePin(GPIOA, rowPins[r], GPIO_PIN_RESET);
            osDelay(5);

            for (int c = 0; c < 4; c++) {
                if (HAL_GPIO_ReadPin(GPIOA, colPins[c]) == GPIO_PIN_RESET) {
                    char key = keymap[r][c];

                    // --- LONG PRESS DETECTION LOGIC ---
                    uint32_t pressStart = HAL_GetTick();
                    while(HAL_GPIO_ReadPin(GPIOA, colPins[c]) == GPIO_PIN_RESET) osDelay(10); // Wait for release
                    uint32_t pressDuration = HAL_GetTick() - pressStart;

                    if (!restockMode) {
                        // NORMAL MODE
                        if (key == '*' && pressDuration >= 3000) {
                            restockMode = 1;
                            restockStep = 99; // 99 = Password Entry Step
                            tempDigitIndex = 0;
                            memset((void*)pwdBuffer, 0, sizeof(pwdBuffer));
                            Debug_Print("[RESTOCK] Enter Password\r\n");
                            osSignalSet(buzzerTaskHandle, 0x01);
                        } else if (pressDuration < 3000) {
                            sprintf(uartBuf, "[KEYPAD] Key: %c\r\n", key);
                            Debug_Print(uartBuf);
                            osMessagePut(keypadQueueHandle, key, 10);
                        }
                    } else {
                        // RESTOCK MODE
                        // Universal Save & Exit trigger: Long press # for 2 seconds anywhere in restock
                        if (key == '#' && pressDuration >= 2000) {
                            restockMode = 0;
                            restockStep = 0;
                            Debug_Print("[RESTOCK] Saved & Exited to Normal Mode (Long #)\r\n");
                            osSignalSet(buzzerTaskHandle, 0x02);
                        }
                        else if (pressDuration < 2000) { // Standard short press processing

                            // --- Password Verification Step ---
                            if (restockStep == 99) {
                                if (key >= '0' && key <= '9') {
                                    if (tempDigitIndex < 4) { // Max 4 digits
                                        pwdBuffer[tempDigitIndex++] = key;
                                        pwdBuffer[tempDigitIndex] = '\0';
                                        osSignalSet(buzzerTaskHandle, 0x01); // Beep on keypress
                                    }
                                } else if (key == '#') { // Press # to Enter Password
                                    if (strcmp((char*)pwdBuffer, "0000") == 0) {
                                        restockStep = 0; // Correct password, go to Select Item
                                        Debug_Print("[RESTOCK] Password Correct. Entering Menu.\r\n");
                                        osSignalSet(buzzerTaskHandle, 0x02); // Success beep
                                    } else {
                                        restockMode = 0; // Wrong password, exit
                                        Debug_Print("[RESTOCK] Password Incorrect. Exiting.\r\n");
                                        osSignalSet(buzzerTaskHandle, 0x03); // Error beep
                                    }
                                } else if (key == '*') {
                                    restockMode = 0; // Cancel password entry
                                    Debug_Print("[RESTOCK] Password Entry Cancelled.\r\n");
                                    osSignalSet(buzzerTaskHandle, 0x02);
                                }
                            }
                            // --- Restock Standard Configuration Steps ---
                            else if (restockStep == 0) {
                                if (key == 'A' || key == 'B' || key == 'C' || key == 'D') {
                                    restockItem = key;
                                    restockStep = 1;
                                    tempInputValue = 0;
                                    tempDigitIndex = 0;
                                    Debug_Print("[RESTOCK] Select quantity\r\n");
                                    osSignalSet(buzzerTaskHandle, 0x01);
                                } else if (key == '*') {
                                    restockMode = 0; // Short press * exits if no item selected
                                    restockStep = 0;
                                    Debug_Print("[RESTOCK] Exited\r\n");
                                    osSignalSet(buzzerTaskHandle, 0x02);
                                }
                            }
                            else if (restockStep == 1) {
                                if (key >= '0' && key <= '9') {
                                    tempInputValue = tempInputValue * 10 + (key - '0');
                                    tempDigitIndex++;
                                    osSignalSet(buzzerTaskHandle, 0x01);
                                } else if (key == '#') {
                                    tempQty = tempInputValue > 99 ? 99 : tempInputValue;
                                    restockStep = 2;
                                    tempInputValue = 0;
                                    tempDigitIndex = 0;
                                    Debug_Print("[RESTOCK] Enter price\r\n");
                                    osSignalSet(buzzerTaskHandle, 0x01);
                                } else if (key == '*') {
                                    restockStep = 0;
                                    Debug_Print("[RESTOCK] Cancelled item\r\n");
                                    osSignalSet(buzzerTaskHandle, 0x03);
                                }
                            }
                            else if (restockStep == 2) {
                                if (key >= '0' && key <= '9') {
                                    tempInputValue = tempInputValue * 10 + (key - '0');
                                    tempDigitIndex++;
                                    osSignalSet(buzzerTaskHandle, 0x01);
                                } else if (key == '#') {
                                    tempPrice = tempInputValue < 1 ? 1 : tempInputValue;
                                    int idx = (restockItem == 'A') ? 0 : (restockItem == 'B') ? 1 : (restockItem == 'C') ? 2 : 3;
                                    itemStock[idx] = tempQty;
                                    itemCost[idx] = tempPrice;
                                    sprintf(uartBuf, "[RESTOCK] Item %c: Qty=%d, Price=%d\r\n", restockItem, tempQty, tempPrice);
                                    Debug_Print(uartBuf);
                                    restockStep = 0;
                                    osSignalSet(buzzerTaskHandle, 0x02);
                                } else if (key == '*') {
                                    restockStep = 1;
                                    tempInputValue = 0;
                                    tempDigitIndex = 0;
                                    Debug_Print("[RESTOCK] Re-enter quantity\r\n");
                                    osSignalSet(buzzerTaskHandle, 0x01);
                                }
                            }
                        }
                    }
                }
            }
        }
        osDelay(20);
    }
}

void StartControlTask(void const * argument) {
    osEvent keyEvent;
    char key;
    char uartBuf[64];
    int idx;
    uint8_t qtyRemaining = 0;

    for(;;) {
        if (restockMode) {
            osDelay(50);
            continue;
        }

        switch(currentState) {
            case IDLE:
                // Allow `#` button cycling in idle
                keyEvent = osMessageGet(keypadQueueHandle, 50);
                if (keyEvent.status == osEventMessage) {
                    key = (char)keyEvent.value.v;
                    if (key == '#') {
                        displayMenuState = (displayMenuState + 1) % 3;
                        osSignalSet(buzzerTaskHandle, 0x01);
                    }
                }
                break;

            case COIN_INSERTED:
                keyEvent = osMessageGet(keypadQueueHandle, 100);
                if (keyEvent.status == osEventMessage) {
                    key = (char)keyEvent.value.v;
                    if (key == 'A' || key == 'B' || key == 'C' || key == 'D') {
                        selectedItem = key;
                        requestedQty = 0;
                        currentState = SELECT_QUANTITY;
                        sprintf(uartBuf, "[CTRL] Item %c selected. Enter quantity.\r\n", selectedItem);
                        Debug_Print(uartBuf);
                    }
                    else if (key == '#') {
                        displayMenuState = (displayMenuState + 1) % 3;
                        osSignalSet(buzzerTaskHandle, 0x01);
                    }
                }
                break;

            case SELECT_QUANTITY:
                keyEvent = osMessageGet(keypadQueueHandle, 5000);
                if (keyEvent.status == osEventMessage) {
                    key = (char)keyEvent.value.v;
                    if (key >= '1' && key <= '9') {
                        requestedQty = key - '0';
                        sprintf(uartBuf, "[CTRL] Quantity = %d\r\n", requestedQty);
                        Debug_Print(uartBuf);
                        currentState = CHECK_BALANCE;
                    } else if (key == '*') {
                        currentState = COIN_INSERTED;
                        Debug_Print("[CTRL] Quantity selection cancelled\r\n");
                    }
                } else {
                    currentState = COIN_INSERTED;
                    Debug_Print("[CTRL] Quantity selection timeout\r\n");
                }
                break;

            case CHECK_BALANCE:
                idx = (selectedItem == 'A') ? 0 :
                      (selectedItem == 'B') ? 1 :
                      (selectedItem == 'C') ? 2 : 3;

                sprintf(uartBuf, "[CTRL] Pre-Check Balance: %d | Cost Required: %d\r\n", currentBalance, (itemCost[idx] * requestedQty));
                Debug_Print(uartBuf);

                if (itemStock[idx] < requestedQty) {
                    sprintf(uartBuf, "[CTRL] Only %d in stock, requested %d\r\n", itemStock[idx], requestedQty);
                    Debug_Print(uartBuf);
                    currentState = SYS_ERROR;
                }
                else if (currentBalance >= (itemCost[idx] * requestedQty)) {
                    // Safety logic: Do not deduct balance yet
                    sprintf(uartBuf, "[CTRL] Balance Verification OK. Attempting drop %d x %c\r\n", requestedQty, selectedItem);
                    Debug_Print(uartBuf);
                    qtyRemaining = requestedQty;
                    currentState = DISPENSE;
                }
                else {
                    Debug_Print("[CTRL] Insufficient balance\r\n");
                    currentState = SYS_ERROR;
                }
                break;

            case DISPENSE:
                Debug_Print("[CTRL] Starting dispense cycle\r\n");
                currentState = VERIFY_DROP;
                dropVerified = 0;

                // Send initial wake-up flag
                osSignalSet(motorTaskHandle, 0x01);

                // --- INFINITE WAIT ---
                // Will not timeout. Waits indefinitely for the sensor to flag a drop.
                while (dropVerified == 0) {
                    osDelay(100);
                }

                if (dropVerified) {
                    // --- ONLY CHARGE ON SUCCESS ---
                    idx = (selectedItem == 'A') ? 0 :
                          (selectedItem == 'B') ? 1 :
                          (selectedItem == 'C') ? 2 : 3;

                    if (currentBalance >= itemCost[idx]) {

                        sprintf(uartBuf, "[CTRL] Pre-Drop Balance Before Charge: %d\r\n", currentBalance);
                        Debug_Print(uartBuf);

                        currentBalance -= itemCost[idx];
                        itemStock[idx]--;
                        totalRevenue += itemCost[idx];
                        qtyRemaining--;

                        sprintf(uartBuf, "[CTRL] Success! Post-Drop Balance After Charge: %d\r\n", currentBalance);
                        Debug_Print(uartBuf);
                    }

                    if (qtyRemaining > 0) {
                        sprintf(uartBuf, "[CTRL] %d more to dispense\r\n", qtyRemaining);
                        Debug_Print(uartBuf);
                        dropVerified = 0;
                        currentState = DISPENSE;
                        osDelay(500);
                    } else {
                        currentState = SYS_SUCCESS;
                    }
                }
                break;

            case VERIFY_DROP:
                break;

            case SYS_SUCCESS:
                Debug_Print("[STATE] All items dispensed successfully.\r\n");
                osSignalSet(buzzerTaskHandle, 0x02);
                osDelay(3000);
                if (currentBalance > 0) {
                    currentState = COIN_INSERTED;
                } else {
                    currentState = IDLE;
                }
                selectedItem = '\0';
                requestedQty = 0;
                break;

            case SYS_ERROR:
                Debug_Print("[STATE] Error occurred.\r\n");
                osSignalSet(buzzerTaskHandle, 0x03);
                osDelay(3000);
                if (currentBalance > 0) currentState = COIN_INSERTED;
                else currentState = IDLE;
                selectedItem = '\0';
                requestedQty = 0;
                break;

            default:
                break;
        }
        osDelay(50);
    }
}

void StartMotorTask(void const * argument) {
    char uartBuf[64];
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);

    // --- INCREASED PWM TO 700 ---
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 700);

    for(;;) {
        osSignalWait(0x01, osWaitForever);

        int pulseCount = 0;

        // --- REMOVED TIMEOUT LOOP ---
        // Will pulse 1s ON / 1s OFF forever until dropVerified == 1
        while (dropVerified == 0) {

            sprintf(uartBuf, "[MOTOR] Pulse %d ON (Item %c)\r\n", pulseCount+1, selectedItem);
            Debug_Print(uartBuf);

            if (selectedItem == 'A') HAL_GPIO_WritePin(GPIOB, MA1_Pin, GPIO_PIN_SET);
            if (selectedItem == 'B') HAL_GPIO_WritePin(GPIOB, MB1_Pin, GPIO_PIN_SET);
            if (selectedItem == 'C') HAL_GPIO_WritePin(GPIOA, MC1_Pin, GPIO_PIN_SET);
            if (selectedItem == 'D') HAL_GPIO_WritePin(GPIOA, MD1_Pin, GPIO_PIN_SET);

            osDelay(300);

            Debug_Print("[MOTOR] Pulse OFF\r\n");

            HAL_GPIO_WritePin(GPIOB, MB1_Pin|MB2_Pin|MA1_Pin|MA2_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(GPIOA, MC1_Pin|MC2_Pin|MD1_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(MD2_GPIO_Port, MD2_Pin, GPIO_PIN_RESET);

            if (dropVerified) {
                Debug_Print("[MOTOR] Stopped early due to drop verification!\r\n");
                break;
            }

            osDelay(300);
            pulseCount++;
        }

        // Safety catch
        HAL_GPIO_WritePin(GPIOB, MB1_Pin|MB2_Pin|MA1_Pin|MA2_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOA, MC1_Pin|MC2_Pin|MD1_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(MD2_GPIO_Port, MD2_Pin, GPIO_PIN_RESET);
    }
}

void StartUltrasonicTask(void const * argument) {
    char uartBuf[64];
    uint32_t local_time = 0;
    uint32_t baselineTime = 0;
    uint32_t lastPrintTick = HAL_GetTick();

    // --- AUTO-CALIBRATE BASELINE AT BOOT ---
    osDelay(1000); // Wait for system to settle
    Debug_Print("[SENSOR] Calibrating Baseline Distance...\r\n");

    uint32_t sum = 0;
    int validReadings = 0;
    for(int i = 0; i < 10; i++) {
        HAL_GPIO_WritePin(GPIOB, TRIG_Pin, GPIO_PIN_SET);
        for(volatile int j=0; j<720; j++);
        HAL_GPIO_WritePin(GPIOB, TRIG_Pin, GPIO_PIN_RESET);

        local_time = 0;
        while (HAL_GPIO_ReadPin(ECHO_GPIO_Port, ECHO_Pin) == GPIO_PIN_RESET && local_time < 10000) local_time++;
        local_time = 0;
        while (HAL_GPIO_ReadPin(ECHO_GPIO_Port, ECHO_Pin) == GPIO_PIN_SET && local_time < 50000) local_time++;

        if (local_time > 0 && local_time < 40000) {
            sum += local_time;
            validReadings++;
        }
        osDelay(100);
    }

    if (validReadings > 0) {
        baselineTime = sum / validReadings;
    } else {
        baselineTime = 2400; // Fallback to safe default
    }

    sprintf(uartBuf, "[SENSOR] Baseline Auto-Set to: %lu\r\n", baselineTime);
    Debug_Print(uartBuf);

    for(;;) {
        // --- ACTIVE FAST SCANNING (During Dispense) ---
        if (currentState == VERIFY_DROP && dropVerified == 0) {
            HAL_GPIO_WritePin(GPIOB, TRIG_Pin, GPIO_PIN_SET);
            for(volatile int i=0; i<720; i++);
            HAL_GPIO_WritePin(GPIOB, TRIG_Pin, GPIO_PIN_RESET);

            local_time = 0;
            while (HAL_GPIO_ReadPin(ECHO_GPIO_Port, ECHO_Pin) == GPIO_PIN_RESET && local_time < 10000) local_time++;

            local_time = 0;
            while (HAL_GPIO_ReadPin(ECHO_GPIO_Port, ECHO_Pin) == GPIO_PIN_SET && local_time < 50000) local_time++;

            // DETECT SUDDEN DROP: If distance is 400 units (~6cm) shorter than baseline
            if (local_time > 0 && local_time < (baselineTime - 400)) {

                // Double verify to prevent false noise triggers
                osDelay(10);
                HAL_GPIO_WritePin(GPIOB, TRIG_Pin, GPIO_PIN_SET);
                for(volatile int i=0; i<720; i++);
                HAL_GPIO_WritePin(GPIOB, TRIG_Pin, GPIO_PIN_RESET);

                uint32_t verify_time = 0;
                while (HAL_GPIO_ReadPin(ECHO_GPIO_Port, ECHO_Pin) == GPIO_PIN_RESET && verify_time < 10000) verify_time++;
                verify_time = 0;
                while (HAL_GPIO_ReadPin(ECHO_GPIO_Port, ECHO_Pin) == GPIO_PIN_SET && verify_time < 50000) verify_time++;

                if (verify_time > 0 && verify_time < (baselineTime - 400)) {
                    sprintf(uartBuf, "[SENSOR] True Drop Confirmed! Time=%lu\r\n", verify_time);
                    Debug_Print(uartBuf);
                    dropVerified = 1;
                }
            }

            osDelay(40); // Fast 40ms polling
            lastPrintTick = HAL_GetTick(); // Keep idle timer reset while busy
        }
        // --- IDLE SCANNING (Every 2 seconds - Silent Auto-Drift) ---
        else {
            if (HAL_GetTick() - lastPrintTick >= 1700) {
                lastPrintTick = HAL_GetTick();

                HAL_GPIO_WritePin(GPIOB, TRIG_Pin, GPIO_PIN_SET);
                for(volatile int i=0; i<720; i++);
                HAL_GPIO_WritePin(GPIOB, TRIG_Pin, GPIO_PIN_RESET);

                local_time = 0;
                while (HAL_GPIO_ReadPin(ECHO_GPIO_Port, ECHO_Pin) == GPIO_PIN_RESET && local_time < 10000) local_time++;

                local_time = 0;
                while (HAL_GPIO_ReadPin(ECHO_GPIO_Port, ECHO_Pin) == GPIO_PIN_SET && local_time < 50000) local_time++;

                // If no object is falling, slightly adjust baseline to handle temperature/box shifting
                if (local_time > 0 && local_time < 40000) {
                    if (local_time > (baselineTime - 200) && local_time < (baselineTime + 200)) {
                        baselineTime = (baselineTime * 3 + local_time) / 4; // Slow moving average
                    }
                }

                // Flush any old signals sent by ControlTask to prevent queue buildup
                osSignalWait(0x01, 0);
            }
            osDelay(100); // Check state every 100ms when idle
        }
    }
}

void StartDisplayTask(void const * argument) {
    char lcdBuffer1[17];
    char lcdBuffer2[17];
    char tempBuf[32]; // For intermediate formatting before space padding
    VendingState_t lastState = 0xFF;
    uint16_t lastBalance = 0xFFFF;
    uint8_t lastRestockMode = 0xFF;
    uint8_t lastMenuState = 0xFF;
    uint8_t lastDigitIndex = 0xFF;  // Track digit changes to force clear
    uint32_t lastRefreshTick = HAL_GetTick();

    for(;;) {
        // --- PERIODIC REFRESH DISABLED (To prevent screen flicker/reset) ---
        // Only sending the config pulse, not wiping the state cache
        if (HAL_GetTick() - lastRefreshTick >= 3000) {
            lastRefreshTick = HAL_GetTick();
            LCD_SendCommand(0x0C); // Display ON, Cursor OFF backup
        }

        if (restockMode) {
            // Force redraw on step change or digit addition
            if (lastRestockMode != restockStep || lastDigitIndex != tempDigitIndex) {
                lastRestockMode = restockStep;
                lastDigitIndex = tempDigitIndex;
            }

            // --- STRICT SPACE PADDING FOR ALL RESTOCK MENUS TO PREVENT OVERLAP ---
            switch(restockStep) {
                case 99:
                    sprintf(lcdBuffer1, "%-16s", "ENTER PASSWORD:");
                    if (tempDigitIndex == 0) sprintf(lcdBuffer2, "                "); // 16 spaces
                    else if (tempDigitIndex == 1) sprintf(lcdBuffer2, "%-16s", "*");
                    else if (tempDigitIndex == 2) sprintf(lcdBuffer2, "%-16s", "**");
                    else if (tempDigitIndex == 3) sprintf(lcdBuffer2, "%-16s", "***");
                    else sprintf(lcdBuffer2, "%-16s", "****");
                    break;
                case 0:
                    sprintf(lcdBuffer1, "%-16s", "RESTOCK MODE");
                    sprintf(lcdBuffer2, "                "); // Empties the second line completely
                    break;
                case 1:
                    sprintf(tempBuf, "Item %c QTY:", restockItem);
                    sprintf(lcdBuffer1, "%-16s", tempBuf);

                    if (tempDigitIndex == 0) {
                        sprintf(lcdBuffer2, "                ");
                    } else {
                        sprintf(tempBuf, "Val: %d", tempInputValue);
                        sprintf(lcdBuffer2, "%-16s", tempBuf); // Pad the value with spaces
                    }
                    break;
                case 2:
                    sprintf(tempBuf, "Item %c COST:", restockItem);
                    sprintf(lcdBuffer1, "%-16s", tempBuf);

                    if (tempDigitIndex == 0) {
                        sprintf(lcdBuffer2, "                ");
                    } else {
                        sprintf(tempBuf, "Val: %d", tempInputValue);
                        sprintf(lcdBuffer2, "%-16s", tempBuf); // Pad the value with spaces
                    }
                    break;
                default:
                    sprintf(lcdBuffer1, "%-16s", "RESTOCK");
                    sprintf(lcdBuffer2, "                ");
            }

            LCD_SetCursor(0,0); LCD_String(lcdBuffer1);
            LCD_SetCursor(1,0); LCD_String(lcdBuffer2);
            osDelay(150);
            continue;
        } else {
            lastRestockMode = 0xFF;
            lastDigitIndex = 0xFF;
        }

        if (currentState != lastState || currentBalance != lastBalance || displayMenuState != lastMenuState) {
            lastState = currentState;
            lastBalance = currentBalance;
            lastMenuState = displayMenuState;
            LCD_Clear(); // Full clear is fine on major state change

            // --- STRICT SPACE PADDING FOR ALL NORMAL MENUS ---
            switch(currentState) {
                case IDLE:
                    if (displayMenuState == 1) {
                        sprintf(lcdBuffer1, "%-16s", "STOCK MENU");
                        sprintf(tempBuf, "A:%d B:%d C:%d D:%d", itemStock[0], itemStock[1], itemStock[2], itemStock[3]);
                        sprintf(lcdBuffer2, "%-16s", tempBuf);
                    } else if (displayMenuState == 2) {
                        sprintf(lcdBuffer1, "%-16s", "PRICE MENU");
                        sprintf(tempBuf, "A:%d B:%d C:%d D:%d", itemCost[0], itemCost[1], itemCost[2], itemCost[3]);
                        sprintf(lcdBuffer2, "%-16s", tempBuf);
                    } else {
                        sprintf(lcdBuffer1, "%-16s", "INSERT COIN");
                        sprintf(lcdBuffer2, "%-16s", "SELECT A-D (#)");
                    }
                    break;
                case COIN_INSERTED:
                    if (displayMenuState == 1) {
                        sprintf(tempBuf, "BAL:%d STOCK", currentBalance);
                        sprintf(lcdBuffer1, "%-16s", tempBuf);
                        sprintf(tempBuf, "A:%d B:%d C:%d D:%d", itemStock[0], itemStock[1], itemStock[2], itemStock[3]);
                        sprintf(lcdBuffer2, "%-16s", tempBuf);
                    } else if (displayMenuState == 2) {
                        sprintf(tempBuf, "BAL:%d PRICE", currentBalance);
                        sprintf(lcdBuffer1, "%-16s", tempBuf);
                        sprintf(tempBuf, "A:%d B:%d C:%d D:%d", itemCost[0], itemCost[1], itemCost[2], itemCost[3]);
                        sprintf(lcdBuffer2, "%-16s", tempBuf);
                    } else {
                        sprintf(tempBuf, "BAL:%d", currentBalance);
                        sprintf(lcdBuffer1, "%-16s", tempBuf);
                        sprintf(lcdBuffer2, "%-16s", "SELECT A-D (#)");
                    }
                    break;
                case SELECT_QUANTITY:
                    sprintf(tempBuf, "ITEM %c", selectedItem);
                    sprintf(lcdBuffer1, "%-16s", tempBuf);
                    sprintf(lcdBuffer2, "%-16s", "QTY 1-9 #");
                    break;
                case CHECK_BALANCE:
                case DISPENSE:
                    sprintf(lcdBuffer1, "%-16s", "DISPENSING...");
                    sprintf(tempBuf, "%c x%d", selectedItem, requestedQty);
                    sprintf(lcdBuffer2, "%-16s", tempBuf);
                    break;
                case VERIFY_DROP:
                    sprintf(lcdBuffer1, "%-16s", "PLEASE WAIT");
                    sprintf(lcdBuffer2, "%-16s", "VERIFYING DROP");
                    break;
                case SYS_SUCCESS:
                    sprintf(lcdBuffer1, "%-16s", "THANK YOU!");
                    sprintf(lcdBuffer2, "%-16s", "ENJOY");
                    break;
                case SYS_ERROR:
                    {
                        int idx = (selectedItem == 'A') ? 0 :
                                  (selectedItem == 'B') ? 1 :
                                  (selectedItem == 'C') ? 2 : 3;
                        if (itemStock[idx] < requestedQty) {
                            sprintf(lcdBuffer1, "%-16s", "OUT OF STOCK");
                            sprintf(tempBuf, "Only %d left", itemStock[idx]);
                            sprintf(lcdBuffer2, "%-16s", tempBuf);
                        } else if (currentBalance < (itemCost[idx] * requestedQty)) {
                            int need = (itemCost[idx] * requestedQty) - currentBalance;
                            sprintf(tempBuf, "NEED %d MORE", need);
                            sprintf(lcdBuffer1, "%-16s", tempBuf);
                            sprintf(tempBuf, "BAL:%d", currentBalance);
                            sprintf(lcdBuffer2, "%-16s", tempBuf);
                        } else {
                            sprintf(lcdBuffer1, "%-16s", "ERROR!");
                            sprintf(lcdBuffer2, "%-16s", "Item jammed");
                        }
                    }
                    break;
                default:
                    break;
            }
            LCD_SetCursor(0,0); LCD_String(lcdBuffer1);
            LCD_SetCursor(1,0); LCD_String(lcdBuffer2);
        }
        osDelay(150);
    }
}

void StartBuzzerTask(void const * argument) {
    osEvent event;
    for(;;) {
        event = osSignalWait(0, osWaitForever);

        if (event.value.signals == 0x01) {
            Debug_Print("[BUZZER] Short Beep\r\n");
            HAL_GPIO_WritePin(GPIOB, BUZZER_Pin, GPIO_PIN_SET);
            osDelay(100);
            HAL_GPIO_WritePin(GPIOB, BUZZER_Pin, GPIO_PIN_RESET);
        }
        else if (event.value.signals == 0x02) {
            Debug_Print("[BUZZER] Long Beep (Success)\r\n");
            HAL_GPIO_WritePin(GPIOB, BUZZER_Pin, GPIO_PIN_SET);
            osDelay(1000);
            HAL_GPIO_WritePin(GPIOB, BUZZER_Pin, GPIO_PIN_RESET);
        }
        else if (event.value.signals == 0x03) {
            Debug_Print("[BUZZER] Double Beep (Error)\r\n");
            HAL_GPIO_WritePin(GPIOB, BUZZER_Pin, GPIO_PIN_SET);
            osDelay(150);
            HAL_GPIO_WritePin(GPIOB, BUZZER_Pin, GPIO_PIN_RESET);
            osDelay(100);
            HAL_GPIO_WritePin(GPIOB, BUZZER_Pin, GPIO_PIN_SET);
            osDelay(150);
            HAL_GPIO_WritePin(GPIOB, BUZZER_Pin, GPIO_PIN_RESET);
        }
    }
}
/* USER CODE END 4 */

void StartDefaultTask(void const * argument)
{
  for(;;)
  {
    osDelay(1);
  }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM4)
  {
    HAL_IncTick();
  }
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif /* USE_FULL_ASSERT */
