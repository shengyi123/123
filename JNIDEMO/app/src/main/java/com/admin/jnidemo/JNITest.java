package com.admin.jnidemo;

public class JNITest {
    static {
        System.loadLibrary("Jnilib");
    }

    public native int add(int x, int y);
}
