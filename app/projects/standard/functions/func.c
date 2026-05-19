#include "include.h"

func_cb_t func_cb AT(.buf.func_cb);

void spi1_led_auto_process(void);
void dumphuart_io_test(void);
#if SYS_SLEEP_TIME
static u8 pwroff_flag = 0;
#endif

#if UART_S_UPDATE
bool deal_update_uart_msg(void);
void uart_s_update(void);
#endif // UART_S_UPDATE

AT(.text.func.process)
void print_info(void)
{
    static u32 ticks = 0;
    if (tick_check_expire(ticks,1000)) {
        ticks = tick_get();
        my_printf(".");
        //dumphuart_io_test();
    }
}

AT(.text.func.process)
void func_process(void)
{
    WDT_CLR();
    print_info();

#if RGB_SERIAL_EN
    if (sys_cb.rgb_update_flag) {
        sys_cb.rgb_update_flag = 0;
        SPI1BAUD = get_sysclk_nhz()/SPI_LED_BAUD - 1;//防止修改时钟频率后影响SPILED
        spi1_led_auto_process();
        spi1_led_data_send_proc();
    }
    // spi1_led_auto_process();
    // spi1_led_data_send_proc();
#endif

#if MUSIC_DECODE_BK_EN
    music_decode_process();
#endif
#if VBAT_DETECT_EN
    lowpower_vbat_process();
#if VDDIO_FOLLOW_VBAT_EN
    vddio_follow_vbat_process();
#endif
#endif // VBAT_DETECT_EN

#if HUART_EN
    huart_deal_process();
#endif
#if USER_UART0_EN
    uart_deal_process();
#endif
#if MIDI_UART0_EN
    midi_uart_rx_process();
    midi_uart_tx_process();
#endif
#if MAXTRIX_TRIGLE_KEYBOARD_EN
    maxtrix_triangle_scan_process();
#endif // MAXTRIX_TRIGLE_KEYBOARD_EN
#if TKEY_PRESS_UPDATE
    tkey_press_update();
#endif
#if TKEY_BUF_DOWN_EN
    tkey_tkbuf_down();
#endif
#if (MIDI_EN && MIDI_METRO_EN)
    midi_metro_proc();
#endif
#if PWM_HW_CLK_AUTO_SHIFT
    pwm_clk_sync();
#endif
#if SPI1_AUDIO_EN
    spi1_main_process();
#endif
#if MSC_BREAK_P_SOFT_FADE
    break_play_fade_process();
#endif
    soft_fade_process();
}

//func common message process
AT(.text.func.msg)
void func_message(u16 msg)
{
    switch (msg) {
#if (MIDI_KEYS_TEST || MIDI_REC_TEST_EN)
        case K_VOL_UP:
        #if DAC_SOFT_DNR
            dac_dnr_in();
            set_dac_dnr_sta(1);
        #endif
            mkey_note_onoff(0x96, 0x4A, 0x2D);
            break;
        case KU_VOL_UP:
            mkey_note_onoff(0x96, 0x4A, 0x00);
            break;
        case KLU_VOL_UP:
            mkey_note_onoff(0x96, 0x4A, 0x00);
            break;
#else
        case KU_VOL_UP:
        case KL_VOL_UP:
        case KH_VOL_UP:
#endif
#if MIDI_TEST_EN && MIDI_EN
        #if MIDI_DEC_BK_EN
            #if MIDI_OKON_EN
                midi_song_play(RES_BUF_MID_DEMO_MIDI0_MID, RES_LEN_MID_DEMO_MIDI0_MID, MIDI_MODE_OKON, 0);     //标准midi okon播放示例,先播放再set_midi_okon_next
            #else
            if((get_msc_layer_mode(0) == MSC_MP3) || (get_msc_layer_mode(0) == MSC_MI_NONE)) {
                vmidi_song_play(RES_BUF_MID_DEMO_MIDI1_VMID, RES_LEN_MID_DEMO_MIDI1_VMID, MIDI_MODE_NORM, 0);  //vmidi大循环播放示例
            }
            else{
                mp3_song_play(RES_BUF_EN_NUM127_MP3, RES_LEN_EN_NUM127_MP3, 0);
            }
            #endif
        #else
            vmidi_res_play(RES_BUF_MID_DEMO_MIDI1_VMID, RES_LEN_MID_DEMO_MIDI1_VMID);       //vmidi独占播放示例
        #endif
#else
            bsp_set_volume(bsp_volume_inc(sys_cb.vol, 1));
#endif
            break;

        case KU_VOL_DOWN:
        case KL_VOL_DOWN:
        case KH_VOL_DOWN:
#if MIDI_REC_TEST_EN
            if(get_midi_rec_state() == REC_STATE_NULL) {
                midi_rec_start();
            }
            else{
                midi_rec_stop();
            }
#endif
#if MIDI_TEST_EN && MIDI_EN
        #if MIDI_DEC_BK_EN
            midi_song_play(RES_BUF_MID_DEMO_MIDI0_MID, RES_LEN_MID_DEMO_MIDI0_MID, MIDI_MODE_NORM, 0);         //标准midi播放示例
            #else
            vmidi_res_play(RES_BUF_MID_DEMO_MIDI2_VMID, RES_LEN_MID_DEMO_MIDI2_VMID);
        #endif
#else
            bsp_set_volume(bsp_volume_dec(sys_cb.vol, 1));
#endif
            break;

        case KU_MODE:
        case KU_MODE_POWER:
        case KL_PLAY_MODE:
            func_cb.sta = FUNC_NULL;
            break;

        case KD_VOL_DOWN:
            break;

        case KU_PLAY:
        case KL_PLAY:
        case KH_PLAY:
        case KD_PLAY:
#if MSC_BREAK_P_SOFT_FADE
            mp3_song_play(RES_BUF_EN_POWERON_MP3, RES_LEN_EN_POWERON_MP3, 0);
#endif
#if MIDI_TEST_EN && (!MIDI_OKON_EN) && MIDI_EN
            midi_control(MIDI_STOP);
#endif
#if MIDI_OKON_EN && MIDI_EN
            set_midi_okon_next(1);
            // u8 next_note, next_vel, next_ch;
            // if(get_next_note_info(&next_note, &next_vel, &next_ch)) {
            //     my_printf("next note: %d, vel: %d, ch: %d\n", next_note, next_vel, next_ch);
            // } else {
            //     my_printf("no next note\n");
            // }

            if(get_next_note() != 0xFF) {
                my_printf("next note: %d\n", get_next_note());
            } else {
                my_printf("no next note\n");
            }
#endif
            break;

#if MSC_BREAK_P_SOFT_FADE
        case KU_AB_PLAY:
            wav_song_play(MSC_LAYER1, RES_BUF_EN_DNOTE_WAV, RES_LEN_EN_DNOTE_WAV, 0);
        break;
#endif

#if MUSIC_SDCARD_EN
        case EVT_SD_INSERT:
            if (dev_is_online(DEV_SDCARD)) {
                sys_cb.cur_dev = DEV_SDCARD;
                func_cb.sta = FUNC_MUSIC;
            }
            break;
#endif // MUSIC_SDCARD_EN

#if MUSIC_SDCARD1_EN
        case EVT_SD1_INSERT:
            if (dev_is_online(DEV_SDCARD1)) {
                sys_cb.cur_dev = DEV_SDCARD1;
                func_cb.sta = FUNC_MUSIC;
            }
            break;
#endif // MUSIC_SDCARD1_EN

#if MIX_PWR_DOWN_EN
        case KLH_PLAY_POWER:
            mix_pwr_down(1);
            break;
        case KLH_MODE_POWER:
            mix_pwr_down(0);
            break;
#else
        case KU_PLAY_POWER:
            // lsbc_song_play(RES_BUF_EN_SHIN_LSBC, RES_LEN_EN_SHIN_LSBC, 0);
#if MSC_MP3_LINK_EN
            mp3_song_link_init(MSC_MP3_VMP3_LINK1_NUM);
#endif
            break;
        //长按PP/POWER软关机(通过PWROFF_PRESS_TIME控制长按时间)
        case KLH_PLAY_POWER:
        case KLH_MODE_POWER:
        case KLH_HSF_POWER:
        case KLH_POWER:
            sys_cb.pwrdwn_tone_en = 1;
            func_cb.sta = FUNC_PWROFF;
            break;
#endif
        case K_NEXT:
            #if MIDI_REC_TEST_EN
            midi_rec_stop();
            midi_rec_play();
            #endif
            break;
        case K_PREV:
            #if MIDI_REC_TEST_EN
            midi_rec_play_switch();
            #endif
            break;

        case MSG_SYS_1S:
#if SYS_SLEEP_TIME      //自动休眠
            pwroff_flag++;
            if(pwroff_flag == SYS_SLEEP_TIME){
                pwroff_flag = 0;
                sys_cb.pwrdwn_tone_en = 1;
                func_cb.sta = FUNC_PWROFF;
            }
#endif
            break;

#if IRRX_SW_EN || TKEY_SLEEP_MODE
        case KU_IR_POWER:
            func_cb.sta = FUNC_SLEEPMODE;
            break;
#endif

#if UART_S_UPDATE
        case EVT_UART_UPDATE:
            if(deal_update_uart_msg()) {
                func_cb.sta = FUNC_NULL;
            }
            break;
#endif

#if MUL_PWRON_IO_EN
        case KEY_NUM_0:
            dac_fade_in(6);
            dac_fade_out(6);
            break;
        case KEY_NUM_1:
            dac_fade_in(6);
            dac_fade_out(6);
            break;
        case KEY_NUM_2:
        case KEY_NUM_3:
        case KEY_NUM_4:
        case KEY_NUM_5:
        case KEY_NUM_6:
        case KEY_NUM_7:
        case KEY_NUM_8:
        case KEY_NUM_9:
        case KEY_NUM_P100:
            break;
#endif
        default:
            break;

    }

    //调节音量，3秒后写入flash
    if ((sys_cb.cm_vol_change) && (sys_cb.cm_times >= 6)) {
        sys_cb.cm_vol_change = 0;
        cm_sync();
    }
}

///进入一个功能的总入口
AT(.text.func)
void func_enter(void)
{
#if (GUI_SELECT != GUI_NO)
    gui_box_clear();
#endif
    param_sync();
    func_cb.mp3_res_play = NULL;
    func_cb.set_vol_callback = NULL;
//    bsp_clr_mute_sta();
}

AT(.text.func)
void func_exit(void)
{
#if UART_S_UPDATE
    uart_s_update();
#endif // UART_S_UPDATE
    u8 func_num;
    u8 funcs_total = get_funcs_total();

    for (func_num = 0; func_num != funcs_total; func_num++) {
        if (func_cb.last == func_sort_table[func_num]) {
            break;
        }
    }
    func_num++;                                     //切换到下一个任务
    if (func_num >= funcs_total) {
        func_num = 0;
    }
    func_cb.sta = func_sort_table[func_num];        //新的任务
}

AT(.text.func)
void func_run(void)
{
    printf("%s\n", __func__);
    while (1) {
        func_enter();
        switch (func_cb.sta) {
#if FUNC_MUSIC_EN
        case FUNC_MUSIC:
            func_music();
            break;
#endif // FUNC_MUSIC_EN

#if EX_SPIFLASH_SUPPORT
        case FUNC_EXSPIFLASH_MUSIC:
            func_exspiflash_music();
            break;
#endif // EX_SPIFLASH_SUPPORT

#if FUNC_CLOCK_EN
        case FUNC_CLOCK:
            func_clock();
            break;
#endif // FUNC_CLOCK_EN

#if FUNC_IDLE_EN
        case FUNC_IDLE:
            func_idle();
            break;
#endif // FUNC_IDLE_EN

        case FUNC_PWROFF:
            func_pwroff(sys_cb.pwrdwn_tone_en);
            break;

        case FUNC_SLEEPMODE:
            func_sleepmode();
            break;

        default:
            func_exit();
            break;
        }
    }
}












/* 0.数组、变量初始化 */
#if 1
u8 seg_display_flag = 0; // 数码管显示标志位

u16 real_time_scores = 0;
u16 max_scores = 0;

u32 real_time_scores_reg = 0x3F3F3F3F;  // 默认显示 0000 要实时显示（需要extern给中断中用）
u32 max_scores_reg = 0x3F3F3F3F;

u8 filckering_flag = 0;  // 灯光闪烁 flag

u8 pwroff_count_flag = 0; // 计时关机 标志位
u8 pwroff_flag = 0;       // 是否关机 标志位

u8 cycle_play_flag = 0; // 上电默认开启 cycle_play_flag

u32 display_max_score_table[7] = {
    0x40760640, 0,
    0x40760640, 0,
    0x40760640, 0, 0
};

u32 seg_table[10] = {           // 0-9 数码管显示 reg
    0x3F,  // 0
    0x06,  // 1
    0x5B,  // 2
    0x4F,  // 3
    0x66,  // 4
    0x6D,  // 5
    0x7D,  // 6
    0x07,  // 7
    0x7F,  // 8
    0x6F   // 9
};
#endif

/* 0.IO 初始化 */
void gpio_init(void) {
#if 1
    /* LED 初始化 */
    GPIOBFEN &= ~BIT(1);    // PB1 复用关闭 -> 作为 普通GPIO 使用
    GPIOBDE  |=  BIT(1);    // PB1 设置为 数字 IO
    GPIOBDIR &= ~BIT(1);    // PB1 方向设置为 输出
    GPIOB    &= ~BIT(1);    // 默认输出低电平

    /* KEY1 电源开关 */
    GPIOBFEN &= ~BIT(7);
    GPIOBDE  |=  BIT(7);
    GPIOBDIR |=  BIT(7);
    GPIOBPU  |=  BIT(7);

    /* KEY2 清除实时分数 */
    GPIOBFEN &= ~BIT(6);
    GPIOBDE  |=  BIT(6);
    GPIOBDIR |=  BIT(6);
    GPIOBPU  |=  BIT(6);

    /* KEY3 BGM循环 */
    GPIOBFEN &= ~BIT(5);
    GPIOBDE  |=  BIT(5);
    GPIOBDIR |=  BIT(5);   // PB5 方向设置为 输入
    GPIOBPU  |=  BIT(5);   // PB5 内部上拉

    /* KEY4 显示最高分规则 */
    GPIOBFEN &= ~BIT(4);
    GPIOBDE  |=  BIT(4);
    GPIOBDIR |=  BIT(4);
    GPIOBPU  |=  BIT(4);

    /* KEY5 清除最高分 */
    GPIOBFEN &= ~BIT(2);
    GPIOBDE  |=  BIT(2);
    GPIOBDIR |=  BIT(2);
    GPIOBPU  |=  BIT(2);

    /* 叶子开关 实时分数增加 */
    GPIOBFEN &= ~BIT(9);
    GPIOBDE  |=  BIT(9);
    GPIOBDIR |=  BIT(9);
    GPIOBPU  |=  BIT(9);
#endif

/* 数码管初始化 */
#if 1
    // 初始化 PA5/2/1 PB0 [共阴] 输出 GND
    GPIOAFEN &= ~BIT(5); // bit1
    GPIOADE  |=  BIT(5);
    GPIOADIR &= ~BIT(5);

    GPIOAFEN &= ~BIT(2); // bit2
    GPIOADE  |=  BIT(2);
    GPIOADIR &= ~BIT(2);

    GPIOAFEN &= ~BIT(1); // bit3
    GPIOADE  |=  BIT(1);
    GPIOADIR &= ~BIT(1);

    GPIOBFEN &= ~BIT(0); // bit4
    GPIOBDE  |=  BIT(0);
    GPIOBDIR &= ~BIT(0);

    // 初始化 PA4/0/11/7/6/3/12/8 [ABCDEFG DP]
    GPIOAFEN &= ~BIT(4); // A
    GPIOADE  |=  BIT(4);
    GPIOADIR &= ~BIT(4);

    GPIOAFEN &= ~BIT(0); // B
    GPIOADE  |=  BIT(0);
    GPIOADIR &= ~BIT(0);

    GPIOAFEN &= ~BIT(11); // C
    GPIOADE  |=  BIT(11);
    GPIOADIR &= ~BIT(11);

    GPIOAFEN &= ~BIT(7); // D
    GPIOADE  |=  BIT(7);
    GPIOADIR &= ~BIT(7);

    GPIOAFEN &= ~BIT(6); // E
    GPIOADE  |=  BIT(6);
    GPIOADIR &= ~BIT(6);

    GPIOAFEN &= ~BIT(3); // F
    GPIOADE  |=  BIT(3);
    GPIOADIR &= ~BIT(3);

    GPIOAFEN &= ~BIT(12); // G
    GPIOADE  |=  BIT(12);
    GPIOADIR &= ~BIT(12);

    GPIOAFEN &= ~BIT(8); // DP
    GPIOADE  |=  BIT(8);
    GPIOADIR &= ~BIT(8);

    // 设置默认 IO 默认输出状态（不亮）
    GPIOA |= BIT(5);
    GPIOA |= BIT(2);
    GPIOA |= BIT(1);
    GPIOB |= BIT(0);

    GPIOA &= ~BIT(4);    // A
    GPIOA &= ~BIT(0);    // B
    GPIOA &= ~BIT(11);   // C
    GPIOA &= ~BIT(7);    // D
    GPIOA &= ~BIT(6);    // E
    GPIOA &= ~BIT(3);    // F
    GPIOA &= ~BIT(12);   // G
    GPIOA &= ~BIT(8);    // DP
#endif

/* 取出脏数据 */
#if 1
    msg_queue_detach(0x0A00, 0);
    msg_queue_detach(0x0B00, 0);
    msg_queue_detach(0x0C00, 0);
    msg_queue_detach(0x0D00, 0);
    msg_queue_detach(0x0E00, 0);
    msg_queue_detach(0x0F00, 0);
#endif
}

/* 0.通用 API */
// 0.1 数字转数码管显示寄存器
u32 num_to_digit(u16 num) {

    // 1.获取 千百十个 各位
    u8 thousand = num / 1000;
    u8 hundred  = (num % 1000) / 100;
    u8 ten      = (num % 100) / 10;
    u8 unit     = num % 10;

    // 2.获取 千百十个 各位的数码管显示值

    u8 seg_thousand = seg_table[thousand];
    u8 seg_hundred  = seg_table[hundred];
    u8 seg_ten      = seg_table[ten];
    u8 seg_unit     = seg_table[unit];

    // 拼接起来
    u32 display_reg = ((u32)seg_thousand << 24) |
                      ((u32)seg_hundred  << 16) |
                      ((u32)seg_ten      << 8)  |
                      ((u32)seg_unit);

    return display_reg;
}

// 0.2 灯光闪三下
void lamp_flickering_3(void) {
    GPIOB |= BIT(1);
    delay_ms(400);
    GPIOB &= ~BIT(1);
    delay_ms(400);
    GPIOB |= BIT(1);
    delay_ms(400);
    GPIOB &= ~BIT(1);
    delay_ms(400);
    GPIOB |= BIT(1);
    delay_ms(400);
    GPIOB &= ~BIT(1);
    delay_ms(400);
}

/* 主循环有一个灯光（这边控制 灯光闪烁标志位）*/
// 0.3 灯光 开始 闪烁
void lamp_flickering_start(void) {
    filckering_flag = 1;
}

// 0.4 灯光 关闭 闪烁
void lamp_flickering_stop(void) {
    filckering_flag = 0;
}

// 0.5 灯光闪烁
void lamp_filckering(void) {

    // 如果太快就用累加的方式
    if (filckering_flag) {
        static u8 reverse_count_flag = 0;

        reverse_count_flag++;

        if (reverse_count_flag == 100) {
            GPIOB |= BIT(1);
        }
        else if (reverse_count_flag == 200) {
            GPIOB &= ~BIT(1);
            reverse_count_flag = 0;
        }
    }
    else {
        GPIOB &= ~BIT(1);
    }
}

// 0.6 设备初始化
void device_init(void) {
    lamp_flickering_3();      // 灯闪三下
    sfunc_pwroff();         // 关机
    cycle_play_flag = 1;    // BGM开
    seg_display_flag = 1;   // 数码管开
}

/* 1.自动关机处理函数 */
void auto_shutdown_process(void) {

    u16 msg = msg_dequeue();

    /* 检测是否空闲 */
    if((msg == 0x0A00) || (msg == 0x0B00) || (msg == 0x0C00) || (msg == 0x0D00) || (msg == 0x0D11) || (msg == 0x0E00) || (msg == 0x0F00)) {
        pwroff_count_flag = 0;
        msg_enqueue(msg);
        return;
    }

    if (msg == MSG_SYS_1S) {
        pwroff_count_flag++;
        /* 几秒关闭 */
        if (pwroff_count_flag >= 180) {
            pwroff_count_flag = 0;
            // 1.播放提示音（叠加）
            sfunc_pwroff();
        }
        return;
    }

    msg_enqueue(msg);
}

/* 2.开关机 按键处理函数 msg 0x0A00 */
void power_process(void) {

    static u8 off_music_play = 2;

    u16 msg = msg_dequeue();
    if (msg == 0x0A00) {
        printf("key1 press!\n");
        // 1.播放提示音关机（叠加）
        pwroff_flag = !pwroff_flag;

        if (!pwroff_flag) {
            /* 休眠前处理 */
            music_layer_sta_set(MSC_LAYER1, LAYER_PAUSE); // 暂停音乐，本层原有的音乐
            music_layer_sta_set(MSC_LAYER1, LAYER_STOP);  // 删除音乐，本层原有的音乐
            wav_song_play(MSC_LAYER1, RES_BUF_EN_OFF_WAV, RES_LEN_EN_OFF_WAV, 1);

            off_music_play = 1;

            // 灯光熄灭
            lamp_flickering_stop();
        }
    }
    else {
        msg_enqueue(msg);
    }

    if (off_music_play == 1) {
        if (music_layer_sta_get(MSC_LAYER1) != LAYER_PLAYING) {
            off_music_play = 0;
        }
    }
    else if (off_music_play == 0){
        sfunc_pwroff();
        off_music_play = 2;

        /* 唤醒后处理 */
        // 2.播放提示音开机（叠加）
        music_layer_sta_set(MSC_LAYER1, LAYER_PAUSE); // 暂停音乐，本层原有的音乐
        music_layer_sta_set(MSC_LAYER1, LAYER_STOP); // 删除音乐，本层原有的音乐
        wav_song_play(MSC_LAYER1, RES_BUF_EN_CLR_WAV, RES_LEN_EN_CLR_WAV, 1);

        // // 3.数码管（最高分显示规则）
        // // 3.1 数码管（按照最高分显示规则播放）准备数据
        // // 数组，往里面放好 -H1-，至于最高分，在播放的时候实时获取后赋值到数组里面，到时候数码管就按照这个数组进行显示
        display_max_score_table[1] = max_scores_reg;
        display_max_score_table[3] = max_scores_reg;
        display_max_score_table[5] = max_scores_reg;
        display_max_score_table[6] = real_time_scores_reg;

        // 3.2 0x0D11 表示数据准备好，数码管可以开始显示最高分规则
        msg_enqueue(0x0D11);

        // 4.灯光闪3下
        lamp_flickering_3();
    }

}

/* 3.清空实时分数 按键处理函数 msg 0x0B00 */
void clear_score_process(void) {
    u16 msg = msg_dequeue();
    if (msg == 0x0B00) {
        printf("key2 press!\n");

        // 1.播放提示音（叠加）
        music_layer_sta_set(MSC_LAYER1, LAYER_STOP); // 删除音乐，本层原有的音乐
        wav_song_play(MSC_LAYER1, RES_BUF_EN_CLR_WAV, RES_LEN_EN_CLR_WAV, 1);

        // 2.清除实时分数
        real_time_scores = 0;
        real_time_scores_reg = num_to_digit(real_time_scores);

        // 3.1 数码管（按照最高分显示规则播放）准备数据
        display_max_score_table[1] = max_scores_reg;
        display_max_score_table[3] = max_scores_reg;
        display_max_score_table[5] = max_scores_reg;
        display_max_score_table[6] = real_time_scores_reg;

        // 3.2 0x0D11 表示数据准备好，数码管可以开始显示最高分规则
        msg_enqueue(0x0D11);

        // 4.灯光熄灭（打开和关闭 灯光闪烁API）
        lamp_flickering_stop();
    }
    else {
        msg_enqueue(msg);
    }
}

/* 4.BGM循环 按键处理函数 msg 0x0C00 */
#if 1
// 全局变量


// 声明
void cycle_play(void);

void loop_bgm_process(void) {
    u16 msg = msg_dequeue();
    if (msg == 0x0C00) {
        printf("key3 press!\n");
        cycle_play_flag = !cycle_play_flag;
    }
    else {
        msg_enqueue(msg);
    }

    if (cycle_play_flag) {
        cycle_play();
    } else {
        music_layer_sta_set(MSC_LAYER0, LAYER_PAUSE);
    }
}

void cycle_play(void) {
    layer_sta_e play_sta = music_layer_sta_get(MSC_LAYER0);
    if (play_sta == LAYER_PLAYING) {
        return;
    }

    mp3_song_play(RES_BUF_EN_BGM_MP3, RES_LEN_EN_BGM_MP3, 0);
}


#endif

/* 5.显示最高分 按键处理函数 msg 0x0D00 */
void show_high_score_process(void) {
    u16 msg = msg_dequeue();
    if (msg == 0x0D00) {
        printf("key4 press!\n");

        // 1.播放提示音（叠加）
        music_layer_sta_set(MSC_LAYER1, LAYER_STOP); // 删除音乐，本层原有的音乐
        wav_song_play(MSC_LAYER1, RES_BUF_EN_HIGH_WAV, RES_LEN_EN_HIGH_WAV, 1);

        // 2.1 数码管（按照最高分显示规则播放）准备数据
        // 数组，往里面放好 -H1-，至于最高分，在播放的时候实时获取后赋值到数组里面，到时候数码管就按照这个数组进行显示
        max_scores_reg = num_to_digit(max_scores);
        real_time_scores_reg = num_to_digit(real_time_scores);

        display_max_score_table[1] = max_scores_reg;
        display_max_score_table[3] = max_scores_reg;
        display_max_score_table[5] = max_scores_reg;
        display_max_score_table[6] = real_time_scores_reg;

        // 2.2 0x0D11 表示数据准备好，数码管可以开始显示最高分规则
        msg_enqueue(0x0D11);


        // 3.灯光熄灭（打开和关闭 灯光闪烁API）
        lamp_flickering_stop();
    }
    else {
        msg_enqueue(msg);
    }
}

/* 6.清空最高分 按键处理函数 msg 0x0E00 */
void clear_high_score_process(void) {
    u16 msg = msg_dequeue();
    if (msg == 0x0E00) {
        printf("key5 press!\n");

/* 原 KEY5 的功能 */
#if 1
        // 1.播放提示音（叠加）
        music_layer_sta_set(MSC_LAYER1, LAYER_STOP); // 删除音乐，本层原有的音乐
        wav_song_play(MSC_LAYER1, RES_BUF_EN_CLR_WAV, RES_LEN_EN_CLR_WAV, 1);

        // 2.清除实时分数
        real_time_scores = 0;
        real_time_scores_reg = num_to_digit(real_time_scores);

        // 3.清除最高分
        max_scores = 0;
        max_scores_reg = num_to_digit(max_scores);

        // 4.1 数码管（按照最高分显示规则播放）准备数据
        display_max_score_table[1] = max_scores_reg;
        display_max_score_table[3] = max_scores_reg;
        display_max_score_table[5] = max_scores_reg;
        display_max_score_table[6] = real_time_scores_reg;

        // 4.2 0x0D11 表示数据准备好，数码管可以开始显示最高分规则
        msg_enqueue(0x0D11);


        // 5.灯光熄灭（打开和关闭 灯光闪烁API）
        lamp_flickering_stop();
#endif
    }
    else {
        msg_enqueue(msg);
    }
}

/* 7.叶子 按键处理函数 msg 0x0F00 */
#if 1
u32 last_times = 0;
// 声明函数
void leaf_play(void);

void leaf_process(void) {
    static u8 leaf_music_play = 2;

    u16 msg = msg_dequeue();
    if (msg == 0x0F00) {
        // 如果在300ms 后触发
        if (tick_check_expire(last_times, 300)) {
            // 1.实时分数加 5
            printf("leaf key press!\n");

            last_times = tick_get();

            real_time_scores += 5;
            if (real_time_scores > 9995) {
                real_time_scores = 0;
            }

            if (real_time_scores > max_scores) {
                max_scores = real_time_scores;
            }

            real_time_scores_reg = num_to_digit(real_time_scores);

            // 3.1 灯光有声赫兹闪（开始）
            lamp_flickering_start();

            // 4.每满100分，播放欢呼声
            // if (100 就播放 欢呼，如果不是 100 就正常播放)
            if (real_time_scores % 100 == 0) {
                wav_song_play(MSC_LAYER1, RES_BUF_EN_WIN_WAV, RES_LEN_EN_WIN_WAV, 1);
            }
            else {
                // 2.播放 叶子 提示音
                leaf_play();
            }
            leaf_music_play = 1;
        }
        else {
            printf("re-trigger at the short times!\n");
        }
    }
    else {
        msg_enqueue(msg);
    }

    // 3.1 灯光有声赫兹闪（结束）
    if (leaf_music_play == 1) {
        if (music_layer_sta_get(MSC_LAYER1) != LAYER_PLAYING) {
            leaf_music_play = 0;
        }
    }
    else if (leaf_music_play == 0) {
        lamp_flickering_stop();
    }
}

void leaf_play(void) {
    static u8 current_audio = 1;

    switch(current_audio) {
        case 1:
            wav_song_play(MSC_LAYER1, RES_BUF_EN_SFX1_WAV, RES_LEN_EN_SFX1_WAV, 1);
            break;
        case 2:
            wav_song_play(MSC_LAYER1, RES_BUF_EN_SFX2_WAV, RES_LEN_EN_SFX2_WAV, 1);
            break;
        case 3:
            wav_song_play(MSC_LAYER1, RES_BUF_EN_SFX3_WAV, RES_LEN_EN_SFX3_WAV, 1);
            break;
        case 4:
            wav_song_play(MSC_LAYER1, RES_BUF_EN_SFX4_WAV, RES_LEN_EN_SFX4_WAV, 1);
            break;
        case 5:
            wav_song_play(MSC_LAYER1, RES_BUF_EN_SFX5_WAV, RES_LEN_EN_SFX5_WAV, 1);
            break;
    }

    // 索引递增，到5后回到1
    current_audio++;
    if(current_audio > 5) {
        current_audio = 1;
    }

}

#endif

void user_main(void) {

    /* 初始化 */
    /* 0.1 IO 初始化 */
    gpio_init();

    /* 0.2 设备初始化：灯闪、关机、BGM开、数码管开 */
    device_init();

    /* 本项目所有处理函数 */
    while (1) {
        func_process(); // 系统自带的process

        /* 1.自动关机处理函数 */
        auto_shutdown_process();

        /* 2.开关机 按键处理函数 msg 0x0A00 */
        power_process();

        /* 3.清除分数 按键处理函数 msg 0x0B00 */
        clear_score_process();

        /* 4.BGM循环 按键处理函数 msg 0x0C00 */
        loop_bgm_process();

        /* 5.显示最高分规则 按键处理函数 msg 0x0D00 */
        show_high_score_process();

        /* 6.清除最高分 按键处理函数 msg 0x0E00 */
        clear_high_score_process();

        /* 7.叶子 按键处理函数 msg 0x00F0 */
        leaf_process();

        /* 灯光闪烁 */
        lamp_filckering();
    }
}

