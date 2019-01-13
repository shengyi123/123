package com.shengyi.jniadd;

public class JNIAdd {

    static {
        System.loadLibrary("JniAdd");
    }
    public native int Add(int x, int y);
}
