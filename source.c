#include "stm32f4xx.h"

/* ================= CONFIG ================= */
#define USER_BTN_PIN     0     // PA0
#define DEBOUNCE_MS      20
#define LONG_PRESS_MS    2000
#define STEP_DELAY_MS    3

/* ================= MOTOR CONSTANTS ================= */
#define STEPS_PER_REV        4080
#define SECONDS_PER_MINUTE   60
#define SECONDS_PER_HOUR     3600
#define SECONDS_PER_12H      (12 * 3600)

/* ================= STEP SEQUENCE (HALF STEP) ================= */
const uint8_t step_seq[8] =
{
    0x8, 0xC, 0x4, 0x6,
    0x2, 0x3, 0x1, 0x9
};

/* ================= GLOBALS ================= */
volatile uint32_t ms_ticks = 0;

/* modes */
enum {
    MODE_NORMAL = 0,
    MODE_EDIT_SECONDS,
    MODE_EDIT_MINUTES,
    MODE_EDIT_HOURS
};
int mode = MODE_NORMAL;

/* motors */
enum {
    MOTOR_SECONDS = 0,
    MOTOR_MINUTES,
    MOTOR_HOURS
};

/* step indices */
int step_index[3] = {0, 0, 0};

/* accumulators */
uint32_t minute_acc = 0;
uint32_t hour_acc   = 0;

/* button */
int btn_stable = 0, btn_last = 0;
uint32_t btn_last_change = 0;
uint32_t btn_press_start = 0;
int long_reported = 0;
int button_short = 0, button_long = 0;

/* ================= SYSTICK ================= */
void SysTick_Handler(void)
{
    ms_ticks++;
}

void systick_init(void)
{
    SystemCoreClockUpdate();
    SysTick_Config(SystemCoreClock / 1000);
}

/* ================= GPIO ================= */
void gpio_init(void)
{
    int i;

    RCC->AHB1ENR |= (1<<1) | (1<<0);   // GPIOB, GPIOA

    /* PB0–PB11 outputs */
    for(i = 0; i < 12; i++)
    {
        GPIOB->MODER &= ~(3<<(i*2));
        GPIOB->MODER |=  (1<<(i*2));
    }

    /* PA0 input */
    GPIOA->MODER &= ~(3<<(USER_BTN_PIN*2));
}

/* ================= ONE AND ONLY MOTOR FUNCTION ================= */
void rotate_motor(int motor, int steps)
{
    uint32_t mask, shift;
    int i;
    uint32_t start;

    if(motor == MOTOR_SECONDS)
    {
        mask = 0x000F;  // PB0–3
        shift = 0;
    }
    else if(motor == MOTOR_MINUTES)
    {
        mask = 0x00F0;  // PB4–7
        shift = 4;
    }
    else
    {
        mask = 0x0F00;  // PB8–11
        shift = 8;
    }

    for(i = 0; i < steps; i++)
    {
        GPIOB->ODR = (GPIOB->ODR & ~mask) |
                     ((uint32_t)step_seq[step_index[motor]] << shift);

        step_index[motor] = (step_index[motor] + 1) % 8;

        start = ms_ticks;
        while(ms_ticks - start < STEP_DELAY_MS);
    }
}

/* ================= BUTTON ================= */
void button_update(void)
{
    int sample = (GPIOA->IDR & (1<<USER_BTN_PIN)) ? 1 : 0;

    if(sample != btn_last)
    {
        btn_last_change = ms_ticks;
        btn_last = sample;
    }

    if(ms_ticks - btn_last_change >= DEBOUNCE_MS)
    {
        if(btn_stable != sample)
        {
            btn_stable = sample;
            if(sample)
            {
                btn_press_start = ms_ticks;
                long_reported = 0;
            }
            else if(!long_reported)
                button_short = 1;
        }
    }

    if(btn_stable && !long_reported &&
       (ms_ticks - btn_press_start) >= LONG_PRESS_MS)
    {
        long_reported = 1;
        button_long = 1;
    }
}

/* ================= MAIN ================= */
int main(void)
{
    uint32_t last_second = 0;

    gpio_init();
    systick_init();

    while(1)
    {
        button_update();

        /* ---- MODE SWITCH ---- */
        if(button_long)
        {
            button_long = 0;
            mode++;
            if(mode > MODE_EDIT_HOURS)
                mode = MODE_NORMAL;
        }

        /* ---- SHORT PRESS ---- */
        if(button_short)
        {
            button_short = 0;

            if(mode == MODE_EDIT_SECONDS)
                rotate_motor(MOTOR_SECONDS, STEPS_PER_REV / 60);

            else if(mode == MODE_EDIT_MINUTES){
                rotate_motor(MOTOR_MINUTES, STEPS_PER_REV / 60);
								rotate_motor(MOTOR_HOURS, STEPS_PER_REV / (60*12));
						}
            else if(mode == MODE_EDIT_HOURS)
                rotate_motor(MOTOR_HOURS, STEPS_PER_REV / 12);
        }

        /* ---- 1 SECOND TICK ---- */
        if(ms_ticks - last_second >= 1000)
        {
            last_second += 1000;

            /* seconds (exact) */
            if(mode != MODE_EDIT_SECONDS)
                rotate_motor(MOTOR_SECONDS, STEPS_PER_REV / 60);

            if(mode == MODE_NORMAL)
            {
                /* minutes */
                minute_acc += STEPS_PER_REV;
                while(minute_acc >= SECONDS_PER_HOUR)
                {
                    rotate_motor(MOTOR_MINUTES, 1);
                    minute_acc -= SECONDS_PER_HOUR;
                }

                /* hours */
                hour_acc += STEPS_PER_REV;
                while(hour_acc >= SECONDS_PER_12H)
                {
                    rotate_motor(MOTOR_HOURS, 1);
                    hour_acc -= SECONDS_PER_12H;
                }
            }
        }
    }
}
