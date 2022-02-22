add_executable(rover_lora
        rover_lora.c
        )

# pull in common dependencies and additional uart hardware support
target_link_libraries(rover_lora 
        pico_stdlib 
	pico_multicore
        hardware_uart
        )

# enable usb output, disable uart output
pico_enable_stdio_usb(rover_lora 1)
pico_enable_stdio_uart(rover_lora 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(rover_lora)

