package com.admin.bp;

public class JniBp {
    static{
        System.loadLibrary("JniBp");
    }
    public native int Bp(int argc, String argv);
}
