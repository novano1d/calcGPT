#include <srldrvce.h>
#include <debug.h>
#include <keypadc.h>
#include <stdbool.h>
#include <string.h>
#include <tice.h>
#include <ti/screen.h>
#include <ti/getcsc.h>
#include <stdlib.h>

#define BUFFER_SIZE 1300 //1300 allows for 5 pages
#define BLOCK_SIZE 234 //260 characters fit on screen but last line was being weird

char buffer[BUFFER_SIZE];
int write_index = 0;
int read_index = 0;
int last_block_written_to = -1;
int scroll_pos = 0;
bool firstread = true;

//Allman bracket superiority 
void append_to_buffer(char *data)
{
    int data_length = strlen(data);
    int space_left = BUFFER_SIZE - write_index;

    if (data_length > space_left) {
        // Not enough space at the end of the buffer, wrap around
        memcpy(&buffer[write_index], data, space_left);
        memcpy(buffer, &data[space_left], data_length - space_left);
        write_index = data_length - space_left;
    } else {
        // Enough space at the end of the buffer
        memcpy(&buffer[write_index], data, data_length);
        write_index += data_length;
    }

    // Make sure write_index stays within buffer bounds
    if (write_index >= BUFFER_SIZE) {
        write_index = 0;
    }

    last_block_written_to = (write_index - 1) / BLOCK_SIZE;
}

char *get_block(int block_number)
{
    int block_start = block_number * BLOCK_SIZE;
    int space_left = BUFFER_SIZE - block_start;

    if (space_left < BLOCK_SIZE) {
        // Block wraps around to the beginning of the buffer
        static char block[BLOCK_SIZE];
        memcpy(block, &buffer[block_start], space_left);
        memcpy(&block[space_left], buffer, BLOCK_SIZE - space_left);
        return block;
    } else {
        // Block is contiguous in memory
        return &buffer[block_start];
    }
}
//The above code was made by chatGPT... The prompt I used is below:
//I need to create a character buffer in C. The function controlling the buffer will take in a char pointer and append it to the character buffer. If the buffer is full, then it will go back to the beginning of the buffer again. Another function will exist to extract information from the buffer. The buffer will consist of blocks that are a certain number of characters long. You enter the block number to the function that you want to extract and the function returns a character pointer to this block.

//helper function
void print_block(int block_number)
{
    os_ClrHome();
    char block[BLOCK_SIZE];
    memset(block, 0, BLOCK_SIZE);
    strncpy(block, get_block(block_number), BLOCK_SIZE);
    printf(block);
    scroll_pos = block_number;
    // Clear the last line of the screen bc it's garbage for some reason. I'm guessing it's because of the getblock code. This is my first time really doing anything in C so idk.
    os_SetCursorPos(10 - 1, 0);
    os_PutStrFull("                "); // Fill the last line with spaces
    // Reset the cursor position
    os_SetCursorPos(0, 0);
}

//Pov: void os_GetStringInput(char *string, char *buf, size_t bufsize) is hidden in the documentation so you make an unnecessary function 
//Also couldn't figure out how to make it work soooooo maybe you try that and lmk?
char* takeInput()
{
    os_ClrHome();
    const char* chars = "\0\0\0\0\0\0\0\0\0\0\"WRMH\0\0?[VQLG\0\0:ZUPKFC\0 YTOJEB\0\0XSNIDA\0\0\0\0\0\0\0\0"; //this is awesome stole this from the documentation
    uint8_t key, i = 0;
    char *buffer = (char*)malloc(sizeof(char));
    memset(buffer, 0, sizeof(buffer));
    while ((key = os_GetCSC()) != sk_Enter) {
        if (chars[key]) {
            buffer = (char*)realloc(buffer, (i+1) * sizeof(char));
            buffer[i++] = chars[key];
            os_ClrHome();
            printf(buffer);
            //os_PutStrFull(buffer);
        }
    }
    buffer[i] = '\n';
    //buffer[i+1] = '\0'; //Idk if this works, I don't think it does and I don't think it matters. but technically should be there
    os_ClrHome();
    return buffer;
}

//Almost all of setting up serial was stolen from the documentation...
srl_device_t srl;

bool has_srl_device = false;

uint8_t srl_buf[512];

static usb_error_t handle_usb_event(usb_event_t event, void *event_data,
                                    usb_callback_data_t *callback_data __attribute__((unused))) {
    usb_error_t err;
    /* Delegate to srl USB callback */
    if ((err = srl_UsbEventCallback(event, event_data, callback_data)) != USB_SUCCESS)
        return err;
    /* Enable newly connected devices */
    if(event == USB_DEVICE_CONNECTED_EVENT && !(usb_GetRole() & USB_ROLE_DEVICE)) {
        usb_device_t device = event_data;
        printf("device connected\n");
        usb_ResetDevice(device);
    }

    /* Call srl_Open on newly enabled device, if there is not currently a serial device in use */
    if(event == USB_HOST_CONFIGURE_EVENT || (event == USB_DEVICE_ENABLED_EVENT && !(usb_GetRole() & USB_ROLE_DEVICE))) {

        /* If we already have a serial device, ignore the new one */
        if(has_srl_device) return USB_SUCCESS;

        usb_device_t device;
        if(event == USB_HOST_CONFIGURE_EVENT) {
            /* Use the device representing the USB host. */
            device = usb_FindDevice(NULL, NULL, USB_SKIP_HUBS);
            if(device == NULL) return USB_SUCCESS;
        } else {
            /* Use the newly enabled device */
            device = event_data;
        }

        /* Initialize the serial library with the newly attached device */
        srl_error_t error = srl_Open(&srl, device, srl_buf, sizeof srl_buf, SRL_INTERFACE_ANY, 9600);
        if(error) {
            /* Print the error code to the homescreen */
            printf("Error %d initting serial\n", error);
            return USB_SUCCESS;
        }
	
        printf("Ready\n");

        has_srl_device = true;
    }

    if(event == USB_DEVICE_DISCONNECTED_EVENT) {
        usb_device_t device = event_data;
        if(device == srl.dev) {
            printf("Device disconnected\n");
            srl_Close(&srl);
            has_srl_device = false;
        }
    }

    return USB_SUCCESS;
}

int main(void) {
    memset(buffer, 0, BLOCK_SIZE);
    os_ClrHome();
    const usb_standard_descriptors_t *desc = srl_GetCDCStandardDescriptors();
    /* Initialize the USB driver with our event handler and the serial device descriptors */
    usb_error_t usb_error = usb_Init(handle_usb_event, NULL, desc, USB_DEFAULT_INIT_FLAGS);
    if(usb_error) {
       usb_Cleanup();
       printf("usb init error %u\n", usb_error);
       do kb_Scan(); while(!kb_IsDown(kb_KeyClear));
       return 1;
    }
    os_EnableCursor();
    os_FontSelect(os_SmallFont);
    do {
        kb_Scan();
        
        usb_HandleEvents();

        if(has_srl_device) {
            char in_buf[64];
            memset(in_buf, 0, sizeof(in_buf));
            /* Read up to 64 bytes from the serial buffer */
            size_t bytes_read = srl_Read(&srl, in_buf, sizeof in_buf);
            /* Check for an error (e.g. device disconneced) */
            if(bytes_read < 0) {
                printf("error %d on srl_Read\n", bytes_read);
                has_srl_device = false;
            } else if(bytes_read > 0) {
                //printf(in_buf);
                //os_PutStrFull(in_buf);
                if (firstread)
                {
                    os_ClrHome();
                    printf("Connected! Press 2nd to prompt.");
                    firstread = false;
                }
                else 
                {
                    append_to_buffer(in_buf);
                    print_block(last_block_written_to);
                }
            }
            // bool key, prevkey;
            // key = kb_Data[1] == kb_2nd;
            if (kb_Data[1] & kb_2nd) {
                char* temp = takeInput();
                srl_Write(&srl, temp, strlen(temp));
                printf("Waiting for response...");
                //append_to_buffer(temp);
                //print_block(last_block_written_to);
                //os_PutStrFull(temp);
                memset(temp, 0, sizeof(temp));
            }
            // prevkey = key;
            if (kb_Data[7] & kb_Down)
            {
                scroll_pos++;
                if (scroll_pos > (BUFFER_SIZE/BLOCK_SIZE)) scroll_pos = (BUFFER_SIZE/BLOCK_SIZE);
                print_block(scroll_pos);
            }
            if (kb_Data[7] & kb_Up)
            {
                scroll_pos--;
                if (scroll_pos < 0) scroll_pos = 0;
                print_block(scroll_pos);
            }
        }

    } while(!kb_IsDown(kb_KeyClear));

    usb_Cleanup();
    return 0;
}
