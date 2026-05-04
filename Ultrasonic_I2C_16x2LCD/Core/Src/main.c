#include "main.h"
#include <string.h>
#include <stdio.h>

/* Handles */
I2C_HandleTypeDef hi2c1;
TIM_HandleTypeDef htim1;

/* Ultrasonic pins */
#define TRIG_PIN GPIO_PIN_8
#define TRIG_PORT GPIOA
#define ECHO_PIN GPIO_PIN_9
#define ECHO_PORT GPIOA

/* LCD */
#define LCD_ADDR (0x27 << 1)

/* Variables */
uint32_t pMillis;
uint32_t Value1 = 0;
uint32_t Value2 = 0;
uint16_t Distance = 0;

/* ---------- LCD FUNCTIONS ---------- */
void lcd_send_cmd(char cmd)
{
    uint8_t data_u = (cmd & 0xF0);
    uint8_t data_l = ((cmd << 4) & 0xF0);
    uint8_t data_t[4];

    data_t[0] = data_u | 0x0C;
    data_t[1] = data_u | 0x08;
    data_t[2] = data_l | 0x0C;
    data_t[3] = data_l | 0x08;

    HAL_I2C_Master_Transmit(&hi2c1, LCD_ADDR, data_t, 4, 100);
}

void lcd_send_data(char data)
{
    uint8_t data_u = (data & 0xF0);
    uint8_t data_l = ((data << 4) & 0xF0);
    uint8_t data_t[4];

    data_t[0] = data_u | 0x0D;
    data_t[1] = data_u | 0x09;
    data_t[2] = data_l | 0x0D;
    data_t[3] = data_l | 0x09;

    HAL_I2C_Master_Transmit(&hi2c1, LCD_ADDR, data_t, 4, 100);
}

void lcd_init(void)
{
    HAL_Delay(50);
    lcd_send_cmd(0x30);
    HAL_Delay(5);
    lcd_send_cmd(0x30);
    HAL_Delay(1);
    lcd_send_cmd(0x20);

    lcd_send_cmd(0x28);
    lcd_send_cmd(0x08);
    lcd_send_cmd(0x01);
    HAL_Delay(2);
    lcd_send_cmd(0x06);
    lcd_send_cmd(0x0C);
}

void lcd_set_cursor(int row, int col)
{
    uint8_t addr = (row == 0) ? 0x80 + col : 0xC0 + col;
    lcd_send_cmd(addr);
}

void lcd_send_string(char *str)
{
    while (*str) lcd_send_data(*str++);


}

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM1_Init(void);

/* ---------- MAIN ---------- */
int main(void)
{
  HAL_Init();
  SystemClock_Config();

  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_TIM1_Init();

  HAL_TIM_Base_Start(&htim1);

  lcd_init();

  HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_RESET);

  while (1)
  {
      // 🔹 Trigger (stable method)
      HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_SET);
      HAL_Delay(1);   // IMPORTANT: 1ms works reliably
      HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_RESET);

      // 🔹 Wait for echo HIGH (with timeout)
      uint32_t timeout = HAL_GetTick();
      while (!(HAL_GPIO_ReadPin(ECHO_PORT, ECHO_PIN)))
      {
          if (HAL_GetTick() - timeout > 10) break;
      }

      __HAL_TIM_SET_COUNTER(&htim1, 0);
      Value1 = __HAL_TIM_GET_COUNTER(&htim1);

      // 🔹 Wait for echo LOW
      timeout = HAL_GetTick();
      while (HAL_GPIO_ReadPin(ECHO_PORT, ECHO_PIN))
      {
          if (HAL_GetTick() - timeout > 50) break;
      }

      Value2 = __HAL_TIM_GET_COUNTER(&htim1);

      // 🔹 Distance calculation
      if (Value2 > Value1)
          Distance = (Value2 - Value1) * 0.034 / 2;
      else
          Distance = ((65535 - Value1) + Value2) * 0.034 / 2;

      // 🔹 LCD Display
      char buffer[16];

      lcd_set_cursor(0,0);
      lcd_send_string("Distance:");

      sprintf(buffer, "%d cm   ", Distance);
      lcd_set_cursor(1,0);
      lcd_send_string(buffer);

      HAL_Delay(200);
  }
}
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  // TRIG PA8
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);
  GPIO_InitStruct.Pin = GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  // ECHO PA9
  GPIO_InitStruct.Pin = GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

static void MX_I2C1_Init(void)
{
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  HAL_I2C_Init(&hi2c1);
}

static void MX_TIM1_Init(void)
{
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 71;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 65535;
  HAL_TIM_Base_Init(&htim1);
}
