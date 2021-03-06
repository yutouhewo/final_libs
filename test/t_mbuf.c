/*
 * =====================================================================================
 *
 *       Filename:  t_mbuf.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  11/25/2011 10:37:45
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *        Company:  
 *
 * =====================================================================================
 */


#include "tu_inc.h"
#include "inc.h"
#include "mbuf.h"

void test_mbuf()
{
    mbuf* pbuf = mbuf_create(100);

    {
        int used = mbuf_used(pbuf);
        FTU_ASSERT_EQUAL_INT(0, used);

        int total_free = mbuf_total_free(pbuf);
        FTU_ASSERT_EQUAL_INT(100, total_free);

        int head_free = mbuf_head_free(pbuf);
        FTU_ASSERT_EQUAL_INT(0, head_free);

        int tail_free = mbuf_tail_free(pbuf);
        FTU_ASSERT_EQUAL_INT(100, tail_free);

        int size = mbuf_size(pbuf);
        FTU_ASSERT_EQUAL_INT(100, size);
    }

    mbuf_head_seek(pbuf, 10);
    mbuf_tail_seek(pbuf, 20);

    {
        int used = mbuf_used(pbuf);
        FTU_ASSERT_EQUAL_INT(10, used);

        int total_free = mbuf_total_free(pbuf);
        FTU_ASSERT_EQUAL_INT(90, total_free);

        int head_free = mbuf_head_free(pbuf);
        FTU_ASSERT_EQUAL_INT(10, head_free);

        int tail_free = mbuf_tail_free(pbuf);
        FTU_ASSERT_EQUAL_INT(80, tail_free);

        int size = mbuf_size(pbuf);
        FTU_ASSERT_EQUAL_INT(100, size);
    }
    
    mbuf_rewind(pbuf);

    {
        int used = mbuf_used(pbuf);
        FTU_ASSERT_EQUAL_INT(10, used);

        int total_free = mbuf_total_free(pbuf);
        FTU_ASSERT_EQUAL_INT(90, total_free);

        int head_free = mbuf_head_free(pbuf);
        FTU_ASSERT_EQUAL_INT(0, head_free);

        int tail_free = mbuf_tail_free(pbuf);
        FTU_ASSERT_EQUAL_INT(90, tail_free);

        int size = mbuf_size(pbuf);
        FTU_ASSERT_EQUAL_INT(100, size);
    }

    mbuf* new_buf = mbuf_realloc(pbuf, 100);
    {
        FTU_ASSERT_EXPRESS((new_buf==pbuf));
    }

    pbuf = mbuf_realloc(new_buf, 200);

    {
        int used = mbuf_used(pbuf);
        FTU_ASSERT_EQUAL_INT(10, used);

        int total_free = mbuf_total_free(pbuf);
        FTU_ASSERT_EQUAL_INT(190, total_free);

        int head_free = mbuf_head_free(pbuf);
        FTU_ASSERT_EQUAL_INT(0, head_free);

        int tail_free = mbuf_tail_free(pbuf);
        FTU_ASSERT_EQUAL_INT(190, tail_free);

        int size = mbuf_size(pbuf);
        FTU_ASSERT_EQUAL_INT(200, size);
    }

    pbuf = mbuf_realloc(pbuf, 5);
    {
        int used = mbuf_used(pbuf);
        FTU_ASSERT_EQUAL_INT(10, used);

        int total_free = mbuf_total_free(pbuf);
        FTU_ASSERT_EQUAL_INT(0, total_free);

        int head_free = mbuf_head_free(pbuf);
        FTU_ASSERT_EQUAL_INT(0, head_free);

        int tail_free = mbuf_tail_free(pbuf);
        FTU_ASSERT_EQUAL_INT(0, tail_free);

        int size = mbuf_size(pbuf);
        FTU_ASSERT_EQUAL_INT(10, size);
    }

    pbuf = mbuf_realloc(pbuf, 100);
    mbuf_head_seek(pbuf, 10);
    mbuf_tail_seek(pbuf, 10);
    pbuf = mbuf_realloc(pbuf, 15);

    {
        int used = mbuf_used(pbuf);
        FTU_ASSERT_EQUAL_INT(10, used);

        int total_free = mbuf_total_free(pbuf);
        FTU_ASSERT_EQUAL_INT(5, total_free);

        int head_free = mbuf_head_free(pbuf);
        FTU_ASSERT_EQUAL_INT(0, head_free);

        int tail_free = mbuf_tail_free(pbuf);
        FTU_ASSERT_EQUAL_INT(5, tail_free);

        int size = mbuf_size(pbuf);
        FTU_ASSERT_EQUAL_INT(15, size);
    }

    mbuf_delete(pbuf);
}

void test_mbuf1()
{
    mbuf* pbuf = mbuf_create(10);
    FTU_ASSERT_EXPRESS(pbuf!=NULL);

    char* push_buf[20];
    char* pop_buf[20];

    // push 1 byte data
    {
        int ret = mbuf_push(pbuf, push_buf, 1);
        FTU_ASSERT_EQUAL_INT(0, ret);

        int size = mbuf_size(pbuf);
        FTU_ASSERT_EQUAL_INT(10, size);

        int used = mbuf_used(pbuf);
        FTU_ASSERT_EQUAL_INT(1, used);

        int head_free = mbuf_head_free(pbuf);
        FTU_ASSERT_EQUAL_INT(0, head_free);

        int tail_free = mbuf_tail_free(pbuf);
        FTU_ASSERT_EQUAL_INT(9, tail_free);

        int total_free = mbuf_free(pbuf);
        FTU_ASSERT_EQUAL_INT(8, total_free);
    }

    // push 8 byte data
    {
        int ret = mbuf_push(pbuf, push_buf, 8);
        FTU_ASSERT_EQUAL_INT(0, ret);

        int size = mbuf_size(pbuf);
        FTU_ASSERT_EQUAL_INT(10, size);

        int used = mbuf_used(pbuf);
        FTU_ASSERT_EQUAL_INT(9, used);

        int head_free = mbuf_head_free(pbuf);
        FTU_ASSERT_EQUAL_INT(0, head_free);

        int tail_free = mbuf_tail_free(pbuf);
        FTU_ASSERT_EQUAL_INT(1, tail_free);

        int total_free = mbuf_free(pbuf);
        FTU_ASSERT_EQUAL_INT(0, total_free);
    }

    // continue push 1 byte data when mbuf is full
    {
        int ret = mbuf_push(pbuf, push_buf, 9);
        FTU_ASSERT_EQUAL_INT(1, ret);

        int size = mbuf_size(pbuf);
        FTU_ASSERT_EQUAL_INT(10, size);

        int used = mbuf_used(pbuf);
        FTU_ASSERT_EQUAL_INT(9, used);

        int head_free = mbuf_head_free(pbuf);
        FTU_ASSERT_EQUAL_INT(0, head_free);

        int tail_free = mbuf_tail_free(pbuf);
        FTU_ASSERT_EQUAL_INT(1, tail_free);

        int total_free = mbuf_free(pbuf);
        FTU_ASSERT_EQUAL_INT(0, total_free);
    }

    // clear and seek mbuf and push 5 bytes
    {
        mbuf_clear(pbuf);
        int total_free = mbuf_free(pbuf);
        FTU_ASSERT_EQUAL_INT(9, total_free);

        mbuf_head_seek(pbuf, 4);
        mbuf_tail_seek(pbuf, 6);

        int ret = mbuf_push(pbuf, push_buf, 5);
        FTU_ASSERT_EQUAL_INT(0, ret);

        int size = mbuf_size(pbuf);
        FTU_ASSERT_EQUAL_INT(10, size);

        int used = mbuf_used(pbuf);
        FTU_ASSERT_EQUAL_INT(7, used);

        total_free = mbuf_free(pbuf);
        FTU_ASSERT_EQUAL_INT(2, total_free);
    }

    // push 4 bytes but the mbuf only have 2 bytes left
    {
        int ret = mbuf_push(pbuf, push_buf, 4);
        FTU_ASSERT_EQUAL_INT(1, ret);

        int size = mbuf_size(pbuf);
        FTU_ASSERT_EQUAL_INT(10, size);

        int used = mbuf_used(pbuf);
        FTU_ASSERT_EQUAL_INT(7, used);

        int total_free = mbuf_free(pbuf);
        FTU_ASSERT_EQUAL_INT(2, total_free);
    }

    // push 2 bytes the mbuf is full
    {
        int ret = mbuf_push(pbuf, push_buf, 2);
        FTU_ASSERT_EQUAL_INT(0, ret);

        int size = mbuf_size(pbuf);
        FTU_ASSERT_EQUAL_INT(10, size);

        int used = mbuf_used(pbuf);
        FTU_ASSERT_EQUAL_INT(9, used);

        int total_free = mbuf_free(pbuf);
        FTU_ASSERT_EQUAL_INT(0, total_free);

        ret = mbuf_push(pbuf, push_buf, 1);
        FTU_ASSERT_EQUAL_INT(1, ret);
    }

    //---------------mbuf pop-------------------
    //pop 1 bytes
    {
        mbuf_clear(pbuf);

        int size = mbuf_size(pbuf);
        FTU_ASSERT_EQUAL_INT(10, size);

        int ret = mbuf_pop(pbuf, pop_buf, 1);
        FTU_ASSERT_EQUAL_INT(1, ret);
        
        int used = mbuf_used(pbuf);
        FTU_ASSERT_EQUAL_INT(0, used);
        
        int head_free = mbuf_head_free(pbuf);
        FTU_ASSERT_EQUAL_INT(0, head_free);

        int tail_free = mbuf_tail_free(pbuf);
        FTU_ASSERT_EQUAL_INT(10, tail_free);

        int total_free = mbuf_free(pbuf);
        FTU_ASSERT_EQUAL_INT(9, total_free);
    }

    // push 5 bytes and pop 1 bytes
    {
        int ret = mbuf_push(pbuf, push_buf, 5);
        FTU_ASSERT_EQUAL_INT(0, ret);

        ret = mbuf_pop(pbuf, pop_buf, 1);
        FTU_ASSERT_EQUAL_INT(0, ret);

        int size = mbuf_size(pbuf);
        FTU_ASSERT_EQUAL_INT(10, size);
        
        int used = mbuf_used(pbuf);
        FTU_ASSERT_EQUAL_INT(4, used);
        
        int head_free = mbuf_head_free(pbuf);
        FTU_ASSERT_EQUAL_INT(1, head_free);

        int tail_free = mbuf_tail_free(pbuf);
        FTU_ASSERT_EQUAL_INT(5, tail_free);

        int total_free = mbuf_free(pbuf);
        FTU_ASSERT_EQUAL_INT(5, total_free);
    }

    // pop 3 bytes and left 1 byte
    {
        int ret = mbuf_pop(pbuf, pop_buf, 3);
        FTU_ASSERT_EQUAL_INT(0, ret);

        int size = mbuf_size(pbuf);
        FTU_ASSERT_EQUAL_INT(10, size);
        
        int used = mbuf_used(pbuf);
        FTU_ASSERT_EQUAL_INT(1, used);
        
        int head_free = mbuf_head_free(pbuf);
        FTU_ASSERT_EQUAL_INT(4, head_free);

        int tail_free = mbuf_tail_free(pbuf);
        FTU_ASSERT_EQUAL_INT(5, tail_free);

        int total_free = mbuf_free(pbuf);
        FTU_ASSERT_EQUAL_INT(8, total_free);
    }

    // pop 2 bytes
    {
        int ret = mbuf_pop(pbuf, pop_buf, 2);
        FTU_ASSERT_EQUAL_INT(1, ret);

        int size = mbuf_size(pbuf);
        FTU_ASSERT_EQUAL_INT(10, size);
        
        int used = mbuf_used(pbuf);
        FTU_ASSERT_EQUAL_INT(1, used);
        
        int head_free = mbuf_head_free(pbuf);
        FTU_ASSERT_EQUAL_INT(4, head_free);

        int tail_free = mbuf_tail_free(pbuf);
        FTU_ASSERT_EQUAL_INT(5, tail_free);

        int total_free = mbuf_free(pbuf);
        FTU_ASSERT_EQUAL_INT(8, total_free);
    }

    // clear mbuf and seek tail < head
    {
        mbuf_clear(pbuf);
        mbuf_head_seek(pbuf, 4);
        mbuf_tail_seek(pbuf, 2);

        int size = mbuf_size(pbuf);
        FTU_ASSERT_EQUAL_INT(10, size);
        
        int used = mbuf_used(pbuf);
        FTU_ASSERT_EQUAL_INT(8, used);
        
        int head_free = mbuf_head_free(pbuf);
        FTU_ASSERT_EQUAL_INT(4, head_free);

        int tail_free = mbuf_tail_free(pbuf);
        FTU_ASSERT_EQUAL_INT(8, tail_free);

        int total_free = mbuf_free(pbuf);
        FTU_ASSERT_EQUAL_INT(1, total_free);
    }

    // pop 6 bytes
    {
        int ret = mbuf_pop(pbuf, pop_buf, 6);
        FTU_ASSERT_EQUAL_INT(0, ret);

        int size = mbuf_size(pbuf);
        FTU_ASSERT_EQUAL_INT(10, size);
        
        int used = mbuf_used(pbuf);
        FTU_ASSERT_EQUAL_INT(2, used);
        
        int head_free = mbuf_head_free(pbuf);
        FTU_ASSERT_EQUAL_INT(0, head_free);

        int tail_free = mbuf_tail_free(pbuf);
        FTU_ASSERT_EQUAL_INT(8, tail_free);

        int total_free = mbuf_free(pbuf);
        FTU_ASSERT_EQUAL_INT(7, total_free);
    }

    // pop 2 bytes and then the mbuf is empty
    {
        int ret = mbuf_pop(pbuf, pop_buf, 2);
        FTU_ASSERT_EQUAL_INT(0, ret);

        int size = mbuf_size(pbuf);
        FTU_ASSERT_EQUAL_INT(10, size);
        
        int used = mbuf_used(pbuf);
        FTU_ASSERT_EQUAL_INT(0, used);
        
        int head_free = mbuf_head_free(pbuf);
        FTU_ASSERT_EQUAL_INT(2, head_free);

        int tail_free = mbuf_tail_free(pbuf);
        FTU_ASSERT_EQUAL_INT(8, tail_free);

        int total_free = mbuf_free(pbuf);
        FTU_ASSERT_EQUAL_INT(9, total_free);
    }

    // vpop is necessary to test
    // test the tail_move
    {
        mbuf_clear(pbuf);
        mbuf_head_seek(pbuf, 4);
        mbuf_tail_seek(pbuf, 6);

        mbuf_head_move(pbuf, 4);

        int size = mbuf_size(pbuf);
        FTU_ASSERT_EQUAL_INT(10, size);
        
        int used = mbuf_used(pbuf);
        FTU_ASSERT_EQUAL_INT(2, used);
        
        int head_free = mbuf_head_free(pbuf);
        FTU_ASSERT_EQUAL_INT(4, head_free);

        int tail_free = mbuf_tail_free(pbuf);
        FTU_ASSERT_EQUAL_INT(4, tail_free);

        int total_free = mbuf_free(pbuf);
        FTU_ASSERT_EQUAL_INT(7, total_free);
    }

    // move 2 bytes
    {
        mbuf_head_move(pbuf, 2);

        int size = mbuf_size(pbuf);
        FTU_ASSERT_EQUAL_INT(10, size);
        
        int used = mbuf_used(pbuf);
        FTU_ASSERT_EQUAL_INT(0, used);
        
        int head_free = mbuf_head_free(pbuf);
        FTU_ASSERT_EQUAL_INT(6, head_free);

        int tail_free = mbuf_tail_free(pbuf);
        FTU_ASSERT_EQUAL_INT(4, tail_free);

        int total_free = mbuf_free(pbuf);
        FTU_ASSERT_EQUAL_INT(9, total_free);
    }

    {
        mbuf_clear(pbuf);
        mbuf_head_seek(pbuf, 6);
        mbuf_tail_seek(pbuf, 4);

        mbuf_head_move(pbuf, 5);

        int size = mbuf_size(pbuf);
        FTU_ASSERT_EQUAL_INT(10, size);
        
        int used = mbuf_used(pbuf);
        FTU_ASSERT_EQUAL_INT(3, used);
        
        int head_free = mbuf_head_free(pbuf);
        FTU_ASSERT_EQUAL_INT(1, head_free);

        int tail_free = mbuf_tail_free(pbuf);
        FTU_ASSERT_EQUAL_INT(6, tail_free);

        int total_free = mbuf_free(pbuf);
        FTU_ASSERT_EQUAL_INT(6, total_free);
    }

    mbuf_delete(pbuf);
}
