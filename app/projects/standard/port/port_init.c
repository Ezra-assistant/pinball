#include "include.h"

sys_cb_t sys_cb AT(.buf.bsp.sys_cb);

void freqdet_init(void);





/* 自定义的代码 */
// 测试 打印字符串
// AT(.com_rodata.bsp.test)
// const char str[] = "msg = 0x%04X\n";
AT(.com_rodata.bsp.test)
const char str_trigger[] = "last digit seg = 0x%02X\n";


#if 1

/* 1.按键检测（开关机、清除实时分数、循环播放BGM、显示最高分规则、清空最高分、叶子触发） */
/* 按键扫描变量 与宏 */
#if 1
#define KEY1_PRESS_CODE      0x0A00
#define KEY2_PRESS_CODE      0x0B00
#define KEY3_PRESS_CODE      0x0C00
#define KEY4_PRESS_CODE      0x0D00
#define KEY5_PRESS_CODE      0x0E00
#define LEAF_KEY_PRESS_CODE  0x0F00


#define KEY_SUM 6

u16 key_return = 0; // 初始化按键返回值

u16 key_press_count[KEY_SUM] = {0};   // 按键按下时间
u8 last_key_state[KEY_SUM] = {0};     // 上一次按键状态
u8 key_processed[KEY_SUM] = {0};      // 防止长按时重复触发
u8 current_key_state[KEY_SUM] = {0};  // 按键当前状态 1-按下 0-松开

u16 key_msgs[KEY_SUM] = {
    KEY1_PRESS_CODE,
    KEY2_PRESS_CODE,
    KEY3_PRESS_CODE,
    KEY4_PRESS_CODE,
    KEY5_PRESS_CODE,
    LEAF_KEY_PRESS_CODE
};

#endif



/* 1.按键扫描 函数 */
#if 1
/* 函数声明 */
void leaf_key_scan(void);

AT(.com_text.key)
void keys_scan(void) {
    // 获取按键当前状态
    current_key_state[0] = (GPIOB & BIT(7)) == 0; // KEY1 开关键
    current_key_state[1] = (GPIOB & BIT(6)) == 0; // KEY2 清空实时分数
    current_key_state[2] = (GPIOB & BIT(5)) == 0; // KEY3 播放BGM
    current_key_state[3] = (GPIOB & BIT(4)) == 0; // KEY4 显示数码管最高分
    current_key_state[4] = (GPIOB & BIT(2)) == 0; // KEY5 清空最高分
    // current_key_state[5] = (GPIOB & BIT(9)) == 0; // LEAF_KEY 增加分数


    // 循环检测 KEY1~KEY5
    for(int i = 0; i < (KEY_SUM - 1); i++) {
        // 下降沿检测（按键按下）
        if (current_key_state[i] == 1 && last_key_state[i] == 0) {
            key_press_count[i] = 0;
            key_processed[i] = 0;
        }

        // 按键按住时累加（取消注释）
        if (current_key_state[i] == 1) {
            key_press_count[i]++;

            // 去抖后触发（到达6次且未处理）
            if (key_press_count[i] >= 6 && !key_processed[i]) {
                key_return = key_msgs[i];
                key_processed[i] = 1;  // 防止继续触发
            }
        }

        // 上升沿检测（按键松开）
        if (current_key_state[i] == 0 && last_key_state[i] == 1) {
            key_press_count[i] = 0;  // 复位计数
            key_processed[i] = 0;
        }

        last_key_state[i] = current_key_state[i];

        // 发送消息
        if (key_return != 0) {
            msg_enqueue(key_return);
            key_return = 0;
        }
    }

    // 单独检测 LEAF_KEY
    // leaf_key_scan();

}

AT(.com_text.key)
void leaf_key_scan(void) {
    if (current_key_state[5] == 1 && last_key_state[5] == 0) {
        key_press_count[5] = 0;
        key_processed[5] = 0;
    }

    // 按键按住时累加（取消注释）
    if (current_key_state[5] == 1) {
        key_press_count[5]++;
        delay_us(60);
        current_key_state[5] = (GPIOB & BIT(9)) == 0;

        if (current_key_state[5] == 1) {
            key_press_count[5]++;

            if (key_press_count[5] >= 2 && !key_processed[5]) {
                key_return = key_msgs[5];
                key_processed[5] = 1;  // 防止继续触发
            }
        }

        // 上升沿检测（按键松开）
        if (current_key_state[5] == 0 && last_key_state[5] == 1) {
            key_press_count[5] = 0;  // 复位计数
            key_processed[5] = 0;
        }
        last_key_state[5] = current_key_state[5];

        // 发送消息
        if (key_return != 0) {
            msg_enqueue(key_return);
            key_return = 0;
        }
    }
}
#endif




/* 2.数码管刷新 */
#if 1
#define MAX_SCORE_DISPLAY_INTERVAL 600 // ms
#define DIGIT_NUM 4 // 一共有几个数码管
#define IS_GPIOA  1 // 用于判断是否调用 GPIOA or GPIOB
#define IS_GPIOB  2

#define DISPLAY_OFFSET 24

u8 gnd_count_flag = 0;

u32 display_register = 0; // 默认不显示

u8 max_score_display_count_flag = 0xFF; // 最高分显示 flag
u16 max_score_display_count_interval = 0;

// 定义段码对应的GPIO端口和引脚
typedef struct {
    uint32_t is_portx;  // 存储GPIO端口基地址
    uint16_t pin;        // 引脚号（BIT0、BIT8等）
} SegPinMap;

// 建立 A~DP 的映射表（索引0~7对应A~DP）
AT(.com_text.rodata)
const SegPinMap seg_pin[8] = {
    {IS_GPIOA, 4},  // A
    {IS_GPIOA, 0},  // B
    {IS_GPIOA, 11}, // C
    {IS_GPIOA, 7},  // D
    {IS_GPIOA, 6},  // E
    {IS_GPIOA, 3},  // F
    {IS_GPIOA, 12}, // G
    {IS_GPIOA, 8}   // DP
};



// 函数声明
void seg_display(u32 display_register1, u8 gnd_count_flag1);
void change_common_cathode_gnd(u8 gnd_count_flag);
void set_seg7_states(u32 display_register, u8 gnd_count_flag);

AT(.com_text.seg)
void seg7_refresh(void) {
    u16 msg = msg_dequeue();

    // 1.显示最高分规则 - flag 清零
    if (msg == 0x0D11) {
        max_score_display_count_flag = 0;
    }
    else {
        // 没用上的 msg 放回队列
        msg_enqueue(msg);
    }

    // 1.显示最高分规则 - 判断条件 与 显示
    if (max_score_display_count_flag <= 6) {
        // 如果是第一个显示，就直接显示
        if (max_score_display_count_flag == 0) {
            display_register = display_max_score_table[max_score_display_count_flag++];
        }
        else {
            max_score_display_count_interval++; // 每次进来 +1 ms

            // 已经间隔了600ms才显示下一个
            if (max_score_display_count_interval >= MAX_SCORE_DISPLAY_INTERVAL) {
                display_register = display_max_score_table[max_score_display_count_flag++];

                printf(str_trigger, max_score_display_count_flag);

                max_score_display_count_interval = 0;
            }
        }
    } 
    else {
        max_score_display_count_flag = 0xFF;

        // 2.实时分数显示（如果最高分规则不显示的话）
        display_register = real_time_scores_reg;
    }

    
    seg_display(display_register, gnd_count_flag);


    // GND 持有共阴线路往前一步
    gnd_count_flag++;
    if (gnd_count_flag >= DIGIT_NUM) gnd_count_flag = 0;
}

AT(.com_text.seg)
void seg_display(u32 display_register1, u8 gnd_count_flag1) {
    // 1.切换数码管 共阴线路
    change_common_cathode_gnd(gnd_count_flag1);
    delay_us(10);

    // 2.设置当前行的 IO 状态
    set_seg7_states(display_register1, gnd_count_flag1);
}

AT(.com_text.seg)
void change_common_cathode_gnd(u8 gnd_count_flag2) {
    if (gnd_count_flag2 == 0) {
        GPIOADE  |=  BIT(5);
        GPIOADIR &= ~BIT(5);
        GPIOA    &= ~BIT(5);

        GPIOADE  &= ~BIT(2);
        GPIOADIR |=  BIT(2);
        GPIOADE  &= ~BIT(1);
        GPIOADIR |=  BIT(1);
        GPIOBDE  &= ~BIT(0);
        GPIOBDIR |=  BIT(0);
    }
    else if (gnd_count_flag2 == 1) {
        GPIOADE  |=  BIT(2);
        GPIOADIR &= ~BIT(2);
        GPIOA    &= ~BIT(2);

        GPIOADE  &= ~BIT(5);
        GPIOADIR |=  BIT(5);
        GPIOADE  &= ~BIT(1);
        GPIOADIR |=  BIT(1);
        GPIOBDE  &= ~BIT(0);
        GPIOBDIR |=  BIT(0);

    }
    else if (gnd_count_flag2 == 2) {
        GPIOADE  |=  BIT(1);
        GPIOADIR &= ~BIT(1);
        GPIOA    &= ~BIT(1);

        GPIOADE  &= ~BIT(5);
        GPIOADIR |=  BIT(5);
        GPIOADE  &= ~BIT(2);
        GPIOADIR |=  BIT(2);
        GPIOBDE  &= ~BIT(0);
        GPIOBDIR |=  BIT(0);

    }
    else if (gnd_count_flag2 == 3) {
        GPIOBDE  |=  BIT(0);
        GPIOBDIR &= ~BIT(0);
        GPIOB    &= ~BIT(0);

        GPIOADE  &= ~BIT(5);
        GPIOADIR |=  BIT(5);
        GPIOADE  &= ~BIT(2);
        GPIOADIR |=  BIT(2);
        GPIOADE  &= ~BIT(1);
        GPIOADIR |=  BIT(1);
    }

}

AT(.com_text.seg)
void set_seg7_states(u32 display_register, u8 gnd_count_flag) {

    // 获取当前行 8 位
    u8 display_register_u8 = display_register >> (DISPLAY_OFFSET - (gnd_count_flag * 8));

    for (int i = 0; i < 8; i++) {
        if (display_register_u8 & (1 << i)) { // 该bit为1
            if (seg_pin[i].is_portx == IS_GPIOA) {
                GPIOADE  |=  BIT(seg_pin[i].pin);
                GPIOADIR &= ~BIT(seg_pin[i].pin);
                GPIOA    |=  BIT(seg_pin[i].pin);
            }
            if (seg_pin[i].is_portx == IS_GPIOB) {
                GPIOBDE  |=  BIT(seg_pin[i].pin);
                GPIOBDIR &= ~BIT(seg_pin[i].pin);
                GPIOB    |=  BIT(seg_pin[i].pin);
            }
        }
        else {
            if (seg_pin[i].is_portx == IS_GPIOA) {
                GPIOA    &= ~BIT(seg_pin[i].pin);
                GPIOADE  &= ~BIT(seg_pin[i].pin);
                GPIOADIR |=  BIT(seg_pin[i].pin);
            }
            if (seg_pin[i].is_portx == IS_GPIOB) {
                GPIOB    &= ~BIT(seg_pin[i].pin);
                GPIOBDE  &= ~BIT(seg_pin[i].pin);
                GPIOBDIR |=  BIT(seg_pin[i].pin);
            }
        }
    }
}



#endif /* 数码管刷新 */




#endif /* 自定义的代码 */

//timer tick interrupt(5ms)
AT(.com_text.timer)
void usr_tmr5ms_isr(void)
{
/* 自定义的代码 */
#if 1
    /* 1.按键检测（开关机、清除实时分数、循环播放BGM、显示最高分规则、清空最高分、叶子触发） */
    keys_scan();

    /* 2.数码管刷新 */
    // seg7_refresh();


#endif




    sys_cb.tmr5ms_cnt++;
#if !USER_KEY_KNOB2_EN && !MATRIX_XY_KEY_SCAN_SEL
    bsp_key_scan();
#endif

#if MUSIC_SDCARD_EN
    sd_detect();
#endif // MUSIC_SDCARD_EN

#if MUSIC_SDCARD1_EN
    sd1_detect();
#endif // MUSIC_SDCARD1_EN

#if RGB_SERIAL_EN
    // spi1_led_data_send_proc();
    sys_cb.rgb_update_flag = 1;
#endif

#if DAC_SOFT_DNR
    if ((sys_cb.tmr5ms_cnt % 2) == 0) {    //10ms检测dac dnr
        dac_dnr_detect();
    }
#endif

    //500ms timer process
    if ((sys_cb.tmr5ms_cnt % 100) == 0) {
        sys_cb.cm_times++;
    }

    //1s timer process
    if ((sys_cb.tmr5ms_cnt % 200) == 0) {
        msg_enqueue(MSG_SYS_1S);
        sys_cb.tmr5ms_cnt = 0;
        sys_cb.lpwr_warning_cnt++;

        #if (DAC_CLASSD_VDET && VBAT_DETECT_EN)
            dac_classd_dec_proc(sys_cb.vbat);
        #endif
    }
}

//timer tick interrupt(1ms)
AT(.com_text.timer)
void usr_tmr1ms_isr(void)
{

    /* 2.数码管刷新 */
    seg7_refresh();









    sys_cb.tmr1ms_cnt++;

    if ((sys_cb.tmr1ms_cnt % 5) == 0) {
        usr_tmr5ms_isr();
    }

#if LED_DISP_EN
    if ((sys_cb.tmr1ms_cnt % 50) == 0) {
        led_scan();
    }
#endif // LED_DISP_EN

#if !USER_KEY_KNOB2_EN && MATRIX_XY_KEY_SCAN_SEL
    bsp_key_scan();
#endif

    //100ms timer process
    if ((sys_cb.tmr1ms_cnt % 100) == 0) {
        sys_cb.tmr1ms_cnt = 0;
        if (sys_cb.lpwr_cnt > 0) {
            sys_cb.lpwr_cnt++;
        }
#if MIDI_METRO_EN
        metro_tick_up();
#endif
    }

#if TKEY_SCAN_SWITCH_EN
    if ((sys_cb.tmr1ms_cnt % TKEY_SCAN_TIME) == 0) {
    	tkey_cir_scan_en();
    }
#endif
}

AT(.text.bsp.sys.init)
void power_on_off_init(void)
{
#if SOFT_POWER_ON_OFF
    //避免ADkey睡眠唤醒后状态异常
    // if(POWER_ON_FALL_IO > IO_PA15){
    //     GPIOBFEN &= ~BIT(POWER_ON_FALL_IO-1-IO_PA15);
    //     GPIOBDE  |=  BIT(POWER_ON_FALL_IO-1-IO_PA15);
    //     GPIOBDIR |=  BIT(POWER_ON_FALL_IO-1-IO_PA15);
    // }else{
    //     GPIOAFEN &= ~BIT(POWER_ON_FALL_IO-1);
    //     GPIOADE  |=  BIT(POWER_ON_FALL_IO-1);
    //     GPIOADIR |=  BIT(POWER_ON_FALL_IO-1);
    // }
#endif
}

AT(.rodata.vol)
const u8 maxvol_tbl[4] = {16, 32, 50};

//开user timer前初始化的内容
AT(.text.bsp.sys.init)
static void bsp_var_init(void)
{
    memset(&sys_cb, 0, sizeof(sys_cb));
    sys_cb.ms_ticks = tick_get();
    sys_cb.vol_max = maxvol_tbl[xcfg_cb.vol_max];
    if (SYS_INIT_VOLUME > sys_cb.vol_max) {
        SYS_INIT_VOLUME = sys_cb.vol_max;
    }
    if (WARNING_VOLUME > sys_cb.vol_max) {
        WARNING_VOLUME = sys_cb.vol_max;
    }

    // key_var_init();
    plugin_var_init();

    msg_queue_init();

    dac_cb_init();
#if MUSIC_SDCARD_EN || MUSIC_SDCARD1_EN
    dev_init(is_sd_support());
#endif

#if MUSIC_SDCARD_EN
//    if((xcfg_cb.sddet_iosel == IO_MUX_SDCLK) || (xcfg_cb.sddet_iosel == IO_MUX_SDCMD)) {
//        dev_delay_offline_times(DEV_SDCARD, 3); //复用时, 加快拔出检测. 这里拔出检测为3次.
//    }
#endif

#if MUSIC_SDCARD1_EN
//    if((xcfg_cb.sd1det_iosel == IO_MUX_SDCLK) || (xcfg_cb.sd1det_iosel == IO_MUX_SDCMD)) {
//        dev_delay_offline_times(DEV_SDCARD1, 3); //复用时, 加快拔出检测. 这里拔出检测为3次.
//    }
#endif

#if (MUSIC_UDISK_EN || MUSIC_SDCARD_EN || MUSIC_SDCARD1_EN)
    //fs_var_init();
#endif
}

AT(.text.bsp.sys.init)
static void bsp_io_init(void)
{
    GPIOADE = 0;
    GPIOBDE = 0;
    GPIOGDE = 0x3F;             //MCP FLASH
    uart0_mapping_sel();        //调试UART IO选择或关闭
    mclr_reset_init();

#if MUSIC_SDCARD_EN
    SD_DETECT_INIT();
#endif // MUSIC_SDCARD_EN

#if MUSIC_SDCARD1_EN
    SD1_DETECT_INIT();
#endif // MUSIC_SDCARD1_EN
}

AT(.text.bsp.sys.init)
void bsp_update_init(void)
{
    /// config
    if (!xcfg_init(&xcfg_cb, sizeof(xcfg_cb))) {           //获取配置参数
        printf("xcfg init error\n");
    }

    // io init
    bsp_io_init();

    // var init
    bsp_var_init();
    sys_cb.lang_id = 0;

    // peripheral init
    rtc_init();
    param_init(sys_cb.rtc_first_pwron);

    plugin_init();
    sys_set_tmr_enable(1);

    adpll_init(SYS_CLK_SEL);
#if DAC_EN
    dac_init();
#endif
#if WARNING_UPDATE_DONE
    mp3_res_play(RES_BUF_UPDATE_DONE_MP3, RES_LEN_UPDATE_DONE_MP3, 0);
#endif
}

AT(.text.bsp.sys.init)
void bsp_sys_init(void)
{
    /// config
    if (!xcfg_init(&xcfg_cb, sizeof(xcfg_cb))) {           //获取配置参数
        printf("xcfg init error\n");
    }

    // io init
    bsp_io_init();

    // var init
    bsp_var_init();

    // power init
    pmu_init(0);

    // clock init
    adpll_init(SYS_CLK_SEL);
    set_sys_clk(SYS_CLK_SEL);
    // dbg_clk_out(9, 10);

    // peripheral init
    rtc_init();
    param_init(sys_cb.rtc_first_pwron);
    plugin_init();
    power_on_off_init();        //开关机io初始化

#if IRRX_SW_EN
    irrx_sw_init();
#endif // IRRX_SW_EN

    led_init();
    key_init();

    /// enable user timer for display & dac
    sys_set_tmr_enable(1);

#if LED_DISP_EN
    led_power_up();
#endif // LED_DISP_EN

#if DAC_EN
    dac_init();
#endif

    bsp_change_volume(sys_cb.vol);

#if DAC_DRC_EN
    drc_v3_init((u8 *)RES_BUF_DRC_DAC_MUSIC_DRC, RES_LEN_DRC_DAC_MUSIC_DRC);
#endif

#if EX_SPIFLASH_SUPPORT
    exspiflash_init();
#endif

#if SUPPORT_SDCARD0_MUSIC
    sdcard_music_init();
#endif // SUPPORT_SDCARD0_MUSIC

#if SPI_DUMP_EN
    my_spi_init();
#endif
#if HUART_DEUMP_EN
    huart_dump_init();
#endif

#if HUART_EN
    bsp_huart_init();
#endif

#if USER_UART0_EN
    bsp_uart_init();
#endif

#if MIDI_UART0_EN
    midi_uart_init();
#endif

#if TKEY_MUL_SCAN_EN
    tkey_init();
#endif // TKEY_MUL_SCAN_EN

#if MUSIC_SDCARD_EN
    sd_soft_detect_poweron_check();
#endif // MUSIC_SDCARD_EN

#if SPI1_AUDIO_EN
    spi1_audio_init();
#endif

#if DAC_SOFT_EQ_EN
    dac_set_eq_by_res(&RES_BUF_EQ_DAC_16K_EQ, &RES_LEN_EQ_DAC_16K_EQ);
    // dac_set_eq_by_res(&RES_BUF_EQ_DAC_48K_EQ, &RES_LEN_EQ_DAC_48K_EQ);
#endif

#if WARNING_TONE_EN || MIDI_DEC_BK_EN
    music_res_init();
#endif

#if WARNING_POWER_ON && !SPI1_AUDIO_TEST_EN
    // mp3_res_play(RES_BUF_EN_POWERON_MP3, RES_LEN_EN_POWERON_MP3, 0);
    vmp3_res_play(RES_BUF_EN_POWERON_VMP3, RES_LEN_EN_POWERON_VMP3, 0);
#endif

#if PWM_HW_EN
    pwm_hw_cfg_init();
#endif // PWM_HW_EN

#if PWM_TMR2_EN
    tmr2pwm_cfg_init();
#endif // PWM_TMR2_EN

#if FREQ_DET_EN
    freqdet_init();
#endif // FREQ_DET_EN

#if TMR2_US_EN
    timer2_init();
#endif  //TMR2_US_EN

#if RGB_SERIAL_EN
    spi1_led_init();
    spi1_test_set();
#endif

#if LCD_DISPLAY_EN
    lcd_display_init();
#endif

#if MIDI_EN
    midi_init();
    #if MIDI_REC_EN
        midi_rec_init();
    #endif
    #if MIDI_METRO_EN
        midi_metro_init();
    #endif
#endif

#if UART_S_UPDATE
    uart_upd_init(UART_UPD_PORT_SEL,UART_UPD_BAUD,SYS_CLK_SEL);
    uart_upd_isr_init();
#endif // UART_S_UPDATE
}
