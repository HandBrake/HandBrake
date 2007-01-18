/*
 *
 */

enum HBState_e
{
    HB_STATE_NEED_DEVICE,
    HB_STATE_SCANNING,
    HB_STATE_INVALID_DEVICE,
    HB_STATE_READY_TO_RIP,
    HB_STATE_ENCODING,
    HB_STATE_DONE,
    HB_STATE_CANCELED,
    HB_STATE_ERROR,
};

typedef struct
{
    int i_state;
    int i_error;
    int b_new;

    int i_title;
    int i_title_count;

    HBList *titleList;

    float   position;
    int     i_remaining;
    float   fps;

} HBStatus;

