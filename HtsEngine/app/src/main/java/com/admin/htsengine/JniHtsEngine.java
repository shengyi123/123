package com.admin.htsengine;

public class JniHtsEngine {
    static {
        System.loadLibrary("JniLib");
    }


    public native int HtsEngine(int argc ,char argv);
}
