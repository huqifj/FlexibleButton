/**
 * @File:    flexible_button.c
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
 *                          2.新增变量 release_max_cnt，庙祠按键释放后计时，在此变量时间内
 *                            再次按键判定为连击
 *                          3.新增宏 FLEX_BTN_SCAN_HZ，标时调用 flex_button_scan 的频率
 *                            用户填写其他时间时可以以ms为单位，如 release_max_cnt = 400
 * 2018-09-29  MurphyZhao   First add
 * 2019-08-02  MurphyZhao   迁移代码到 murphyzhao 仓库
 *
 */

#include "flexible_button.h"
#include <stdio.h>
#include <string.h>

// 设置指定按键的事件，并运行相关回调函数
#define SET_EVENT_AND_RUN_CB(btn, evt)                                                                                                                         \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        btn->event = evt;                                                                                                                                      \
        if (btn->cb)                                                                                                                                           \
        {                                                                                                                                                      \
            btn->cb((flex_button_t *)btn);                                                                                                                     \
        }                                                                                                                                                      \
    } while (0);

#define MAX_BUTTON_CNT 32

static flex_button_t *btn_head = NULL;
static uint8_t button_cnt = 0;
static uint32_t btn_data_cur;
static uint32_t btn_data_pre;
static uint32_t btn_toggle; // 按键是否发生翻转

/**
 * @brief Register a user button
 *
 * @param button: button structure instance
 * @return Number of btns that have been registered
 */
int8_t flex_button_register(flex_button_t *button)
{
    flex_button_t *curr = btn_head;

    if (!button || (button_cnt > MAX_BUTTON_CNT))
    {
        return -1;
    }

    while (curr)
    {
        if (curr == button)
        {
            return -1; // already exist.
        }
        curr = curr->next;
    }

    button->next = btn_head;
    button->status = 0;
    button->event = FLEX_BTN_PRESS_NONE;
    button->scan_cnt = 0;
    button->click_cnt = 0;
    btn_head = button;
    button_cnt++;
    return button_cnt;
}

/**
 * @brief Read all btn values in one scan cycle
 *
 * @param void
 * @return none
 */
static void flex_button_read(void)
{
    flex_button_t *target;
    btn_data_cur = 0;
    uint8_t i = 0;

    // btn_data_cur ，按下按键相应位清零（后注册的按键在低位），允许多个键同时按下
    for (target = btn_head, i = 0; (target != NULL) && (target->usr_button_read != NULL); target = target->next, i++)
    {
        btn_data_cur = btn_data_cur | (target->pressed_logic_level ? (target->usr_button_read() << i) : (!(target->usr_button_read()) << i));
    }

    btn_toggle = btn_data_cur ^ btn_data_pre;
    btn_data_pre = btn_data_cur;
}

/**
 * @brief Handle all btn events in one scan cycle.
 *        Must be used after 'flex_button_read' API
 *
 * @param void
 * @return none
 */
static void flex_button_process(void)
{
    uint8_t i = 0;
    flex_button_t *target;

#define BTN_TOGGLE(i) (btn_toggle & (1 << i))

    for (target = btn_head, i = 0; target != NULL; target = target->next, i++)
    {
        if (target->status > 0)
        {
            target->scan_cnt++;
        }

        switch (target->status)
        {
        case 0: // 检测是否按下。（初始：释放）
            if (BTN_TOGGLE(i))
            {
                target->scan_cnt = 0;
                target->click_cnt = 0;
                // SET_EVENT_AND_RUN_CB(target, kVicKeyPress);
                target->status = 1;
            }
            else
            {
                target->event = FLEX_BTN_PRESS_NONE;
            }
            break;
        case 1: // 检测是否长按。（初始：按下）
            if (BTN_TOGGLE(i))
            {
                if (target->scan_cnt <= FLEX_MS_TO_CNT(target->short_press_start_tick))
                {
                    target->status = 2; // 未达到短按，则跳转检测是否连击
                    target->scan_cnt = 0;
                }
                else
                {
                    if (target->scan_cnt > FLEX_MS_TO_CNT(target->long_hold_start_tick))
                    { // 达到超长按
                        SET_EVENT_AND_RUN_CB(target, FLEX_BTN_PRESS_LONG_HOLD_UP);
                    }
                    else if (target->scan_cnt > FLEX_MS_TO_CNT(target->long_press_start_tick))
                    { // 达到长按
                        SET_EVENT_AND_RUN_CB(target, FLEX_BTN_PRESS_LONG_UP);
                    }
                    else if (target->scan_cnt > FLEX_MS_TO_CNT(target->short_press_start_tick))
                    { // 达到短按
                        SET_EVENT_AND_RUN_CB(target, FLEX_BTN_PRESS_SHORT_UP);
                    }
                    target->status = 0; // 已检测到短按/长按/超长按，释放跳到初始状态
                }
            }
            else
            {
                if (target->scan_cnt > FLEX_MS_TO_CNT(target->long_hold_start_tick))
                { // 达到超长按
                    if (target->event != FLEX_BTN_PRESS_LONG_HOLD)
                    {
                        SET_EVENT_AND_RUN_CB(target, FLEX_BTN_PRESS_LONG_HOLD);
                    }
                }
                else if (target->scan_cnt > FLEX_MS_TO_CNT(target->long_press_start_tick))
                { // 达到长按
                    if (target->event != FLEX_BTN_PRESS_LONG_START)
                    {
                        SET_EVENT_AND_RUN_CB(target, FLEX_BTN_PRESS_LONG_START);
                    }
                }
                else if (target->scan_cnt > FLEX_MS_TO_CNT(target->short_press_start_tick))
                { // 达到短按
                    if (target->event != FLEX_BTN_PRESS_SHORT_START)
                    {
                        SET_EVENT_AND_RUN_CB(target, FLEX_BTN_PRESS_SHORT_START);
                    }
                }
            }
            break;
        case 2: // 检测单击。（初始：释放）
            if (target->scan_cnt > FLEX_MS_TO_CNT(target->release_max_cnt))
            { // 超过释放时间没有按下
                if (target->click_cnt <= (FLEX_BTN_PRESS_LEGENDARY - FLEX_BTN_PRESS_CLICK))
                { // 低于最大连击次数
                    SET_EVENT_AND_RUN_CB(target, FLEX_BTN_PRESS_CLICK + target->click_cnt);
                }
                target->status = 0;
            }
            else if (BTN_TOGGLE(i))
            {                        // 限制时间内按下
                target->click_cnt++; // 连击+1
                target->scan_cnt = 0;
                target->status = 1; // 跳转状态1继续检测
            }
            break;
        }
    }
}

/**
 * flex_button_event_read
 *
 * @brief Get the button event of the specified button.
 *
 * @param button: button structure instance
 * @return button event
 */
flex_button_event_t flex_button_event_read(flex_button_t *button) { return (flex_button_event_t)(button->event); }

/**
 * flex_button_scan
 *
 * @brief Start btn scan.
 *        Need to be called cyclically within the specified period.
 *        Sample cycle: 5 - 20ms
 *
 * @param void
 * @return none
 */
void flex_button_scan(void)
{
    flex_button_read();
    flex_button_process();
}
