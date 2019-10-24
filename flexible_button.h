/**
 * @File:    flexible_button.h
 * @Author:  MurphyZhao
 * @Date:    2018-09-29
 *
 * Copyright (c) 2018-2019 MurphyZhao <d2014zjt@163.com>
 *               https://github.com/murphyzhao
 * All rights reserved.
 * License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Change logs:
 * Date        Author       Notes
 * 2019-10-24  huqifan      1.重写按键检测，现在可以检测任意多次连击
 *                           （只定义了8连击的枚举，超过8连击需要添加枚举）
 *                          2.新增变量 release_max_cnt，每次按键释放后计时，在此变量时间内
 *                            再次按键判定为连击
 *                          3.新增宏 FLEX_BTN_SCAN_HZ，标时调用 flex_button_scan 的频率
 *                            用户填写其他时间时可以以ms为单位，如 release_max_cnt = 400
 * 2018-09-29  MurphyZhao   First add
 * 2019-08-02  MurphyZhao   迁移代码到 murphyzhao 仓库
 *
 */

#ifndef __FLEXIBLE_BUTTON_H__
#define __FLEXIBLE_BUTTON_H__

#include "stdint.h"
#include "string.h"

#define FLEX_BTN_SCAN_HZ 50 // 调用 flex_button_scan() 函数的频率
#define FLEX_MS_TO_CNT(ms) (ms / (1000 / FLEX_BTN_SCAN_HZ))

typedef void (*flex_button_response_callback)(void *);

typedef enum
{
    FLEX_BTN_PRESS_DOWN = 0,
    FLEX_BTN_PRESS_CLICK,
    FLEX_BTN_PRESS_DOUBLE_CLICK,
    FLEX_BTN_PRESS_TRIPLE_CLICK,
    FLEX_BTN_PRESS_QUADRA_CLICK,
    FLEX_BTN_PRESS_PENTA_CLICK,
    FLEX_BTN_PRESS_HEXA_CLICK,
    FLEX_BTN_PRESS_GODLIKE_CLICK,
    FLEX_BTN_PRESS_LEGENDARY,
    FLEX_BTN_PRESS_SHORT_START,
    FLEX_BTN_PRESS_SHORT_UP,
    FLEX_BTN_PRESS_LONG_START,
    FLEX_BTN_PRESS_LONG_UP,
    FLEX_BTN_PRESS_LONG_HOLD,
    FLEX_BTN_PRESS_LONG_HOLD_UP,
    FLEX_BTN_PRESS_MAX,
    FLEX_BTN_PRESS_NONE,
} flex_button_event_t;

/**
 * flex_button_t
 *
 * @brief Button data structure
 *        Below are members that need to user init before scan.
 *
 * @member pressed_logic_level:    Logic level when the button is pressed.
 *                                 Must be inited by 'flex_button_register' API
 *                                                     before start button scan.
 * @member debounce_tick:          The time of button debounce.
 *                                 The value is number of button scan cycles.
 * @member click_start_tick:       The time of start click.
 *                                 The value is number of button scan cycles.
 * @member short_press_start_tick: The time of short press start tick.
 *                                 The value is number of button scan cycles.
 * @member long_press_start_tick:  The time of long press start tick.
 *                                 The value is number of button scan cycles.
 * @member long_hold_start_tick:   The time of hold press start tick.
 *                                 The value is number of button scan cycles.
 * @member usr_button_read:        Read the logic level value of specified
 * button.
 * @member cb:                     Button event callback function.
 *                                 If use 'flex_button_event_read' api,
 *                                 you don't need to initialize the 'cb' member.
 * @member next :                  Next button struct
 */
typedef struct flex_button
{
    uint8_t pressed_logic_level : 1; /* need user to init */

    /**
     * @event
     * The event of button in flex_button_evnt_t enum list.
     * Automatically initialized to the default value FLEX_BTN_PRESS_NONE
     *                                      by 'flex_button_register' API.
     */
    uint8_t event : 5;

    /**
     * @status
     * Used to record the status of the button
     * Automatically initialized to the default value 0.
     */
    uint8_t status : 2;
    uint16_t scan_cnt;  /* default 0. Used to record the number of key scans */
    uint16_t click_cnt; /* default 0. Used to record the number of key click */

    uint16_t release_max_cnt; // 最大连击释放间隔计数

    uint16_t debounce_tick;
    uint16_t click_start_tick;
    uint16_t short_press_start_tick;
    uint16_t long_press_start_tick;
    uint16_t long_hold_start_tick;

    uint8_t (*usr_button_read)(void);
    flex_button_response_callback cb;
    struct flex_button *next;
} flex_button_t;

#ifdef __cplusplus
extern "C"
{
#endif

    int8_t flex_button_register(flex_button_t *button);
    flex_button_event_t flex_button_event_read(flex_button_t *button);
    void flex_button_scan(void);

#ifdef __cplusplus
}
#endif
#endif /* __FLEXIBLE_BUTTON_H__ */
