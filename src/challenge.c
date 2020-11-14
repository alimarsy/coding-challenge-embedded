#include <FreeRTOS.h>
#include <task.h>
#include <console.h>
#include <stdlib.h>
#include <assert.h>

#define BUFFER_MAX_SIZE 200

typedef struct circular_buf_t {
    uint8_t buffer [BUFFER_MAX_SIZE];
    size_t head;
    size_t tail;
    size_t max; //of the buffer
    int full;
} buffer_t;

buffer_t *circular_buf_init() {
    // assert(buffer && size);

    buffer_t *cbuf = malloc(sizeof(struct circular_buf_t));
    // assert(cbuf);

    memset(cbuf->buffer, 0, BUFFER_MAX_SIZE);
    cbuf->max = BUFFER_MAX_SIZE;
    circular_buf_reset(cbuf);

    // assert(circular_buf_empty(cbuf));

    return cbuf;
}

void circular_buf_reset(buffer_t *cbuf) {
    // assert(cbuf);

    cbuf->head = 0;
    cbuf->tail = 0;
    cbuf->full = 0;
}

void circular_buf_free(buffer_t *cbuf) {
    // assert(cbuf);
    free(cbuf);
}

int circular_buf_is_full(buffer_t *cbuf) {
    // assert(cbuf);

    return cbuf->full;
}

int circular_buf_is_empty(buffer_t *cbuf) {
    return (!cbuf->full && (cbuf->head == cbuf->tail));
}

size_t circular_buf_size(buffer_t *cbuf) {
    size_t size = cbuf->max;

    if (!cbuf->full)
    {
        if (cbuf->head >= cbuf->tail)
        {
            size = (cbuf->head - cbuf->tail);
        }
        else
        {
            size = (cbuf->max + cbuf->head - cbuf->tail);
        }
    }

    return size;
}

static void advance_pointer(buffer_t *cbuf) {

    if (cbuf->full)
    {
        cbuf->tail = (cbuf->tail + 1) % cbuf->max;
    }

    cbuf->head = (cbuf->head + 1) % cbuf->max;
    cbuf->full = (cbuf->head == cbuf->tail);
}
static void retreat_pointer(buffer_t *cbuf) {
    cbuf->full = 0;
    cbuf->tail = (cbuf->tail + 1) % cbuf->max;
}

void circular_buf_put(buffer_t *cbuf, uint8_t data) {

    cbuf->buffer[cbuf->head] = data;

    advance_pointer(cbuf);
}

int circular_buf_get(buffer_t cbuf, uint8_t data) {
    int r = -1;
    while (circular_buf_is_empty(cbuf)) {
        //vTaskDelay(1);
    }
    if (!circular_buf_is_empty(cbuf))
    {
        *data = cbuf->buffer[cbuf->tail];
        retreat_pointer(cbuf);

        r = 0;
    }

    return r;
}

/**
 * Call this to "send" data over the (simulated) serial interface.
 * @param message message buffer
 * @param length length of the message
 */
void send(uint8_t *message, uint32_t length);

uint8_t EMPTY_MSG = 16;
uint8_t RESULT_MSG = 0x32;

buffer_t * buffer = NULL;

struct AddHandler
{

    int status;
    uint16_t first;
    uint16_t second;
};

void *currentCommand = NULL;
uint8_t currentCmdType = -1;

/**
 * This will get called for each byte of data received.
 * @param data received byte
 */
void receive_ISR(uint8_t data)
{
    // console_print("put byte %d\n", data);
    circular_buf_put(buffer, data);
}


/**
 * Initialize challenge. This is called once on startup, before any interrupts can occur.
 */
void challenge_init()
{
    buffer = circular_buf_init();
}

/**
 * Main function for the coding challenge. Must not return.
 *
 * Please use the following API functions if necessary:
 *
 * print string to stdout
 * console_print("format %d", 123);
 *
 * millisecond delay
 * vTaskDelay(123);
 *
 * get elapsed milliseconds
 * TickType_t ms = xTaskGetTickCount();
 */
void challenge_run()
{
    // TODO: insert awesome stuff here
    vTaskDelay(100);
    while (1) {
        uint8_t data;
        circular_buf_get(buffer, &data);
        uint8_t length = data & 0x0F; // low bits
        uint8_t type = data >> 4;     // high bits
        // console_print("got byte. (type: %d), (length: %d)\n", (int)type, (int)length);

        if (type == 1) {
            send(&EMPTY_MSG, 1);
        } else if (type == 2) {
            uint8_t bytes[4];
            circular_buf_get(buffer, bytes);
            circular_buf_get(buffer, bytes+1);
            circular_buf_get(buffer, bytes+2);
            circular_buf_get(buffer, bytes+3);
            console_print("got Add command (%d + %d)\n", bytes[0]  256 + bytes[1], bytes[2]  256 + bytes[3]);
            
            uint16_t res = (bytes[0] + bytes[2]) * 256 + bytes[1] + bytes[3];
            uint8_t result[] = { 0x32, res >> 8, res };
            
            send((uint8_t*)&result, 3);
        } else if (type == 4) {
            uint8_t bytes[3];
            circular_buf_get(buffer, bytes);
            circular_buf_get(buffer, bytes+1);
            circular_buf_get(buffer, bytes+2);
            uint16_t delay = bytes[0] * 256 + bytes[1];
            uint8_t result[] = { 0x51, bytes[2] };
            console_print("got Delay command (%dms -> %d)\n", delay, bytes[2]);
            
            vTaskDelay(delay);
            send((uint8_t*)result, 2);
        } else if (type == 6) {
            uint8_t bytes[length + 1];
            memset(bytes, 0, length + 1);
            for (int i=0;i<length;i++) {
                circular_buf_get(buffer, bytes + i);
            }
            console_print("got Log command (%s)\n", bytes);
        }
    }
}