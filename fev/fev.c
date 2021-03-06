/*
 * =====================================================================================
 *
 *       Filename:  fev.c
 *
 *    Description:  a light-weight event framework
 *
 *        Version:  1.0
 *        Created:  2011/11/13 15/59/50
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  finaldie 
 *        Company:  
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h> 
#include <sys/stat.h>
#include <sys/types.h>
#include "fev.h"

#define FEV_MAX_EVENT_NUM   (1024 * 10)

typedef struct fev_event {
    int         mask;       //READ OR WRITE
    pfev_read   pread;
    pfev_write  pwrite;
    void*       arg;
    int         fire_idx;   //we set the idx when the event has been disabled in one loop
}fev_event;

struct fev_state{
    void*       state;
    fev_event*  fevents;
    int*        firelist;
    int         max_ev_size;
    int         fire_num;
    int         in_processing;
};

static void fev_add_firelist(fev_state* fev, int fd)
{
    fev->firelist[ fev->fire_num ] = fd;
    fev->fevents[fd].fire_idx = fev->fire_num;
    fev->fire_num++;
}

static int fev_is_fired(fev_state* fev, int fd)
{
    return fev->fevents[fd].fire_idx < fev->fire_num &&
        fev->firelist[ fev->fevents[fd].fire_idx ] == fd;
}

static void fev_clear_firelist(fev_state* fev)
{
    fev->fire_num = 0;
}

#ifdef __linux__
#include "fev_epoll.c"
#else
#error "only support linux os now!"
#endif

fev_state*    fev_create(int max_ev_size)
{
    if( max_ev_size <= 0 ) max_ev_size = FEV_MAX_EVENT_NUM;

    fev_state* fev = (fev_state*)malloc(sizeof(fev_state));
    if( !fev ){
        perror("fev create malloc");
        return NULL;
    }

    if( fev_state_create(fev, max_ev_size) ) {
        perror("fev create state");
        free(fev);
        return NULL;
    }

    fev->fevents = (fev_event*)malloc( sizeof(fev_event) * max_ev_size );
    fev->firelist = (int*)malloc( sizeof(int) * max_ev_size );
    if( !fev->fevents || !fev->firelist ) {
        perror("fev create events pool failed");
        fev_state_destroy(fev);
        free(fev->fevents);
        free(fev->firelist);
        free(fev);
        return NULL;
    }

    fev->max_ev_size = max_ev_size;
    fev_clear_firelist(fev);
    fev->in_processing = 0;

    int i;
    for(i=0; i<max_ev_size; i++) {
        fev->fevents[i].arg = NULL;
        fev->fevents[i].mask = FEV_NIL;
        fev->fevents[i].pread = NULL;
        fev->fevents[i].pwrite = NULL;
        fev->fevents[i].fire_idx = 0;
    }

    return fev;
}

void    fev_destroy(fev_state* fev)
{
    if( !fev ) return;

    fev_state_destroy(fev);
    free(fev->fevents);
    free(fev->firelist);
    free(fev);
}

// return -1 : fev is null
// return -2 : reg event failed
// return > 0 : sucess
int     fev_reg_event(fev_state* fev, int fd, int mask, pfev_read pread, pfev_write pwrite, void* arg)
{
    if( !fev ) return -1; 

    // only reversed FEV_READ & FEV_WRITE state 
    mask &= FEV_READ | FEV_WRITE;
    if( mask == FEV_NIL ) return -2;

    if( fev->fevents[fd].mask != FEV_NIL ) 
        return -3;

    if( fev_state_addevent(fev, fd, mask) == -1 ) 
        return -4;

    fev->fevents[fd].pread = pread;
    fev->fevents[fd].pwrite = pwrite;
    fev->fevents[fd].arg = arg;

    return 0;
}

int fev_add_event(fev_state* fev, int fd, int mask)
{
    if( !fev ) return -1; 

    // only reversed FEV_READ & FEV_WRITE state 
    mask &= FEV_READ | FEV_WRITE;
    if( mask == FEV_NIL ) return 0;

    if( fev->fevents[fd].mask == FEV_NIL ) 
        return -2;

    if( fev->fevents[fd].mask == mask )
        return 0;

    if( fev_state_addevent(fev, fd, mask) == -1 ) 
        return -3;

    return 0;
}

// return -1 : fev is null
// return -2 : del event failed
// return > 0 : sucess
int     fev_del_event(fev_state* fev, int fd, int mask)
{
    if( !fev ) return -1;

    // only reversed FEV_READ & FEV_WRITE state 
    mask &= FEV_READ | FEV_WRITE;
    if( mask == FEV_NIL ) return 0;

    if( fev_state_delevent(fev, fd, mask) == -1 )
        return -2;

    //finally if the fd's mask is FEV_NIL , then put the fd into firelist
    if( fev->fevents[fd].mask == FEV_NIL ) {
        fev_add_firelist(fev, fd);
    }

    return 0;
}

int     fev_poll(fev_state* fev, int timeout)
{
    if( !fev ) return 0;

    if ( fev->in_processing ) {
        perror("fev_poll shouldn't support nest call");
        return -2;
    }

    fev->in_processing = 1;
    fev_clear_firelist(fev);
    int num = fev_state_poll(fev, fev->max_ev_size, timeout);
    fev->in_processing = 0;

    return num;
}

int  fev_get_mask(fev_state* fev, int fd)
{
    if( fd < 0 || fd >= fev->max_ev_size )
        return -1;

    return fev->fevents[fd].mask;
}

int  fev_get_fd(fev_state* fev)
{
    if( !fev ) return -1;
    return fev_state_getfd(fev);
}
