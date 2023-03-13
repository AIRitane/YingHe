idf_component_register(SRCS "main.c"
                            "../bsp/mma7600fc.c"
                            "../bsp/st7789.c"
                            "../bsp/font.c"
                            "../bsp/led.c"



                        INCLUDE_DIRS "."
                                     "../bsp"
                        )
