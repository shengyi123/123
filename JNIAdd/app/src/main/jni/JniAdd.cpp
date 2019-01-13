// Created by rabbi on 2018/10/1.
#include <jni.h>
//
#include <stdio.h>
#include "com_shengyi_jniadd_JNIAdd.h"
#include "shengyi.h"
#include "LOG.h"

/*
 * Class:     com_shengyi_jniadd_JNIAdd
 * Method:    Add
 * Signature: (II)I
 */
JNIEXPORT jint JNICALL Java_com_shengyi_jniadd_JNIAdd_Add
        (JNIEnv * env, jobject obj, jint x, jint y) {
    FILE *fp;
    fp =fopen("/storage/emulated/0/input.xlab","r");
    if(!fp){
        LOGD("123");
    }
    write();
    return x+y;
};
