/*******************************************************************************
 *                            WSAS Copyright(c)
 * File         :fm25cl64b.c
 * Summary      :fm25cl64bÇý¶¯½Ó¿Ú
 * Version      :v0.1
 * Author       :lkf
 * Date         :2025/3/12
 * Change Logs  :
 * Date         Version  Author    Notes
 * 2025/3/12   v0.1     lkf       first version
*******************************************************************************/
#include "fm25cl64b.h"
#include <rtthread.h>
#include <rtdevice.h>
#include <rtdbg.h>
#include "drv_spi.h"
#include "drivers/dev_spi.h"


#ifdef PKG_USING_FM25CL64B

#define FM25CL64B_DEVICE_NAME       "fm25cl64"

//FM25LL64B Registers
#define WRITE           0x02                                                    //Write to Memory instruction
#define WRSR            0x01                                                    //Write Status Register instruction
#define WREN            0x06                                                    //Write enable instruction
#define READ            0x03                                                    //Read from Memory instruction
#define RDSR            0x05                                                    //Read Status Register instruction
#define WRDI            0x04                                                    //
#define DUMMY           0xA5                                                    //

struct fm25cl64b_device
{
    struct rt_device parent;
    struct rt_spi_device *spi_device;
};

struct fm25cl64b_device fm25cl64b_dev;


static rt_ssize_t fm25cl64b_read(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size)
{
    struct rt_spi_device *spi_dev = ((struct fm25cl64b_device *)dev)->spi_device;
    struct rt_spi_message msg;
    rt_uint8_t cmd[3] = {READ, (rt_uint8_t)(pos >> 8), (rt_uint8_t)pos};

    msg.cs_take = 1;
    msg.send_buf = cmd;
    msg.recv_buf = RT_NULL;
    msg.length = 3;
    msg.cs_release = 0;
    msg.next = RT_NULL;
    rt_spi_transfer_message(spi_dev, &msg);
    
    msg.cs_take = 0;
    msg.send_buf = RT_NULL;
    msg.recv_buf = (rt_uint8_t *)buffer;
    msg.length = size;
    msg.cs_release = 1;
    msg.next = RT_NULL;
    rt_spi_transfer_message(spi_dev, &msg);

    return size;
}

static rt_ssize_t fm25cl64b_write(rt_device_t dev, rt_off_t pos, const void *buffer, rt_size_t size)
{
    struct rt_spi_device *spi_dev = ((struct fm25cl64b_device *)dev)->spi_device;
    rt_uint8_t *send = rt_malloc(size + 3);
    rt_uint8_t enable = WREN;
    
    if(send == RT_NULL)
    {
        return 0;
    }
    
    rt_spi_transfer(spi_dev, &enable, RT_NULL, 1);
    
    send[0] = WRITE;
    send[1] = (rt_uint8_t)(pos >> 8);
    send[2] = (rt_uint8_t)pos;
    rt_memcpy(send + 3, buffer, size);
    rt_spi_transfer(spi_dev, send, RT_NULL, size+3);
    rt_free(send);
    
    return size;
}

#ifdef RT_USING_DEVICE_OPS
const static struct rt_device_ops fram_bus_ops =
{
    RT_NULL,
    RT_NULL,
    RT_NULL,
    fm25cl64b_read,
    fm25cl64b_write,
    RT_NULL
};
#endif

int rt_hw_fm25cl64b_init(void)
{		
    rt_err_t result;
	
    rt_pin_mode(FM25CL64B_SPI_CS, PIN_MODE_OUTPUT);
    rt_pin_write(FM25CL64B_SPI_CS, PIN_HIGH);
    
    /* attach the device to spi bus*/
    result = rt_hw_spi_device_attach(FM25CL64B_SPI_BUS_NAME, FM25CL64B_SPI_DEVICE_NAME, FM25CL64B_SPI_CS);
    if (result != RT_EOK)
    {
        LOG_E("%s attach to %s faild, %d\n", FM25CL64B_SPI_DEVICE_NAME, FM25CL64B_SPI_BUS_NAME, result);
        
        return result;
    }
    
    fm25cl64b_dev.spi_device = (struct rt_spi_device *)rt_device_find(FM25CL64B_SPI_DEVICE_NAME);
    if(fm25cl64b_dev.spi_device == RT_NULL)
    {
        return -RT_EEMPTY;
    }
    
    fm25cl64b_dev.parent.type = RT_Device_Class_Char;
    fm25cl64b_dev.parent.user_data 	= RT_NULL;
#ifdef RT_USING_DEVICE_OPS
    fm25cl64b_dev.parent.ops = &fram_bus_ops;
#else
    fm25cl64b_dev.parent.init = RT_NULL;
    fm25cl64b_dev.parent.open = RT_NULL;
    fm25cl64b_dev.parent.close = RT_NULL;
    fm25cl64b_dev.parent.read = fm25cl64b_read;
    fm25cl64b_dev.parent.write = fm25cl64b_write;
    fm25cl64b_dev.parent.control = RT_NULL;
#endif
    
    rt_device_register(&fm25cl64b_dev.parent, FM25CL64B_DEVICE_NAME, RT_DEVICE_FLAG_RDWR);
    
    struct rt_spi_configuration spi_cfg = \
    {
        .mode = (rt_uint8_t)(RT_SPI_MODE_0 | SPI_MODE_MASTER | RT_SPI_MSB),
        .data_width = 8,
        .max_hz = 18000000,
    };
    
    rt_spi_configure(fm25cl64b_dev.spi_device, &spi_cfg);
    
    rt_kprintf("fm25cl64b init finish\n");

    return RT_EOK;
}
INIT_DEVICE_EXPORT(rt_hw_fm25cl64b_init);

#endif
