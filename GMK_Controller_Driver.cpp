// GMK_Controller_Driver.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "GMK_Controller_Driver.h"

#define DEBUG 1

// HID Class-Specific Requests values. See section 7.2 of the HID specifications
#define HID_GET_REPORT                  0x01
#define HID_GET_IDLE                    0x02
#define HID_GET_PROTOCOL                0x03
#define HID_SET_REPORT                  0x09
#define HID_SET_IDLE                    0x0A
#define HID_SET_PROTOCOL                0x0B
#define HID_REPORT_TYPE_INPUT           0x01
#define HID_REPORT_TYPE_OUTPUT          0x02
#define HID_REPORT_TYPE_FEATURE         0x03

#define GMK_ENDPOINT_IN                 0x81
#define GMK_ENDPOINT_OUT                0x01

uint16_t vid = 0x0483;
uint16_t pid = 0x5750;

static libusb_device_handle* gmk_handle = NULL;

static PVIGEM_CLIENT vigem_client = NULL;

void display_controller_data(Controller_Data_TypeDef* c) 
{
    HANDLE std_out_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD position = { 0, 2 };
    SetConsoleCursorPosition(std_out_handle, position);

    printf("Controller Data:\n");

    printf("Buttons\n");
    printf(" A:\t%u\n", c->buttons.a);
    printf(" B:\t%u\n", c->buttons.b);
    printf(" X:\t%u\n", c->buttons.x);
    printf(" Y:\t%u\n", c->buttons.y);
    printf(" Lth:\t%u\n", c->buttons.lth);
    printf(" Rth:\t%u\n", c->buttons.rth);
    printf(" Lb:\t%u\n", c->buttons.lb);
    printf(" Rb:\t%u\n", c->buttons.rb);
    printf(" D-Pad, Left:\t%u\n", c->buttons.left);
    printf(" D-Pad, Right:\t%u\n", c->buttons.right);
    printf(" D-Pad, Up:\t%u\n", c->buttons.up);
    printf(" D-Pad, Down:\t%u\n", c->buttons.down);

    printf("Joysticks\n");
    printf(" Left,  X: % f\tY:% f\n", (float)c->joysticks.left.x / (float)UINT16_MAX, (float)c->joysticks.left.y / (float)UINT16_MAX);
    printf(" Right, X: % f\tY:% f\n", (float)c->joysticks.right.x / (float)UINT16_MAX, (float)c->joysticks.right.y / (float)UINT16_MAX);
    
    printf("Triggers\n");
    printf(" Left:  % 3u\n", c->triggers.left);
    printf(" Right: % 3u\n", c->triggers.right);
}

void map_controller_data(HID_Report_In_TypeDef* in, Controller_Data_TypeDef* out)
{
    out->buttons._bits = in->buttons[1] << 8 | in->buttons[0];
    out->joysticks._bits[0] = in->joysticks[1] << 8 | in->joysticks[0];
    out->joysticks._bits[1] = in->joysticks[3] << 8 | in->joysticks[2];
    out->joysticks._bits[2] = in->joysticks[5] << 8 | in->joysticks[4];
    out->joysticks._bits[3] = in->joysticks[7] << 8 | in->joysticks[6];
    out->triggers._bits[0] = in->triggers[0];
    out->triggers._bits[1] = in->triggers[1];
}

void map_controller_data_to_xinput(Controller_Data_TypeDef* in, XINPUT_STATE* out)
{
    out->Gamepad.wButtons = 0x0000;
    out->Gamepad.wButtons |= in->buttons.a ? XINPUT_GAMEPAD_A : 0x00;
    out->Gamepad.wButtons |= in->buttons.b ? XINPUT_GAMEPAD_B : 0x00;
    out->Gamepad.wButtons |= in->buttons.x ? XINPUT_GAMEPAD_X : 0x00;
    out->Gamepad.wButtons |= in->buttons.y ? XINPUT_GAMEPAD_Y : 0x00;
    out->Gamepad.wButtons |= in->buttons.rb ? XINPUT_GAMEPAD_RIGHT_SHOULDER : 0x00;
    out->Gamepad.wButtons |= in->buttons.lb ? XINPUT_GAMEPAD_LEFT_SHOULDER : 0x00;
    out->Gamepad.wButtons |= in->buttons.rth ? XINPUT_GAMEPAD_RIGHT_THUMB : 0x00;
    out->Gamepad.wButtons |= in->buttons.lth ? XINPUT_GAMEPAD_LEFT_THUMB : 0x00;
    out->Gamepad.wButtons |= in->buttons.back ? XINPUT_GAMEPAD_BACK : 0x00;
    out->Gamepad.wButtons |= in->buttons.start ? XINPUT_GAMEPAD_START : 0x00;
    out->Gamepad.wButtons |= in->buttons.right ? XINPUT_GAMEPAD_DPAD_RIGHT : 0x00;
    out->Gamepad.wButtons |= in->buttons.left ? XINPUT_GAMEPAD_DPAD_LEFT : 0x00;
    out->Gamepad.wButtons |= in->buttons.down ? XINPUT_GAMEPAD_DPAD_DOWN : 0x00;
    out->Gamepad.wButtons |= in->buttons.up ? XINPUT_GAMEPAD_DPAD_UP : 0x00;

    out->Gamepad.bLeftTrigger = in->triggers.left;
    out->Gamepad.bRightTrigger = in->triggers.right;
    out->Gamepad.sThumbLX = in->joysticks.left.x;
    out->Gamepad.sThumbLY = -1*in->joysticks.left.y;
    out->Gamepad.sThumbRX = in->joysticks.right.x;
    out->Gamepad.sThumbRY = in->joysticks.right.y;
}

libusb_error loop_input() 
{
    libusb_error ret = LIBUSB_SUCCESS;
    VIGEM_ERROR vigem_error = VIGEM_ERROR_NONE;

    HID_Report_In_TypeDef hid_report_in;
    Controller_Data_TypeDef controller_data;

    XINPUT_STATE gamepad_state;
    
    PVIGEM_TARGET pad = vigem_target_x360_alloc();
    vigem_error = vigem_target_add(vigem_client, pad);

    int len = 0;
    while (ret == LIBUSB_SUCCESS && VIGEM_SUCCESS(vigem_error))
    {
        ret = (libusb_error)libusb_interrupt_transfer(gmk_handle, GMK_ENDPOINT_IN, (uint8_t*)&hid_report_in, sizeof(hid_report_in), &len, 0);
        map_controller_data(&hid_report_in, &controller_data);
        map_controller_data_to_xinput(&controller_data, &gamepad_state);
        if(DEBUG)
            display_controller_data(&controller_data);
        vigem_error = vigem_target_x360_update(vigem_client, pad, *reinterpret_cast<XUSB_REPORT*>(&gamepad_state.Gamepad));
    }

    // Free ViGEm resources
    vigem_target_remove(vigem_client, pad);
    vigem_target_free(pad);

    return ret;
}

libusb_error initialize_device() 
{
    libusb_error ret;
    
    ret = (libusb_error)libusb_init(NULL); // Init libusb
    if (ret != LIBUSB_SUCCESS)
    {
        printf("Unable to initialize libusb.\n");
        return ret;
    }


    gmk_handle = libusb_open_device_with_vid_pid(NULL, vid, pid);
    if (gmk_handle == NULL)
    {
        printf("Unable to open GMK Controller\n VID=%x\tPID=%x\n", vid, pid);
        return LIBUSB_ERROR_NOT_FOUND;
    }

    int config;
    libusb_get_configuration(gmk_handle, &config);

    ret = (libusb_error)libusb_set_configuration(gmk_handle, config);
    if (ret != LIBUSB_SUCCESS)
    {
        printf("Could not set GMK Controller config.\n");
        return ret;
    }

    ret = (libusb_error)libusb_claim_interface(gmk_handle, 0);

    if (ret != LIBUSB_SUCCESS)
    {
        printf("Unable to claim interface.\n");
        return ret;
    }

    printf("Successfully opened GMK Controller!\n");
    return ret;
}

VIGEM_ERROR inititalize_vigem()
{
    VIGEM_ERROR vigem_error = VIGEM_ERROR_NOT_SUPPORTED;

    vigem_client = vigem_alloc();
    if (vigem_client == nullptr)
    {
        printf("Not enough memory?\n");
        return vigem_error;
    }

    vigem_error = vigem_connect(vigem_client);
    if (!VIGEM_SUCCESS(vigem_error))
    {
        printf("Unable to find ViGEmBus device driver.\n");
        return vigem_error;
    }

    return vigem_error;
}

int main()
{
    bool run = true;

    while (run)
    {
        libusb_error usb_error = initialize_device();

        if (usb_error != LIBUSB_SUCCESS)
        {
            printf(" Error: %s (%i)\n", libusb_strerror(usb_error), usb_error);
            return -1;
        }

        VIGEM_ERROR vigem_error = inititalize_vigem();

        if (!VIGEM_SUCCESS(vigem_error))
        {
            printf("Error: %i", vigem_error);
            return -1;
        }
        else
        {
            usb_error = loop_input();
        }

        if (usb_error != LIBUSB_SUCCESS)
        {
            printf(" Error: %s (%i)\n", libusb_strerror(usb_error), usb_error);
            return -1;
        }

        if (gmk_handle != NULL)
        {
            libusb_release_interface(gmk_handle, 0);
            libusb_close(gmk_handle);
        }

        vigem_disconnect(vigem_client);
        vigem_free(vigem_client);

        libusb_exit(NULL);
        return usb_error;
    }
}
