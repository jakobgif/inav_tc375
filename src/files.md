# INAV src files

## File tree

```Tree
src/
┣ bl/
┃ ┗ bl_main.c (Bootloader not needed)
┣ main/
┃ ┣ blackbox/ (enabled with USE_BLACKBOX, no porting required)
┃ ┃ ┣ blackbox.c
┃ ┃ ┣ blackbox.h
┃ ┃ ┣ blackbox_encoding.c
┃ ┃ ┣ blackbox_encoding.h
┃ ┃ ┣ blackbox_fielddefs.h
┃ ┃ ┣ blackbox_io.c
┃ ┃ ┗ blackbox_io.h
┃ ┣ build/  
┃ ┃ ┣ assert.c (asserts for compile time)
┃ ┃ ┣ assert.h
┃ ┃ ┣ atomic.h (i dont know)
┃ ┃ ┣ build_config.c (not sure if needed, contains some cpu specific things)
┃ ┃ ┣ build_config.h
┃ ┃ ┣ debug.c (not sure)
┃ ┃ ┣ debug.h
┃ ┃ ┣ version.c (contains some version strings, port required)
┃ ┃ ┗ version.h
┃ ┣ cms/ (can be enabled with USE_CMS, no porting required)
┃ ┃ ┣ cms.c
┃ ┃ ┣ cms.h
┃ ┃ ┣ cms_menu_battery.c
┃ ┃ ┣ cms_menu_battery.h
┃ ┃ ┣ cms_menu_blackbox.c
┃ ┃ ┣ cms_menu_blackbox.h
┃ ┃ ┣ cms_menu_builtin.c
┃ ┃ ┣ cms_menu_builtin.h
┃ ┃ ┣ cms_menu_imu.c
┃ ┃ ┣ cms_menu_imu.h
┃ ┃ ┣ cms_menu_ledstrip.c
┃ ┃ ┣ cms_menu_ledstrip.h
┃ ┃ ┣ cms_menu_misc.c
┃ ┃ ┣ cms_menu_misc.h
┃ ┃ ┣ cms_menu_mixer_servo.c
┃ ┃ ┣ cms_menu_mixer_servo.h
┃ ┃ ┣ cms_menu_navigation.c
┃ ┃ ┣ cms_menu_navigation.h
┃ ┃ ┣ cms_menu_osd.c
┃ ┃ ┣ cms_menu_osd.h
┃ ┃ ┣ cms_menu_saveexit.c
┃ ┃ ┣ cms_menu_saveexit.h
┃ ┃ ┣ cms_menu_vtx.c
┃ ┃ ┣ cms_menu_vtx.h
┃ ┃ ┗ cms_types.h
┃ ┣ common/
┃ ┃ ┣ axis.h 
┃ ┃ ┣ bitarray.c (maybe requires port)
┃ ┃ ┣ bitarray.h
┃ ┃ ┣ calibration.c (no port)
┃ ┃ ┣ calibration.h
┃ ┃ ┣ circular_queue.c (no port)
┃ ┃ ┣ circular_queue.h
┃ ┃ ┣ color.h
┃ ┃ ┣ colorconversion.c (no port)
┃ ┃ ┣ colorconversion.h
┃ ┃ ┣ constants.h
┃ ┃ ┣ crc.c (no port, maybe can be optimized with aurix crc)
┃ ┃ ┣ crc.h
┃ ┃ ┣ encoding.c (no port)
┃ ┃ ┣ encoding.h
┃ ┃ ┣ filter.c (there is a FAST_CODE NOINLINE option required)
┃ ┃ ┣ filter.h
┃ ┃ ┣ fp_pid.c (using float, not sure if using FPU)
┃ ┃ ┣ fp_pid.h
┃ ┃ ┣ gps_conversion.c (enable using USE_GPS, no port)
┃ ┃ ┣ gps_conversion.h
┃ ┃ ┣ log.c (logging can be enabled with USE_LOG, maybe port)
┃ ┃ ┣ log.h
┃ ┃ ┣ lulu.c (using FAST_CODE)
┃ ┃ ┣ lulu.h
┃ ┃ ┣ maths.c (uses float)
┃ ┃ ┣ maths.h
┃ ┃ ┣ memory.c  (no port)
┃ ┃ ┣ memory.h
┃ ┃ ┣ olc.c (no port)
┃ ┃ ┣ olc.h
┃ ┃ ┣ printf.c (no port)
┃ ┃ ┣ printf.h
┃ ┃ ┣ quaternion.h 
┃ ┃ ┣ streambuf.c (no port)
┃ ┃ ┣ streambuf.h
┃ ┃ ┣ string_light.c (no port)
┃ ┃ ┣ string_light.h
┃ ┃ ┣ time.c (no port)
┃ ┃ ┣ time.h
┃ ┃ ┣ tristate.h
┃ ┃ ┣ typeconversion.c (no port)
┃ ┃ ┣ typeconversion.h
┃ ┃ ┣ utils.h
┃ ┃ ┣ uvarint.c (no port)
┃ ┃ ┣ uvarint.h
┃ ┃ ┗ vector.h
┃ ┣ config/ (handles config storage. for litekit use EEPROM?)
┃ ┃ ┣ config_eeprom.c (compile issues)
┃ ┃ ┣ config_eeprom.h 
┃ ┃ ┣ config_master.h
┃ ┃ ┣ config_reset.h
┃ ┃ ┣ config_streamer.c
┃ ┃ ┣ config_streamer.h
┃ ┃ ┣ config_streamer_at32f43x.c (target specific)
┃ ┃ ┣ config_streamer_extflash.c 
┃ ┃ ┣ config_streamer_file.c
┃ ┃ ┣ config_streamer_ram.c
┃ ┃ ┣ config_streamer_stm32f4.c (target specific)
┃ ┃ ┣ config_streamer_stm32f7.c (target specific)
┃ ┃ ┣ config_streamer_stm32h7.c (target specific)
┃ ┃ ┣ feature.c (no port)
┃ ┃ ┣ feature.h
┃ ┃ ┣ general_settings.c (no port)
┃ ┃ ┣ general_settings.h
┃ ┃ ┣ parameter_group.c (no port)
┃ ┃ ┣ parameter_group.h
┃ ┃ ┗ parameter_group_ids.h
┃ ┣ drivers/ (can be enabled with USE_1WIRE)
┃ ┃ ┣ 1-wire/
┃ ┃ ┃ ┣ ds2482.c
┃ ┃ ┃ ┣ ds2482.h
┃ ┃ ┃ ┣ ds_crc.c
┃ ┃ ┃ ┗ ds_crc.h
┃ ┃ ┣ accgyro/ (each IMU can be individually enabled by macro)
┃ ┃ ┃ ┣ accgyro.c
┃ ┃ ┃ ┣ accgyro.h
┃ ┃ ┃ ┣ accgyro_bmi088.c
┃ ┃ ┃ ┣ accgyro_bmi088.h
┃ ┃ ┃ ┣ accgyro_bmi160.c
┃ ┃ ┃ ┣ accgyro_bmi160.h
┃ ┃ ┃ ┣ accgyro_bmi270.c
┃ ┃ ┃ ┣ accgyro_bmi270.h
┃ ┃ ┃ ┣ accgyro_bmi270_maximum_fifo.c
┃ ┃ ┃ ┣ accgyro_fake.c
┃ ┃ ┃ ┣ accgyro_fake.h
┃ ┃ ┃ ┣ accgyro_icm20689.c
┃ ┃ ┃ ┣ accgyro_icm20689.h
┃ ┃ ┃ ┣ accgyro_icm42605.c
┃ ┃ ┃ ┣ accgyro_icm42605.h
┃ ┃ ┃ ┣ accgyro_lsm6dxx.c
┃ ┃ ┃ ┣ accgyro_lsm6dxx.h
┃ ┃ ┃ ┣ accgyro_mpu.c
┃ ┃ ┃ ┣ accgyro_mpu.h
┃ ┃ ┃ ┣ accgyro_mpu6000.c
┃ ┃ ┃ ┣ accgyro_mpu6000.h
┃ ┃ ┃ ┣ accgyro_mpu6500.c
┃ ┃ ┃ ┣ accgyro_mpu6500.h
┃ ┃ ┃ ┣ accgyro_mpu9250.c
┃ ┃ ┃ ┗ accgyro_mpu9250.h
┃ ┃ ┣ barometer/ (baros can be enabled individually by macro)
┃ ┃ ┃ ┣ barometer.h
┃ ┃ ┃ ┣ barometer_2smpb_02b.c
┃ ┃ ┃ ┣ barometer_2smpb_02b.h
┃ ┃ ┃ ┣ barometer_bmp085.c
┃ ┃ ┃ ┣ barometer_bmp085.h
┃ ┃ ┃ ┣ barometer_bmp280.c
┃ ┃ ┃ ┣ barometer_bmp280.h
┃ ┃ ┃ ┣ barometer_bmp388.c
┃ ┃ ┃ ┣ barometer_bmp388.h
┃ ┃ ┃ ┣ barometer_dps310.c
┃ ┃ ┃ ┣ barometer_dps310.h
┃ ┃ ┃ ┣ barometer_fake.c
┃ ┃ ┃ ┣ barometer_fake.h
┃ ┃ ┃ ┣ barometer_lps25h.c
┃ ┃ ┃ ┣ barometer_lps25h.h
┃ ┃ ┃ ┣ barometer_ms56xx.c
┃ ┃ ┃ ┣ barometer_ms56xx.h
┃ ┃ ┃ ┣ barometer_msp.c
┃ ┃ ┃ ┣ barometer_msp.h
┃ ┃ ┃ ┣ barometer_spl06.c
┃ ┃ ┃ ┗ barometer_spl06.h
┃ ┃ ┣ compass/ (compass can be enabled individually by macro)
┃ ┃ ┃ ┣ compass.h
┃ ┃ ┃ ┣ compass_ak8963.c
┃ ┃ ┃ ┣ compass_ak8963.h
┃ ┃ ┃ ┣ compass_ak8975.c
┃ ┃ ┃ ┣ compass_ak8975.h
┃ ┃ ┃ ┣ compass_fake.c
┃ ┃ ┃ ┣ compass_fake.h
┃ ┃ ┃ ┣ compass_hmc5883l.c
┃ ┃ ┃ ┣ compass_hmc5883l.h
┃ ┃ ┃ ┣ compass_ist8308.c
┃ ┃ ┃ ┣ compass_ist8308.h
┃ ┃ ┃ ┣ compass_ist8310.c
┃ ┃ ┃ ┣ compass_ist8310.h
┃ ┃ ┃ ┣ compass_lis3mdl.c
┃ ┃ ┃ ┣ compass_lis3mdl.h
┃ ┃ ┃ ┣ compass_mag3110.c
┃ ┃ ┃ ┣ compass_mag3110.h
┃ ┃ ┃ ┣ compass_mlx90393.c
┃ ┃ ┃ ┣ compass_mlx90393.h
┃ ┃ ┃ ┣ compass_mpu9250.c
┃ ┃ ┃ ┣ compass_mpu9250.h
┃ ┃ ┃ ┣ compass_msp.c
┃ ┃ ┃ ┣ compass_msp.h
┃ ┃ ┃ ┣ compass_qmc5883l.c
┃ ┃ ┃ ┣ compass_qmc5883l.h
┃ ┃ ┃ ┣ compass_rm3100.c
┃ ┃ ┃ ┣ compass_rm3100.h
┃ ┃ ┃ ┣ compass_vcm5883.c
┃ ┃ ┃ ┗ compass_vcm5883.h
┃ ┃ ┣ opflow/ (flow sensor)
┃ ┃ ┃ ┣ opflow.h
┃ ┃ ┃ ┣ opflow_fake.c
┃ ┃ ┃ ┣ opflow_fake.h
┃ ┃ ┃ ┣ opflow_virtual.c
┃ ┃ ┃ ┗ opflow_virtual.h
┃ ┃ ┣ pitotmeter/ (something for boats so we dont care, ahoy captain)
┃ ┃ ┃ ┣ pitotmeter.h
┃ ┃ ┃ ┣ pitotmeter_adc.c
┃ ┃ ┃ ┣ pitotmeter_adc.h
┃ ┃ ┃ ┣ pitotmeter_dlvr_l10d.c
┃ ┃ ┃ ┣ pitotmeter_dlvr_l10d.h
┃ ┃ ┃ ┣ pitotmeter_fake.c
┃ ┃ ┃ ┣ pitotmeter_fake.h
┃ ┃ ┃ ┣ pitotmeter_ms4525.c
┃ ┃ ┃ ┣ pitotmeter_ms4525.h
┃ ┃ ┃ ┣ pitotmeter_msp.c
┃ ┃ ┃ ┣ pitotmeter_msp.h
┃ ┃ ┃ ┣ pitotmeter_virtual.c
┃ ┃ ┃ ┗ pitotmeter_virtual.h
┃ ┃ ┣ rangefinder/ (rangefinder can be enabled via macros)
┃ ┃ ┃ ┣ rangefinder.h
┃ ┃ ┃ ┣ rangefinder_srf10.c
┃ ┃ ┃ ┣ rangefinder_srf10.h
┃ ┃ ┃ ┣ rangefinder_teraranger_evo.c
┃ ┃ ┃ ┣ rangefinder_teraranger_evo.h
┃ ┃ ┃ ┣ rangefinder_tof10120_i2c.c
┃ ┃ ┃ ┣ rangefinder_tof10120_i2c.h
┃ ┃ ┃ ┣ rangefinder_us42.c
┃ ┃ ┃ ┣ rangefinder_us42.h
┃ ┃ ┃ ┣ rangefinder_virtual.c
┃ ┃ ┃ ┣ rangefinder_virtual.h
┃ ┃ ┃ ┣ rangefinder_vl53l0x.c
┃ ┃ ┃ ┣ rangefinder_vl53l0x.h
┃ ┃ ┃ ┣ rangefinder_vl53l1x.c
┃ ┃ ┃ ┗ rangefinder_vl53l1x.h
┃ ┃ ┣ sdcard/ (sd card can be enabled via macro)
┃ ┃ ┃ ┣ sdcard.c
┃ ┃ ┃ ┣ sdcard.h
┃ ┃ ┃ ┣ sdcard_impl.h
┃ ┃ ┃ ┣ sdcard_sdio.c
┃ ┃ ┃ ┣ sdcard_spi.c
┃ ┃ ┃ ┣ sdcard_standard.c
┃ ┃ ┃ ┣ sdcard_standard.h
┃ ┃ ┃ ┣ sdmmc_sdio.h
┃ ┃ ┃ ┣ sdmmc_sdio_f4xx.c
┃ ┃ ┃ ┗ sdmmc_sdio_hal.c
┃ ┃ ┣ temperature/ (temperature can be enabled via macros)
┃ ┃ ┃ ┣ ds18b20.c
┃ ┃ ┃ ┣ ds18b20.h
┃ ┃ ┃ ┣ lm75.c
┃ ┃ ┃ ┣ lm75.h
┃ ┃ ┃ ┗ temperature.h

here wir start with actual drivers

┃ ┃ ┣ 1-wire.c 
┃ ┃ ┣ 1-wire.h (implementations for these functions are in drivers/1-wire/ds2482.c)
┃ ┃ ┣ adc.c (adc can be enabled by macro)
┃ ┃ ┣ adc.h
┃ ┃ ┣ adc_at32f43x.c (CPU specific)
┃ ┃ ┣ adc_impl.h 
┃ ┃ ┣ adc_stm32f4xx.c (CPU specific)
┃ ┃ ┣ adc_stm32f7xx.c (CPU specific)
┃ ┃ ┣ adc_stm32h7xx.c (CPU specific)
┃ ┃ ┣ buf_writer.c (no port)
┃ ┃ ┣ buf_writer.h
here the bus implementation starts 
┃ ┃ ┣ bus.c (no implementations in this file, only wrappers)
┃ ┃ ┣ bus.h
┃ ┃ ┣ bus_busdev_i2c.c (wrapper for bus_i2c -> this is CPU specific, slight dependency on CPU)
┃ ┃ ┣ bus_busdev_spi.c (wrapper for bus_spi -> this is CPU specific, slight dependency on CPU)
┃ ┃ ┣ bus_i2c.h
┃ ┃ ┣ bus_i2c_at32f43x.c (CPU specific i2c code)
┃ ┃ ┣ bus_i2c_hal.c (CPU specific i2c code)
┃ ┃ ┣ bus_i2c_soft.c (CPU specific i2c code)
┃ ┃ ┣ bus_i2c_stm32f40x.c (CPU specific i2c code)
┃ ┃ ┣ bus_quadspi.c (CPU specific spi code)
┃ ┃ ┣ bus_quadspi.h (CPU specific spi code)
┃ ┃ ┣ bus_quadspi_hal.c (CPU specific spi code)
┃ ┃ ┣ bus_quadspi_impl.h (CPU specific spi code)
┃ ┃ ┣ bus_spi.c (wrappers, slight dependency on CPU)
┃ ┃ ┣ bus_spi.h
┃ ┃ ┣ bus_spi_at32f43x.c (CPU specific spi code)
┃ ┃ ┣ bus_spi_hal_ll.c (CPU specific spi code)

┃ ┃ ┣ display.c (no port)
┃ ┃ ┣ display.h
┃ ┃ ┣ display_canvas.c (no port)
┃ ┃ ┣ display_canvas.h
┃ ┃ ┣ display_font_metadata.c (no port)
┃ ┃ ┣ display_font_metadata.h
┃ ┃ ┣ display_ug2864hsweg01.c (no port, enable with USE_OLED_UG2864)
┃ ┃ ┣ display_ug2864hsweg01.h
┃ ┃ ┣ display_widgets.c (no port, enable with USE_CANVAS)
┃ ┃ ┣ display_widgets.h
//TODO: make sure DMA is not used
┃ ┃ ┣ dma.h (CPU specific dma)
┃ ┃ ┣ dma_at32f43x.c (CPU specific dma)
┃ ┃ ┣ dma_stm32f4xx.c (CPU specific dma)
┃ ┃ ┣ dma_stm32f7xx.c (CPU specific dma)
┃ ┃ ┣ dma_stm32h7xx.c (CPU specific dma)
┃ ┃ ┣ exti.c (external interrupts, seems like it is not really used despite of the init)
┃ ┃ ┣ exti.h
┃ ┃ ┣ flash.c (can be enabled with USE_FLASHFS)
┃ ┃ ┣ flash.h
┃ ┃ ┣ flash_m25p16.c (support for specific flash IC)
┃ ┃ ┣ flash_m25p16.h
┃ ┃ ┣ flash_w25n01g.c
┃ ┃ ┣ flash_w25n01g.h
┃ ┃ ┣ gimbal_common.c (can be enabled with USE_SERIAL_GIMBAL)
┃ ┃ ┣ gimbal_common.h
┃ ┃ ┣ headtracker_common.c (can be enabled with USE_HEADTRACKER)
┃ ┃ ┣ headtracker_common.h
┃ ┃ ┣ i2c_application.c (file that is only included in drivers\bus_i2c_at32f43x.c and therefore not needed)
┃ ┃ ┣ i2c_application.h 
┃ ┃ ┣ io.c (GPIO functions, this needs porting)
┃ ┃ ┣ io.h
┃ ┃ ┣ io_def.h
┃ ┃ ┣ io_def_generated.h
┃ ┃ ┣ io_impl.h
┃ ┃ ┣ io_pcf8574.c (this is a io expander, only wrappers, no port)
┃ ┃ ┣ io_pcf8574.h
┃ ┃ ┣ io_port_expander.c (this is for io expander, only wrappers, no port)
┃ ┃ ┣ io_port_expander.h
┃ ┃ ┣ io_types.h
┃ ┃ ┣ irlock.c (IR lock is some guidance system, can be enabled with USE_IRLOCK)
┃ ┃ ┣ irlock.h
┃ ┃ ┣ lights_io.c (wrapper can be enabled by USE_LIGHTS)
┃ ┃ ┣ lights_io.h
┃ ┃ ┣ light_led.c (actual LED control, no port, uses io.h functions)
┃ ┃ ┣ light_led.h
┃ ┃ ┣ light_ws2811strip.c (can be enabled by USE_LED_STRIP)
┃ ┃ ┣ light_ws2811strip.h
┃ ┃ ┣ logging_codes.h
┃ ┃ ┣ max7456.c (this is a on screen display, can be enabled by USE_MAX7456, no port)
┃ ┃ ┣ max7456.h
┃ ┃ ┣ memprot.h
┃ ┃ ┣ memprot_hal.c (memprot seems to be only used by system_stm32h7xx.c, can be used to get ideas but no port)
┃ ┃ ┣ memprot_stm32h7xx.c
┃ ┃ ┣ nvic.h 
┃ ┃ ┣ osd.c (on screen display, can be enabled by USE_OSD, no port)
┃ ┃ ┣ osd.h
┃ ┃ ┣ osd_symbols.h
┃ ┃ ┣ persistent.c (save persistent data in real time clock registers mof stm32, maybe a port is required)
┃ ┃ ┣ persistent.h
┃ ┃ ┣ pinio.c (this is some spetial io device, can be enabled with USE_PINIO, no port)
┃ ┃ ┣ pinio.h
┃ ┃ ┣ pwm_esc_detect.c (can be enabled with USE_BRUSHED_ESC_AUTODETECT)
┃ ┃ ┣ pwm_esc_detect.h
┃ ┃ ┣ pwm_mapping.c (stm32 pwm uses timers thats why checking for overlap, on aurix maybe somethign different can be used, port required)
┃ ┃ ┣ pwm_mapping.h 
┃ ┃ ┣ pwm_output.c (USE_DSHOT is used to enable digital ESC data transfer, port required)
┃ ┃ ┣ pwm_output.h
┃ ┃ ┣ rcc.c (stm32 system reset and clock control, only used by stm32 specific code. , maybe can be thrown out)
┃ ┃ ┣ rcc.h
┃ ┃ ┣ rcc_at32f43x_periph.h
┃ ┃ ┣ rcc_types.h
┃ ┃ ┣ resource.c (some typedefs, no port)
┃ ┃ ┣ resource.h
┃ ┃ ┣ sdio.h
┃ ┃ ┣ sensor.h
(here the serial driver starts)
┃ ┃ ┣ serial.c (only wrappers, no port)
┃ ┃ ┣ serial.h
┃ ┃ ┣ serial_softserial.c (can be enabled with USE_SOFTSERIAL1)
┃ ┃ ┣ serial_softserial.h 
┃ ┃ ┣ serial_tcp.c (enabled with SITL_BUILD)
┃ ┃ ┣ serial_tcp.h
┃ ┃ ┣ serial_uart.c (slight port required)
┃ ┃ ┣ serial_uart.h
┃ ┃ ┣ serial_uart_at32f43x.c (CPU specific implementation)
┃ ┃ ┣ serial_uart_hal.c (CPU specific implementation)
┃ ┃ ┣ serial_uart_hal_at32f43x.c (CPU specific implementation)
┃ ┃ ┣ serial_uart_impl.h (these have to be implemented for AURIX)
┃ ┃ ┣ serial_uart_stm32f4xx.c (CPU specific implementation)
┃ ┃ ┣ serial_uart_stm32f7xx.c (CPU specific implementation)
┃ ┃ ┣ serial_uart_stm32h7xx.c (CPU specific implementation)
┃ ┃ ┣ serial_usb_vcp.c (can be enabled with USE_VCP)
┃ ┃ ┣ serial_usb_vcp.h
┃ ┃ ┣ serial_usb_vcp_at32f43x.c (can be enabled with USE_VCP)
┃ ┃ ┣ serial_usb_vcp_at32f43x.h
┃ ┃ ┣ sound_beeper.c (enbaled with BEEPER, no port)
┃ ┃ ┣ sound_beeper.h
┃ ┃ ┣ stack_check.c (enabled with STACK_CHECK, maybe port, maybe modifiy linker script)
┃ ┃ ┣ stack_check.h
┃ ┃ ┣ system.c (system init, port required)
┃ ┃ ┣ system.h
┃ ┃ ┣ system_at32f43x.c (CPU specific)
┃ ┃ ┣ system_stm32f4xx.c (CPU specific)
┃ ┃ ┣ system_stm32f7xx.c (CPU specific)
┃ ┃ ┣ system_stm32h7xx.c (CPU specific)
┃ ┃ ┣ time.c (some timing stuff, port required)
┃ ┃ ┣ time.h
┃ ┃ ┣ timer.c (cpu hardware timer, port required)
┃ ┃ ┣ timer.h
┃ ┃ ┣ timer_at32f43x.c
┃ ┃ ┣ timer_def.h
┃ ┃ ┣ timer_def_at32f43x.h
┃ ┃ ┣ timer_def_stm32f4xx.h
┃ ┃ ┣ timer_def_stm32f7xx.h
┃ ┃ ┣ timer_def_stm32h7xx.h
┃ ┃ ┣ timer_impl.h
┃ ┃ ┣ timer_impl_hal.c
┃ ┃ ┣ timer_impl_stdperiph.c
┃ ┃ ┣ timer_impl_stdperiph_at32.c
┃ ┃ ┣ timer_stm32f4xx.c
┃ ┃ ┣ timer_stm32f7xx.c
┃ ┃ ┣ timer_stm32h7xx.c
┃ ┃ ┣ uart_inverter.c (invert uart pins, enable with USE_UART_INVERTER, no port)
┃ ┃ ┣ uart_inverter.h
┃ ┃ ┣ usb_io.c (enable with USE_VCP)
┃ ┃ ┣ usb_io.h
┃ ┃ ┣ usb_msc.c (enable with USE_USB_MSC)
┃ ┃ ┣ usb_msc.h
┃ ┃ ┣ usb_msc_at32f43x.c
┃ ┃ ┣ usb_msc_f4xx.c
┃ ┃ ┣ usb_msc_f7xx.c
┃ ┃ ┣ usb_msc_h7xx.c
┃ ┃ ┣ vtx_common.c (video transmitter, seems like no port)
┃ ┃ ┗ vtx_common.h
┃ ┣ fc/
┃ ┃ ┣ cli.c
┃ ┃ ┣ cli.h
┃ ┃ ┣ config.c
┃ ┃ ┣ config.h
┃ ┃ ┣ controlrate_profile.c
┃ ┃ ┣ controlrate_profile.h
┃ ┃ ┣ controlrate_profile_config_struct.h
┃ ┃ ┣ fc_core.c
┃ ┃ ┣ fc_core.h
┃ ┃ ┣ fc_hardfaults.c
┃ ┃ ┣ fc_init.c
┃ ┃ ┣ fc_init.h
┃ ┃ ┣ fc_msp.c
┃ ┃ ┣ fc_msp.h
┃ ┃ ┣ fc_msp_box.c
┃ ┃ ┣ fc_msp_box.h
┃ ┃ ┣ fc_tasks.c
┃ ┃ ┣ fc_tasks.h
┃ ┃ ┣ firmware_update.c
┃ ┃ ┣ firmware_update.h
┃ ┃ ┣ firmware_update_common.c
┃ ┃ ┣ firmware_update_common.h
┃ ┃ ┣ multifunction.c
┃ ┃ ┣ multifunction.h
┃ ┃ ┣ rc_adjustments.c
┃ ┃ ┣ rc_adjustments.h
┃ ┃ ┣ rc_controls.c
┃ ┃ ┣ rc_controls.h
┃ ┃ ┣ rc_curves.c
┃ ┃ ┣ rc_curves.h
┃ ┃ ┣ rc_modes.c
┃ ┃ ┣ rc_modes.h
┃ ┃ ┣ rc_smoothing.c
┃ ┃ ┣ rc_smoothing.h
┃ ┃ ┣ runtime_config.c
┃ ┃ ┣ runtime_config.h
┃ ┃ ┣ settings.c
┃ ┃ ┣ settings.h
┃ ┃ ┣ settings.yaml
┃ ┃ ┣ stats.c
┃ ┃ ┗ stats.h
┃ ┣ flight/
┃ ┃ ┣ adaptive_filter.c
┃ ┃ ┣ adaptive_filter.h
┃ ┃ ┣ dynamic_gyro_notch.c
┃ ┃ ┣ dynamic_gyro_notch.h
┃ ┃ ┣ dynamic_lpf.c
┃ ┃ ┣ dynamic_lpf.h
┃ ┃ ┣ ez_tune.c
┃ ┃ ┣ ez_tune.h
┃ ┃ ┣ failsafe.c
┃ ┃ ┣ failsafe.h
┃ ┃ ┣ gyroanalyse.c
┃ ┃ ┣ gyroanalyse.h
┃ ┃ ┣ imu.c
┃ ┃ ┣ imu.h
┃ ┃ ┣ kalman.c
┃ ┃ ┣ kalman.h
┃ ┃ ┣ mixer.c
┃ ┃ ┣ mixer.h
┃ ┃ ┣ mixer_profile.c
┃ ┃ ┣ mixer_profile.h
┃ ┃ ┣ pid.c
┃ ┃ ┣ pid.h
┃ ┃ ┣ pid_autotune.c
┃ ┃ ┣ power_limits.c
┃ ┃ ┣ power_limits.h
┃ ┃ ┣ rate_dynamics.c
┃ ┃ ┣ rate_dynamics.h
┃ ┃ ┣ rpm_filter.c
┃ ┃ ┣ rpm_filter.h
┃ ┃ ┣ rth_estimator.c
┃ ┃ ┣ rth_estimator.h
┃ ┃ ┣ secondary_dynamic_gyro_notch.c
┃ ┃ ┣ secondary_dynamic_gyro_notch.h
┃ ┃ ┣ servos.c
┃ ┃ ┣ servos.h
┃ ┃ ┣ smith_predictor.c
┃ ┃ ┣ smith_predictor.h
┃ ┃ ┣ wind_estimator.c
┃ ┃ ┗ wind_estimator.h
┃ ┣ io/
┃ ┃ ┣ asyncfatfs/
┃ ┃ ┃ ┣ asyncfatfs.c
┃ ┃ ┃ ┣ asyncfatfs.h
┃ ┃ ┃ ┣ fat_standard.c
┃ ┃ ┃ ┗ fat_standard.h
┃ ┃ ┣ osd/
┃ ┃ ┃ ┣ custom_elements.c
┃ ┃ ┃ ┗ custom_elements.h
┃ ┃ ┣ adsb.c
┃ ┃ ┣ adsb.h
┃ ┃ ┣ beeper.c
┃ ┃ ┣ beeper.h
┃ ┃ ┣ cms_menu_misc.h
┃ ┃ ┣ dashboard.c
┃ ┃ ┣ dashboard.h
┃ ┃ ┣ displayport_frsky_osd.c
┃ ┃ ┣ displayport_frsky_osd.h
┃ ┃ ┣ displayport_hott.c
┃ ┃ ┣ displayport_hott.h
┃ ┃ ┣ displayport_max7456.c
┃ ┃ ┣ displayport_max7456.h
┃ ┃ ┣ displayport_msp.c
┃ ┃ ┣ displayport_msp.h
┃ ┃ ┣ displayport_msp_dji_compat.c
┃ ┃ ┣ displayport_msp_dji_compat.h
┃ ┃ ┣ displayport_msp_osd.c
┃ ┃ ┣ displayport_msp_osd.h
┃ ┃ ┣ displayport_oled.c
┃ ┃ ┣ displayport_oled.h
┃ ┃ ┣ displayport_srxl.c
┃ ┃ ┣ displayport_srxl.h
┃ ┃ ┣ dji_osd_symbols.h
┃ ┃ ┣ flashfs.c
┃ ┃ ┣ flashfs.h
┃ ┃ ┣ frsky_osd.c
┃ ┃ ┣ frsky_osd.h
┃ ┃ ┣ gimbal_serial.c
┃ ┃ ┣ gimbal_serial.h
┃ ┃ ┣ gps.c
┃ ┃ ┣ gps.h
┃ ┃ ┣ gps_fake.c
┃ ┃ ┣ gps_msp.c
┃ ┃ ┣ gps_private.h
┃ ┃ ┣ gps_ublox.c
┃ ┃ ┣ gps_ublox.h
┃ ┃ ┣ gps_ublox_utils.c
┃ ┃ ┣ gps_ublox_utils.h
┃ ┃ ┣ headtracker_msp.c
┃ ┃ ┣ headtracker_msp.h
┃ ┃ ┣ ledstrip.c
┃ ┃ ┣ ledstrip.h
┃ ┃ ┣ lights.c
┃ ┃ ┣ lights.h
┃ ┃ ┣ opflow.h
┃ ┃ ┣ opflow_cxof.c
┃ ┃ ┣ opflow_msp.c
┃ ┃ ┣ osd.c
┃ ┃ ┣ osd.h
┃ ┃ ┣ osd_canvas.c
┃ ┃ ┣ osd_canvas.h
┃ ┃ ┣ osd_common.c
┃ ┃ ┣ osd_common.h
┃ ┃ ┣ osd_dji_hd.c
┃ ┃ ┣ osd_dji_hd.h
┃ ┃ ┣ osd_grid.c
┃ ┃ ┣ osd_grid.h
┃ ┃ ┣ osd_hud.c
┃ ┃ ┣ osd_hud.h
┃ ┃ ┣ osd_joystick.c
┃ ┃ ┣ osd_joystick.h
┃ ┃ ┣ osd_utils.c
┃ ┃ ┣ osd_utils.h
┃ ┃ ┣ piniobox.c
┃ ┃ ┣ piniobox.h
┃ ┃ ┣ rangefinder.h
┃ ┃ ┣ rangefinder_benewake.c
┃ ┃ ┣ rangefinder_fake.c
┃ ┃ ┣ rangefinder_msp.c
┃ ┃ ┣ rangefinder_nanoradar.c
┃ ┃ ┣ rangefinder_usd1_v0.c
┃ ┃ ┣ rcdevice.c
┃ ┃ ┣ rcdevice.h
┃ ┃ ┣ rcdevice_cam.c
┃ ┃ ┣ rcdevice_cam.h
┃ ┃ ┣ serial.c
┃ ┃ ┣ serial.h
┃ ┃ ┣ serial_4way.c
┃ ┃ ┣ serial_4way.h
┃ ┃ ┣ serial_4way_avrootloader.c
┃ ┃ ┣ serial_4way_avrootloader.h
┃ ┃ ┣ serial_4way_impl.h
┃ ┃ ┣ serial_4way_stk500v2.c
┃ ┃ ┣ serial_4way_stk500v2.h
┃ ┃ ┣ servo_sbus.c
┃ ┃ ┣ servo_sbus.h
┃ ┃ ┣ smartport_master.c
┃ ┃ ┣ smartport_master.h
┃ ┃ ┣ statusindicator.c
┃ ┃ ┣ statusindicator.h
┃ ┃ ┣ vtx.c
┃ ┃ ┣ vtx.h
┃ ┃ ┣ vtx_control.c
┃ ┃ ┣ vtx_control.h
┃ ┃ ┣ vtx_ffpv24g.c
┃ ┃ ┣ vtx_ffpv24g.h
┃ ┃ ┣ vtx_msp.c
┃ ┃ ┣ vtx_msp.h
┃ ┃ ┣ vtx_smartaudio.c
┃ ┃ ┣ vtx_smartaudio.h
┃ ┃ ┣ vtx_string.c
┃ ┃ ┣ vtx_string.h
┃ ┃ ┣ vtx_tramp.c
┃ ┃ ┗ vtx_tramp.h
┃ ┣ msc/
┃ ┃ ┣ at32_msc_diskio.c
┃ ┃ ┣ at32_msc_diskio.h
┃ ┃ ┣ emfat.c
┃ ┃ ┣ emfat.h
┃ ┃ ┣ emfat_file.c
┃ ┃ ┣ emfat_file.h
┃ ┃ ┣ usbd_msc_desc.c
┃ ┃ ┣ usbd_msc_desc.h
┃ ┃ ┣ usbd_storage.c
┃ ┃ ┣ usbd_storage.h
┃ ┃ ┣ usbd_storage_emfat.c
┃ ┃ ┣ usbd_storage_emfat.h
┃ ┃ ┗ usbd_storage_sd_spi.c
┃ ┣ msp/
┃ ┃ ┣ msp.h
┃ ┃ ┣ msp_protocol.h
┃ ┃ ┣ msp_protocol_v2_common.h
┃ ┃ ┣ msp_protocol_v2_inav.h
┃ ┃ ┣ msp_protocol_v2_sensor.h
┃ ┃ ┣ msp_protocol_v2_sensor_msg.h
┃ ┃ ┣ msp_serial.c
┃ ┃ ┗ msp_serial.h
┃ ┣ navigation/
┃ ┃ ┣ navigation.c
┃ ┃ ┣ navigation.h
┃ ┃ ┣ navigation_declination_gen.c
┃ ┃ ┣ navigation_fixedwing.c
┃ ┃ ┣ navigation_fw_launch.c
┃ ┃ ┣ navigation_geo.c
┃ ┃ ┣ navigation_geozone.c
┃ ┃ ┣ navigation_multicopter.c
┃ ┃ ┣ navigation_pos_estimator.c
┃ ┃ ┣ navigation_pos_estimator_agl.c
┃ ┃ ┣ navigation_pos_estimator_flow.c
┃ ┃ ┣ navigation_pos_estimator_private.h
┃ ┃ ┣ navigation_private.h
┃ ┃ ┣ navigation_rover_boat.c
┃ ┃ ┣ rth_trackback.c
┃ ┃ ┣ rth_trackback.h
┃ ┃ ┣ sqrt_controller.c
┃ ┃ ┗ sqrt_controller.h
┃ ┣ programming/
┃ ┃ ┣ global_variables.c
┃ ┃ ┣ global_variables.h
┃ ┃ ┣ logic_condition.c
┃ ┃ ┣ logic_condition.h
┃ ┃ ┣ pid.c
┃ ┃ ┣ pid.h
┃ ┃ ┣ programming_task.c
┃ ┃ ┗ programming_task.h
┃ ┣ rx/
┃ ┃ ┣ crsf.c
┃ ┃ ┣ crsf.h
┃ ┃ ┣ fport.c
┃ ┃ ┣ fport.h
┃ ┃ ┣ fport2.c
┃ ┃ ┣ fport2.h
┃ ┃ ┣ frsky_crc.c
┃ ┃ ┣ frsky_crc.h
┃ ┃ ┣ ghst.c
┃ ┃ ┣ ghst.h
┃ ┃ ┣ ghst_protocol.h
┃ ┃ ┣ ibus.c
┃ ┃ ┣ ibus.h
┃ ┃ ┣ jetiexbus.c
┃ ┃ ┣ jetiexbus.h
┃ ┃ ┣ mavlink.c
┃ ┃ ┣ mavlink.h
┃ ┃ ┣ msp.c
┃ ┃ ┣ msp.h
┃ ┃ ┣ msp_override.c
┃ ┃ ┣ msp_override.h
┃ ┃ ┣ rx.c
┃ ┃ ┣ rx.h
┃ ┃ ┣ sbus.c
┃ ┃ ┣ sbus.h
┃ ┃ ┣ sbus_channels.c
┃ ┃ ┣ sbus_channels.h
┃ ┃ ┣ sim.c
┃ ┃ ┣ sim.h
┃ ┃ ┣ spektrum.c
┃ ┃ ┣ spektrum.h
┃ ┃ ┣ srxl2.c
┃ ┃ ┣ srxl2.h
┃ ┃ ┣ srxl2_types.h
┃ ┃ ┣ sumd.c
┃ ┃ ┗ sumd.h
┃ ┣ scheduler/
┃ ┃ ┣ protothreads.h
┃ ┃ ┣ scheduler.c
┃ ┃ ┗ scheduler.h
┃ ┣ sensors/
┃ ┃ ┣ acceleration.c
┃ ┃ ┣ acceleration.h
┃ ┃ ┣ barometer.c
┃ ┃ ┣ barometer.h
┃ ┃ ┣ battery.c
┃ ┃ ┣ battery.h
┃ ┃ ┣ battery_config_structs.h
┃ ┃ ┣ battery_sensor_fake.c
┃ ┃ ┣ battery_sensor_fake.h
┃ ┃ ┣ boardalignment.c
┃ ┃ ┣ boardalignment.h
┃ ┃ ┣ compass.c
┃ ┃ ┣ compass.h
┃ ┃ ┣ diagnostics.c
┃ ┃ ┣ diagnostics.h
┃ ┃ ┣ esc_sensor.c
┃ ┃ ┣ esc_sensor.h
┃ ┃ ┣ gyro.c
┃ ┃ ┣ gyro.h
┃ ┃ ┣ initialisation.c
┃ ┃ ┣ initialisation.h
┃ ┃ ┣ irlock.c
┃ ┃ ┣ irlock.h
┃ ┃ ┣ opflow.c
┃ ┃ ┣ opflow.h
┃ ┃ ┣ pitotmeter.c
┃ ┃ ┣ pitotmeter.h
┃ ┃ ┣ rangefinder.c
┃ ┃ ┣ rangefinder.h
┃ ┃ ┣ sensors.h
┃ ┃ ┣ temperature.c
┃ ┃ ┗ temperature.h
┃ ┣ startup/
┃ ┃ ┣ startup_at32f435_437.s
┃ ┃ ┣ startup_stm32f40xx.s
┃ ┃ ┣ startup_stm32f411xe.s
┃ ┃ ┣ startup_stm32f427xx.s
┃ ┃ ┣ startup_stm32f446xx.s
┃ ┃ ┣ startup_stm32f722xx.s
┃ ┃ ┣ startup_stm32f745xx.s
┃ ┃ ┣ startup_stm32f746xx.s
┃ ┃ ┣ startup_stm32f765xx.s
┃ ┃ ┣ startup_stm32h743xx.s
┃ ┃ ┗ startup_stm32h7a3xx.s
┃ ┣ target/ (included subfolders for all targets, to shorten this file all habve been deleted except FHTW and MICOAIR743)
┃ ┃ ┣ FHTW_TC375/
┃ ┃ ┃ ┣ CMakeLists.txt
┃ ┃ ┃ ┣ settings_generated.c
┃ ┃ ┃ ┣ settings_generated.h
┃ ┃ ┃ ┣ target.c
┃ ┃ ┃ ┗ target.h
┃ ┃ ┣ MICOAIR743/
┃ ┃ ┃ ┣ CMakeLists.txt
┃ ┃ ┃ ┣ config.c
┃ ┃ ┃ ┣ target.c
┃ ┃ ┃ ┗ target.h
┃ ┃ ┣ MICOAIR743AIO/
┃ ┃ ┃ ┣ CMakeLists.txt
┃ ┃ ┃ ┣ config.c
┃ ┃ ┃ ┣ target.c
┃ ┃ ┃ ┗ target.h
┃ ┃ ┣ MICOAIR743V2/
┃ ┃ ┃ ┣ CMakeLists.txt
┃ ┃ ┃ ┣ config.c
┃ ┃ ┃ ┣ target.c
┃ ┃ ┃ ┗ target.h
┃ ┃ ┣ CMakeLists.txt
┃ ┃ ┣ common.h
┃ ┃ ┣ common_hardware.c
┃ ┃ ┣ common_post.h
┃ ┃ ┣ sanity_check.h
┃ ┃ ┣ stm32f7xx_hal_conf.h
┃ ┃ ┣ stm32h7xx_hal_conf.h
┃ ┃ ┣ system.h
┃ ┃ ┣ system_at32f435_437.c
┃ ┃ ┣ system_at32f435_437.h
┃ ┃ ┣ system_stm32f4xx.c
┃ ┃ ┣ system_stm32f4xx.h
┃ ┃ ┣ system_stm32f7xx.c
┃ ┃ ┣ system_stm32f7xx.h
┃ ┃ ┣ system_stm32h7xx.c
┃ ┃ ┗ system_stm32h7xx.h
┃ ┣ telemetry/
┃ ┃ ┣ crsf.c
┃ ┃ ┣ crsf.h
┃ ┃ ┣ ghst.c
┃ ┃ ┣ ghst.h
┃ ┃ ┣ hott.c
┃ ┃ ┣ hott.h
┃ ┃ ┣ ibus.c
┃ ┃ ┣ ibus.h
┃ ┃ ┣ ibus_shared.c
┃ ┃ ┣ ibus_shared.h
┃ ┃ ┣ jetiexbus.c
┃ ┃ ┣ jetiexbus.h
┃ ┃ ┣ ltm.c
┃ ┃ ┣ ltm.h
┃ ┃ ┣ mavlink.c
┃ ┃ ┣ mavlink.h
┃ ┃ ┣ msp_shared.c
┃ ┃ ┣ msp_shared.h
┃ ┃ ┣ sbus2.c
┃ ┃ ┣ sbus2.h
┃ ┃ ┣ sbus2_sensors.c
┃ ┃ ┣ sbus2_sensors.h
┃ ┃ ┣ sim.c
┃ ┃ ┣ sim.h
┃ ┃ ┣ smartport.c
┃ ┃ ┣ smartport.h
┃ ┃ ┣ srxl.c
┃ ┃ ┣ srxl.h
┃ ┃ ┣ telemetry.c
┃ ┃ ┗ telemetry.h
┃ ┣ vcp/
┃ ┃ ┣ hw_config.c
┃ ┃ ┣ hw_config.h
┃ ┃ ┣ platform_config.h
┃ ┃ ┣ stm32_it.c
┃ ┃ ┣ stm32_it.h
┃ ┃ ┣ usb_conf.h
┃ ┃ ┣ usb_desc.c
┃ ┃ ┣ usb_desc.h
┃ ┃ ┣ usb_endp.c
┃ ┃ ┣ usb_istr.c
┃ ┃ ┣ usb_istr.h
┃ ┃ ┣ usb_prop.c
┃ ┃ ┣ usb_prop.h
┃ ┃ ┣ usb_pwr.c
┃ ┃ ┗ usb_pwr.h
┃ ┣ vcpf4/
┃ ┃ ┣ stm32f4xx_it.c
┃ ┃ ┣ stm32f4xx_it.h
┃ ┃ ┣ usbd_cdc_vcp.c
┃ ┃ ┣ usbd_cdc_vcp.h
┃ ┃ ┣ usbd_conf.h
┃ ┃ ┣ usbd_desc.c
┃ ┃ ┣ usbd_desc.h
┃ ┃ ┣ usbd_usr.c
┃ ┃ ┣ usb_bsp.c
┃ ┃ ┗ usb_conf.h
┃ ┣ vcp_hal/
┃ ┃ ┣ usbd_cdc_interface.c
┃ ┃ ┣ usbd_cdc_interface.h
┃ ┃ ┣ usbd_conf.h
┃ ┃ ┣ usbd_conf_stm32f7xx.c
┃ ┃ ┣ usbd_conf_stm32h7xx.c
┃ ┃ ┣ usbd_desc.c
┃ ┃ ┗ usbd_desc.h
┃ ┣ CMakeLists.txt
┃ ┣ main.c
┃ ┗ platform.h
┣ test/ (not needed here, content removed from this file)
┣ utils/ 
┃ ┣ assistnow.py
┃ ┣ bf2inav.py
┃ ┣ build_stamp.rb
┃ ┣ combine_tool
┃ ┣ compiler.rb
┃ ┣ declination.py
┃ ┣ def_generated.pl
┃ ┣ draft_rn_settings.rb
┃ ┣ generate-prlist-rn.rb
┃ ┣ intelhex.rb
┃ ┣ mcus.json
┃ ┣ openocd_flash.py
┃ ┣ rename-ifdefs.py
┃ ┣ scan_target_headers.py
┃ ┣ settings.rb
┃ ┣ settings.txt
┃ ┣ timer_pins.yaml
┃ ┗ update_cli_docs.py
┣ CMakeLists.txt
┗ files.md (this file)
```