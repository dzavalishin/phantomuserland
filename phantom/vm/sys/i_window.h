#ifndef _I_WINDOW
#define _I_WINDOW


struct data_area_4_window
{
    pvm_object_t                        o_pixels;       //< .i.binary object containing bitmap for window
    pvm_object_t                        callback;      // Used for callbacks - events

    uint32_t                            x, y; // in pixels
    rgba_t                              fg, bg; // colors

    //pvm_object_t                      event_handler; // connection!
    char                                title[PVM_MAX_TTY_TITLE+1]; // TODO wchar_t

    uint32_t                            autoupdate;

    drv_video_window_t                  w;
};



#endif // _I_WINDOW
